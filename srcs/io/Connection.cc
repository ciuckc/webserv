#include "Connection.h"

#include "io/task/IOTask.h"
#include "io/task/ReadRequest.h"
#include "io/task/SendResponse.h"

Connection::Connection(Socket&& socket, EventQueue& event_queue, const host_map_t& host_map)
    : host_map_(host_map),
      socket_(std::forward<Socket>(socket)),
      buffer_(),
      event_queue_(event_queue) {
  event_queue_.add(socket_.get_fd());
  last_event_ = std::time(nullptr);
}

Connection::~Connection() = default;

bool Connection::handle(EventQueue::event_t& event) {
  if (EventQueue::isWrHangup(event)) {
    Log::info(*this, "Client closed connection\n");
    return true;
  }
  if (EventQueue::isRead(event))
    if (handleIn() == WS::IO_FAIL)
      return true;
  if (EventQueue::isWrite(event))
    if (handleOut() == WS::IO_FAIL)
      return true;

  if (EventQueue::isRdHangup(event)) {
    Log::debug(*this, "Client done transmitting\n");
    client_fin_ = true;
    if (oqueue_.empty() && buffer_.outEmpty())
      return true;
  }

  last_event_ = std::time(nullptr);
  return false;
}

WS::IOStatus Connection::handleIn() {
  if (reset_)
    return WS::IO_WAIT;

  WS::IOStatus status = WS::IO_GOOD;
  while (status == WS::IO_GOOD) {
    if (buffer_.needRead() && !buffer_.readIn(socket_, status))
        return status;
    if (iqueue_.empty() && !awaitRequest())
        return WS::IO_WAIT;
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

  while (buffer_.needWrite() || !oqueue_.empty()) {
    if (buffer_.needWrite()) {
      if (!buffer_.writeOut(socket_, status))
        return status;
      continue;
    }
    OTask& task = *oqueue_.front();
    if (task(*this)) {
      task.onDone(*this);
      oqueue_.pop_front();
    }
  }
  if (!buffer_.outEmpty() && !buffer_.writeOut(socket_, status))
      return status;
  if (!keep_alive_)
    shutdown();
  if (client_fin_ || reset_)
    return WS::IO_FAIL; // This destructs the connection
  event_queue_.mod(socket_.get_fd(), EventQueue::in);
  return WS::IO_GOOD;
}

void Connection::addTask(ITask::ptr_type&& task) {
  iqueue_.push_back(std::forward<ITask::ptr_type>(task));
}

void Connection::addTask(OTask::ptr_type&& task) {
  oqueue_.push_back(std::forward<OTask::ptr_type>(task));
}

ConnectionBuffer& Connection::getBuffer() {
  return buffer_;
}

const Connection::host_map_t& Connection::getHostMap() const {
  return host_map_;
}

void Connection::shutdown() {
  Log::debug(*this, "Server done transmitting\n");
  socket_.shutdown(SHUT_WR);
}

bool Connection::awaitRequest() {
  if (!keep_alive_)
    return false;
  if (++request_count_ > WS::max_requests) {
    // Don't read more, do not pass start, do not get 200
    Log::warn(*this, "Max requests exceeded\n");
    enqueueResponse(std::forward<Response>(Response::builder().message(429).build()));
    reset_ = true;
    return false;
  } else if (request_count_ == WS::max_requests) {
    Log::debug(*this, "Max requests reached\n");
    keep_alive_ = false;
  }
  addTask(std::make_unique<ReadRequest>());
  return true;
}

void Connection::enqueueResponse(Response&& response) {
  if (oqueue_.empty())
    event_queue_.mod(socket_.get_fd(), EventQueue::both);
  if (!keep_alive_)
    response.addHeader("connection: close\r\n");
  response.setKeepAlive(WS::timeout, WS::max_requests);
  addTask(std::make_unique<SendResponse>(std::forward<Response>(response)));
}

void Connection::timeout() {
  Log::debug(*this, "Timed out\n");
  keep_alive_ = false;
  reset_ = true;
  request_count_ = 0; // so we don't get 2 log messages
  Response response;
  response.setMessage(408);
  enqueueResponse(std::move(response));
}

bool Connection::stale(time_t now) const {
  return now - last_event_ > WS::timeout;
}

bool Connection::idle() const {
  return oqueue_.empty();
}

const std::string& Connection::getName() const {
  return socket_.getName();
}
