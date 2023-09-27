#include "SpliceOut.h"

#include "Server.h"
#include "io/Connection.h"
#include "util/Log.h"

SpliceOut::SpliceOut(Server& server, Connection& conn, const std::string& cgi_path, const ConfigServer& cfg, int pipe_fd)
  : server_(server) {
  auto handler = std::make_unique<IHandler>(server, *this, conn, cgi_path, cfg, pipe_fd);
  handler_ = handler.operator->();
  server.add_sub(std::move(handler));
}

SpliceOut::~SpliceOut() {
  if (!done_ && !fail_)
    server_.del_sub(handler_->getIndex());
}

WS::IOStatus SpliceOut::operator()(Connection& connection) {
  if (fail_)
    return WS::IO_FAIL;
  if (done_)
    return WS::IO_GOOD;
  handler_->enableFilter(server_.getEventQueue(), EventQueue::in);
  if (connection.getOutBuffer().empty())
    return WS::IO_BLOCKED;
  return WS::IO_AGAIN;
}

void SpliceOut::onDone(Connection& connection) {
  Log::debug(connection, "Done reading body from pipe\n");
  connection.setOutSize();
}
void SpliceOut::setDone() {
  done_ = true;
}
void SpliceOut::setFail() {
  fail_ = true;
}

SpliceOut::IHandler::IHandler(Server& server, SpliceOut& parent, Connection& connection,
                              const std::string& cgi_path, const ConfigServer& cfg, int pipe_fd)
    : Handler(pipe_fd, EventQueue::in, 1000), server_(server),
      parent_(parent), connection_(connection), cgi_path_(cgi_path),
      cfg_(cfg), buffer_(connection.getOutBuffer()),
      name_(Str::join("SpliceOut::IHandler(", std::to_string(pipe_fd), ")")),
      state_headers_(true), chunked_(false) {}

SpliceOut::IHandler::~IHandler() {
  close(fd_);
}

const std::string& SpliceOut::IHandler::getName() const {
  return name_;
}

bool SpliceOut::IHandler::handleTimeout(Server& server, bool) {
  connection_.handleTimeout(server, false);
  return false;
}

void SpliceOut::IHandler::chunkBuffer_()
{
  std::ostringstream stream;
  stream << std::hex << buffer_.totalSize();
  buffer_.prepend("\r\n");
  buffer_.prepend(stream.str());
  buffer_.put("\r\n");
}

// keep reading buffer until headers are complete
// pass headers to cgi and chunk the body
void SpliceOut::IHandler::readBuffer_()
{
  if (state_headers_) {
    std::string tmp;
    while (buffer_.getline(tmp)) {
      if (!tmp.compare("\n") || !tmp.compare("\r\n")) { // end of headers
        if (!buffer_.empty()) { // if there is a body in buf we chunk
          chunked_ = true;
          chunkBuffer_();
        }
        Cgi cgi(cfg_, connection_, cgi_path_);
        cgi.act(headers_);
        state_headers_ = false;
        return;
      }
      headers_.append(tmp);
    }
  }
  else if (chunked_) {
    chunkBuffer_();
  }
}

bool SpliceOut::IHandler::handleRead() {
  if (buffer_.capacity() != RingBuffer::file_buf_size_) // first run
    connection_.setOutSize(RingBuffer::file_buf_size_);
  if (buffer_.full()) {
    delFilter(EventQueue::in);
    // we're blocked!
    return false;
  }
  connection_.enableFilter(server_.getEventQueue(), EventQueue::out);
  WS::IOStatus status = buffer_.read(fd_);
  if (status == WS::IO_EOF) {
    parent_.setDone();
    if (chunked_) {
      buffer_.put("0\r\n\r\n"); // closing chunk
    }
    return true;
  } else if (status == WS::IO_FAIL) {
    parent_.setFail();
    return true;
  }
  readBuffer_();
  return false;
}

bool SpliceOut::IHandler::handleWHup() {
  parent_.setDone();
  if (chunked_) {
    buffer_.put("0\r\n\r\n"); // closing chunk
  }
  //delFilter(EventQueue::in);
  connection_.enableFilter(server_.getEventQueue(), EventQueue::out);
  return true;
}
bool SpliceOut::IHandler::handleError() {
  parent_.setFail();
  return true;
}
bool SpliceOut::IHandler::handleWrite() {
  abort();
}
bool SpliceOut::IHandler::handleRHup() {
  abort();
}
