#include "SimpleBody.h"
#include "io/Connection.h"

SimpleBody::SimpleBody(std::unique_ptr<char[]> &&data, size_t len)
    : data_(std::forward<std::unique_ptr<char[]>>(data)), len_(len), ofs_() {}

WS::IOStatus SimpleBody::operator()(Connection &connection) {
  auto& buf = connection.getOutBuffer();
  size_t to_write = std::min(len_ - ofs_, buf.freeLen());
  buf.put(std::string_view(data_.get() + ofs_, to_write));
  ofs_ += to_write;
  return ofs_ == len_ ? WS::IO_GOOD : WS::IO_AGAIN;
}

void SimpleBody::onDone(Connection &connection) {
  Log::trace(connection, "Sent simple body\n");
}
