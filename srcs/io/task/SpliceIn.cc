#include "SpliceIn.h"

#include "Server.h"
#include "io/Connection.h"
#include "util/Log.h"

SpliceIn::SpliceIn(Server& server, Connection& conn, int pipe_fd, size_t len) : server_(server) {
  auto handler = std::make_unique<OHandler>(server, *this, conn, pipe_fd, len);
  handler_ = handler.operator->();
  server_.add_sub(std::move(handler));
}

SpliceIn::~SpliceIn() {
  if (!done_ && !fail_)
    server_.del_sub(handler_->getIndex());
}

WS::IOStatus SpliceIn::operator()(Connection&) {
  if (fail_)
    return WS::IO_FAIL;
  if (done_)
    return WS::IO_GOOD;
  handler_->enableFilter(server_.getEventQueue(), EventQueue::out);
  return WS::IO_AGAIN; // Connection will block before running tasks if buffer is full
}

void SpliceIn::onDone(Connection& connection) {
  if (fail_) // don't need to log this
    return;
  Log::debug(connection, "Done reading body into pipe\n");
  connection.setInSize(); // reset to default
}
void SpliceIn::setDone() {
  done_ = true;
}
void SpliceIn::setFail() {
  fail_ = true;
}

SpliceIn::OHandler::OHandler(Server& server, SpliceIn& parent, Connection& conn, int pipe_fd, size_t len)
    : Handler(pipe_fd, 0, 1000), server_(server), parent_(parent), connection_(conn), buffer_(connection_.getInBuffer()),
      remaining_(len), name_(Str::join("SpliceIn::OHandler(", std::to_string(pipe_fd), ")")) {}

SpliceIn::OHandler::~OHandler() {
  close(fd_);
}

const std::string& SpliceIn::OHandler::getName() const {
  return name_;
}

bool SpliceIn::OHandler::handleTimeout(Server& server, bool) {
  connection_.handleTimeout(server, false);
  return false;
}
bool SpliceIn::OHandler::handleWrite() {
  if (buffer_.capacity() != RingBuffer::file_buf_size_) // first run
    connection_.setInSize(RingBuffer::file_buf_size_);
  if (buffer_.empty()) {
    delFilter(EventQueue::out);
    return false;
  }
  connection_.enableFilter(server_.getEventQueue(), EventQueue::in);
  WS::IOStatus status = buffer_.write(fd_, remaining_);
  if (status != WS::IO_GOOD)
    return handleError();
  if (remaining_ == 0) {
    parent_.setDone();
    connection_.notifyInDone(false);
    return true;
  }
  return false;
}
bool SpliceIn::OHandler::handleError() {
  parent_.setFail();
  connection_.notifyInDone(true);
  return true;
}
bool SpliceIn::OHandler::handleRead() {
  abort();
}
bool SpliceIn::OHandler::handleRHup() {
  abort();
}
bool SpliceIn::OHandler::handleWHup() {
  abort();
}
