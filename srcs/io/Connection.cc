#include "Connection.h"

#include "io/task/IOTask.h"

Connection::Connection(int fd, EventQueue& event_queue, BufferPool& buf_mgr)
    : socket_(fd), buffer_(buf_mgr), event_queue_(event_queue), should_close_() {
  addTask(new ReadRequest());
}

Connection::~Connection() {
  event_queue_.del(socket_.get_fd());
  iqueue_.clear();
  oqueue_.clear();
}

void Connection::handle(EventQueue::event& event) {
  WS::IOStatus in_status = WS::FULL;
  WS::IOStatus out_status = WS::FULL;
  if (EventQueue::isRead(event)) {
    in_status = WS::OK;
    handleIn(in_status);
  }
  if (EventQueue::isWrite(event)) {
    out_status = WS::OK;
    handleOut(out_status);
    writing_ = (out_status == WS::FULL);
  }
  uint32_t flags = 0;
  if (writing_ || !oqueue_.empty())
    flags |= EV_OUT;
  if (!iqueue_.empty())
    flags |= EV_IN;
  event_queue_.mod(socket_.get_fd(), flags);
  // For good measure?
  socket_.flush();
}

void Connection::handleIn(WS::IOStatus& status) {
  while (!iqueue_.empty()) {
    if (buffer_.readFailed()) {
      if (status != WS::OK) {
        return;
      }
      status = buffer_.readIn(socket_);
    }
    if (status == WS::ERR) {
      should_close_ = true;
      return;
    }
    ITask& task = *iqueue_.front();
    if (task(*this)) {
      task.onDone(*this);
      iqueue_.pop_front();
    }
  }
}

void Connection::handleOut(WS::IOStatus& status) {
  while (status == WS::OK && (!oqueue_.empty() || buffer_.needWrite())) {
    if (buffer_.needWrite()) {
      status = buffer_.writeOut(socket_);
      continue;
    }
    OTask& task = *oqueue_.front();
    if (task(*this)) {
      task.onDone(*this);
      oqueue_.pop_front();
    }
  }
  if (status == WS::OK)  // We can't let this sit in the buffer
    status = buffer_.writeOut(socket_);
}

void Connection::addTask(ITask* task) {
  if (iqueue_.empty())
    event_queue_.add(socket_.get_fd(), EV_IN);
  iqueue_.push_back(std::unique_ptr<ITask>(task));
}

void Connection::addTask(OTask* task) {
  if (oqueue_.empty())
    event_queue_.add(socket_.get_fd(), EV_OUT);
  oqueue_.push_back(std::unique_ptr<OTask>(task));
}

Socket& Connection::getSocket() {
  return socket_;
}
ConnectionBuffer& Connection::getBuffer() {
  return buffer_;
}
