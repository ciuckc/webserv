#include "Connection.h"

#include "io/task/IOTask.h"
#include "io/task/ReadRequest.h"

Connection::Connection(int fd, EventQueue& event_queue, BufferPool<>& buf_mgr)
    : socket_(fd), buffer_(buf_mgr), event_queue_(event_queue) {
  addTask(new ReadRequest(request_));
  event_queue_.add(fd);
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
  // For good measure?
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

void Connection::addTask(ITask* task) {
  iqueue_.push_back(std::unique_ptr<ITask>(task));
}

void Connection::addTask(OTask* task) {
  if (oqueue_.empty())
    event_queue_.mod(socket_.get_fd(), keep_alive_ ? EventQueue::both : EventQueue::out);
  oqueue_.push_back(std::unique_ptr<OTask>(task));
}

Socket& Connection::getSocket() {
  return socket_;
}
ConnectionBuffer& Connection::getBuffer() {
  return buffer_;
}
void Connection::shutdown() {
  Log::debug('[', socket_.get_fd(), "]\tServer done transmitting\n");
  socket_.shutdown(SHUT_WR);
}
