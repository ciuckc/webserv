#include "SendResponse.h"

SendResponse::SendResponse(Response&& response)
  : response_(std::forward<Response>(response)),
    header_(response_.getHeaders().begin()) {}

bool SendResponse::operator()(Connection& connection) {
  while (!connection.getBuffer().needWrite()) {
    switch (state_) {
      case MSG:
        Log::info(connection, "OUT:\t", util::without_crlf(response_.getMessage()), '\n');
        connection.getBuffer() << response_.getMessage();
        state_ = HEADERS;
        break;
      case HEADERS:
        Log::trace(connection, "H:\t\t", util::without_crlf(*header_), '\n');
        connection.getBuffer() << *header_++;
        if (header_ == response_.getHeaders().end())
          state_ = SEPARATOR;
        break;
      case SEPARATOR:
        connection.getBuffer() << "\r\n";
        return true;
    }
  }
  return false;
}

void SendResponse::onDone(Connection& connection) {
  Log::trace(connection, "Completed response\n");
}
