#include "SendResponse.h"

#include "io/Connection.h"
#include "util/Log.h"

SendResponse::SendResponse(Response&& response)
  : response_(std::forward<Response>(response)), header_(response_.getHeaders().begin()) {}

WS::IOStatus SendResponse::operator()(Connection& connection) {
  RingBuffer& buffer = connection.getOutBuffer();
  while (!buffer.full()) {
    switch (state_) {
      case MSG:
        Log::info(connection, "OUT:\t", util::without_crlf(response_.getMessage()), '\n');
        buffer.put(response_.getMessage());
        state_ = HEADERS;
        break;
      case HEADERS:
        Log::trace(connection, "H:\t\t", util::without_crlf(*header_), '\n');
        buffer.put(*header_++);
        if (header_ == response_.getHeaders().end())
          state_ = SEPARATOR;
        break;
      case SEPARATOR:
        buffer.put("\r\n");
        return WS::IO_GOOD;
    }
  }
  return WS::IO_AGAIN;
}

void SendResponse::onDone(Connection& connection) {
  Log::trace(connection, "Completed response\n");
}
