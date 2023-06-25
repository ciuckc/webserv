#include "SendResponse.h"

SendResponse::SendResponse(const Response& response)
    : response_(response), header_(response_.getHeaders().begin()) {}

bool SendResponse::operator()(Connection& connection) {
  while (!connection.getBuffer().needWrite()) {
    switch (state_) {
      case MSG:
        Log::info("OUT: ", response_.getMessage());
        connection.getBuffer() << response_.getMessage();
        state_ = HEADERS;
        break;
      case HEADERS:
        connection.getBuffer() << *header_++;
        if (header_ == response_.getHeaders().end())
          state_ = SEPARATOR;
        break;
      case SEPARATOR:
        connection.getBuffer() << "\r\n";
        if (response_.getBodySize() == 0)
          return true;
        state_ = BODY;
        break;
      case BODY:
        connection.getBuffer() << response_.getBody();
        return true;
    }
  }
  return false;
}

void SendResponse::onDone(Connection& connection) {
  // todo: Add task to read next request!
  connection.close();
}
