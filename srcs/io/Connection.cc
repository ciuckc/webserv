#include "Connection.h"

#include "io/task/IOTask.h"
#include "io/task/ReadRequest.h"
#include "io/task/SendResponse.h"
#include "http/ErrorResponse.h"

Connection::Connection(int fd, EventQueue& event_queue, BufferPool<>& buf_mgr, const host_map_t& host_map)
    : host_map_(host_map),
      socket_(fd),
      buffer_(buf_mgr),
      event_queue_(event_queue) {
  awaitRequest();
  event_queue_.add(fd);
  last_event_ = std::time(nullptr);
}

Connection::~Connection() {
  // Destructing the socket removes the connection from the eventqueue
  iqueue_.clear();
  oqueue_.clear();
}

bool Connection::handle(EventQueue::event_t& event) {
  if (EventQueue::isWrHangup(event)) {
    Log::info('[', socket_.get_fd(), "]\tClient dropped connection\n");
    return true;
  }
  if (EventQueue::isRead(event)) {
    WS::IOStatus in_status = handleIn();
    if (in_status == WS::IO_FAIL)
      return true;
  }
  if (EventQueue::isWrite(event)) {
    WS::IOStatus out_status = handleOut();
    if (out_status == WS::IO_FAIL)
      return true;
    if (out_status == WS::IO_GOOD && oqueue_.empty()) {
      if (!keep_alive_)
        shutdown();
      event_queue_.mod(socket_.get_fd(), EventQueue::in);
    }
  }
  if (EventQueue::isRdHangup(event)) {
    Log::debug('[', socket_.get_fd(), "]\tClient done transmitting\n");
    client_fin_ = true;
  }
  socket_.flush();
  if (client_fin_ && oqueue_.empty() && !buffer_.needWrite()) {
    return true;
  }
  last_event_ = std::time(nullptr);
  return false;
}

WS::IOStatus Connection::handleIn() {
  WS::IOStatus status = WS::IO_GOOD;
  while (!iqueue_.empty()) {
    if (buffer_.readFailed()) {
      if (status == WS::IO_WAIT)
        return status; // We've already read all we can
      status = buffer_.readIn(socket_);
      if (status == WS::IO_FAIL)
        return status;
    }
    ITask& task = *iqueue_.front();
    if (task(*this)) {
      task.onDone(*this);
      iqueue_.pop_front();
    }
  }
  return status;
}

WS::IOStatus Connection::handleOut() {
  WS::IOStatus status = WS::IO_GOOD;

  while (!oqueue_.empty() || buffer_.needWrite()) {
    if (buffer_.needWrite()) {
      if (status == WS::IO_WAIT)
        return status;
      status = buffer_.writeOut(socket_);
      if (status == WS::IO_FAIL)
        return status;
      continue;
    }
    OTask& task = *oqueue_.front();
    if (task(*this)) {
      task.onDone(*this);
      oqueue_.pop_front();
    }
  }
  if (status == WS::IO_GOOD)  // We can't let this sit in the buffer
    status = buffer_.writeOut(socket_);
  return status;
}

void Connection::addTask(ITask::ptr_type&& task) {
  iqueue_.push_back(std::forward<ITask::ptr_type>(task));
}

void Connection::addTask(OTask::ptr_type&& task) {
  oqueue_.push_back(std::forward<OTask::ptr_type>(task));
}

Socket& Connection::getSocket() {
  return socket_;
}

ConnectionBuffer& Connection::getBuffer() {
  return buffer_;
}

const Connection::host_map_t& Connection::getHostMap() const {
  return host_map_;
}

void Connection::shutdown() {
  Log::debug('[', socket_.get_fd(), "]\tServer done transmitting\n");
  socket_.shutdown(SHUT_WR);
}

void Connection::awaitRequest() {
  //todo: rename ReadRequest RequestReader and make class var instead of the request itself?
  //  will we still need the RequestReader while writing output?
  addTask(std::make_unique<ReadRequest>());
}

void Connection::enqueueResponse(Response&& response) {
  if (++request_count_ > WS::max_requests) {
    keep_alive_ = false;
    Log::debug('[', socket_.get_fd(), "] Max requests reached\n");
  }
  if (oqueue_.empty()) {
    auto dir = keep_alive_ ? EventQueue::both : EventQueue::out;
    event_queue_.mod(socket_.get_fd(), dir);
  }
  if (!keep_alive_)
    response.addHeader("connection: close\r\n");
  response.setKeepAlive(WS::timeout, WS::max_requests);
  addTask(std::make_unique<SendResponse>(std::forward<Response>(response)));
}

void Connection::timeout() {
  Log::debug('[', socket_.get_fd(), "] Timed out\n");
  keep_alive_ = false;
  request_count_ = 0; // so we don't get 2 log messages
  enqueueResponse(ErrorResponse(408));
}

bool Connection::stale(time_t now) const {
  return now - last_event_ > WS::timeout;
}
