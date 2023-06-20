#include "IOTask.h"

// =========== SendResponse ===========
SendResponse::SendResponse(const Response& response)
    : response_(response), header_(response_.getHeaders().begin()) {}

bool SendResponse::operator()(Connection& connection) {
  while (!connection.getBuffer().needWrite()) {
    switch (state) {
      case MSG:
        connection.getBuffer() << response_.getMessage();
        state = HEADERS;
        break;
      case HEADERS:
        connection.getBuffer() << *header_++;
        if (header_ == response_.getHeaders().end())
          state = SEPARATOR;
        break;
      case SEPARATOR:
        connection.getBuffer() << "\r\n";
        if (response_.getBodySize() == 0)
          return true;
        state = BODY;
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

// =========== ReadRequest ===========
bool ReadRequest::operator()(Connection& connection) {
  ConnectionBuffer& buf = connection.getBuffer();
  std::string line;
  while (!buf.getline(line).readFailed()) {
    if (line == "\r\n" || line == "\n")
      return true;
    std::cout << line;
  }
  return false;
}
void ReadRequest::onDone(Connection& connection) {
  connection.addTask(new SendResponse(ErrorResponse(404)));
}
