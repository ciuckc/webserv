#include "Connection.h"

#include <sys/socket.h>

#include "io/task/ReadRequest.h"
#include "io/task/SendResponse.h"
#include "util/Log.h"

Connection::Connection(Server& srv, Socket&& sock, const std::map<std::string, ConfigServer&>& hosts)
    : Handler(sock.get_fd(), EventQueue::in | EventQueue::r_hup, WS::timeout * 1000),
    server_(srv), socket_(std::forward<Socket>(sock)),
    in_buffer_(), out_buffer_(), host_map_(hosts) {}

Connection::~Connection() = default;

bool Connection::handleWHup() {
  Log::info(*this, "Client closed connection\n");
  return true;
}

bool Connection::handleRead() {
  WS::IOStatus status;

  if (in_buffer_.full()) {
    Log::debug(*this, "Removing read filter, in buffer full (blocked)\n");
    delFilter(EventQueue::in);
    return false;
  }
  if (in_buffer_size_ != in_buffer_.capacity()) {
    if (in_buffer_.dataLen() < in_buffer_size_)
      in_buffer_.resize(in_buffer_size_);
    else goto tasks;
  }
  status = in_buffer_.read_sock(socket_);
  if (status == WS::IO_EOF) {
    // This only happens when we also get a read hangup
    Log::debug(*this, "Removing read filter, read 0 bytes from socket\n");
    delFilter(EventQueue::in);
    if (in_buffer_.empty())
      return false;
  } else if (status == WS::IO_FAIL) {
    return handleError();
  }

  tasks:
  do {
    if (in_queue_.empty() && !awaitRequest())
      return false;
    status = runITask();
  } while (status == WS::IO_GOOD && !in_buffer_.empty());

  if (status == WS::IO_FAIL) {
    Log::debug(*this, "Closing read, task failed\n");
    // so a read task failed halfway through, don't read anymore
    closeRead();
    return out_queue_.empty() && out_buffer_.empty();
  }
  if (status == WS::IO_BLOCKED) {
    Log::debug(*this, "Removing read filter, task returned blocked\n");
    delFilter(EventQueue::in);
  }
  return false;
}

bool Connection::handleWrite() {
  WS::IOStatus status;
  if (out_buffer_size_ != out_buffer_.capacity()) {
    if (out_buffer_.dataLen() < out_buffer_size_)
      out_buffer_.resize(out_buffer_size_);
    else goto io;
  }
  while (!out_buffer_.full() && !out_queue_.empty()) {
    status = runOTask();
    if (status != WS::IO_GOOD)
      break;
  }
  if (status == WS::IO_BLOCKED) {
    Log::debug(*this, "Write task blocked, removing write filter\n");
    delFilter(EventQueue::out);
    if (out_buffer_.empty())
      return false;
  } else if (status == WS::IO_FAIL) { // yeah.. what do we do now? close the connection?
    Log::debug(*this, "Write task failed, resetting connection\n");
    return true;
  }



  io:
  if (!out_buffer_.empty()) {
    if (out_buffer_.write_sock(socket_) != WS::IO_GOOD) // if write on a socket returns 0 something is wrong as well
      return true;
  } else {
    Log::debug(*this, "Write task succeeded but buffer empty, removing write filter\n");
    delFilter(EventQueue::out);
  }
  if (!out_queue_.empty() || !out_buffer_.empty())
    return false;
  Log::trace(*this, "All out tasks done\n");
  // All tasks done and all data written
  if (read_closed_)
    return true;
  if (!keep_alive_)
    shutdown();
  delFilter(EventQueue::out);
  return false;
}

bool Connection::handleRHup() {
  Log::debug(*this, "Client done transmitting\n");
  delFilter(EventQueue::in);
  read_closed_ = true;
  return out_queue_.empty() && out_buffer_.empty();
}

bool Connection::handleError() {
  return true;
}

WS::IOStatus Connection::runITask() {
  auto& task = *in_queue_.front();
  WS::IOStatus result = task(*this);
  if (result == WS::IO_GOOD) {
    task.onDone(*this);
    in_queue_.pop_front();
  }
  return result;
}

WS::IOStatus Connection::runOTask() {
  auto& task = *out_queue_.front();
  WS::IOStatus result = task(*this);
  if (result == WS::IO_GOOD) {
    task.onDone(*this);
    out_queue_.pop_front();
  }
  return result;
}

void Connection::addTask(std::unique_ptr<ITask>&& task) {
  in_queue_.push_back(std::forward<std::unique_ptr<ITask>>(task));
}

void Connection::addTask(std::unique_ptr<OTask>&& task) {
  out_queue_.push_back(std::forward<std::unique_ptr<OTask>>(task));
}

static inline void errorResponse(Connection& c, int err) {
  c.enqueueResponse(
      std::forward<Response>(
          Response::builder().message(err).build()));

}

bool Connection::awaitRequest() {
  if (!keep_alive_ || read_closed_) {
    delFilter(EventQueue::in); // should be unreachable
    return false;
  }
  if (++request_count_ > WS::max_requests) {
    // Don't read more, do not pass start, do not get 200
    Log::warn(*this, "Max requests exceeded\n");
    errorResponse(*this, 429);
    closeRead();
    return false;
  } else if (request_count_ == WS::max_requests) {
    Log::debug(*this, "Max requests reached\n");
    keep_alive_ = false;
  }
  addTask(std::make_unique<ReadRequest>());
  return true;
}

void Connection::enqueueResponse(Response&& response) {
  if (out_queue_.empty())
    addFilter(EventQueue::out);
  if (keep_alive_)
    response.setKeepAlive(WS::timeout, WS::max_requests);
  else
    response.addHeader("connection: close\r\n");
  addTask(std::make_unique<SendResponse>(std::forward<Response>(response)));
}

bool Connection::handleTimeout(Server& server, bool src) {
  // does this timeout come from socket or pipe?
  if (src) {
    Log::debug(*this, "Timed out\n");
    if (!out_queue_.empty() || !out_buffer_.empty()) {
      // Timed out while writing response
      shutdown();
      delFilter(EventQueue::out);
    } else {
      keep_alive_ = false;
      errorResponse(*this, 408);
    }
    delFilter(EventQueue::in);
  } else {
    Log::debug(*this, "Task failed asynchronously, queueing deletion\n");
    // task timed out :( cgi hung, what do? Let's just close lmao
    server.run_later([&server, id = getIndex()](){server.del_sub(id);});
  }
  return false;
}

void Connection::shutdown() {
  Log::debug(*this, "Server done transmitting\n");
  socket_.shutdown(SHUT_WR);
}

Server& Connection::getServer() {
  return server_;
}

RingBuffer& Connection::getInBuffer() {
  return in_buffer_;
}
RingBuffer& Connection::getOutBuffer() {
  return out_buffer_;
}
void Connection::setInSize(size_t size) {
  in_buffer_size_ = size;
  if (in_buffer_.dataLen() <= size)
    in_buffer_.resize(size);
}
void Connection::setOutSize(size_t size) {
  out_buffer_size_ = size;
  if (out_buffer_.dataLen() <= size)
    out_buffer_.resize(size);
}

const std::map<std::string, ConfigServer &>& Connection::getHostMap() const {
  return host_map_;
}

const std::string& Connection::getName() const {
  return socket_.getName();
}

const std::string& Connection::getAddress() const {
  return socket_.getAddress();
}

void Connection::setKeepAlive(bool keepAlive) {
  keep_alive_ = keepAlive;
}

void Connection::closeRead() {
  read_closed_ = true;
  delFilter(EventQueue::in);
}

void Connection::notifyInDone(bool error) {
  if (error)
    handleTimeout(server_, false); // this queues our destruction
  in_queue_.front()->onDone(*this);
  in_queue_.pop_front();
}
