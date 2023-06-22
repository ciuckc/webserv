#include "ReadRequest.h"
#include "http/RequestHandler.h"
#include "SendResponse.h"

// =========== ReadRequest ===========
ReadRequest::ReadRequest(Request& req) : request_(req) {}
bool ReadRequest::operator()(Connection& connection) {
  ConnectionBuffer& buf = connection.getBuffer();
  if (state_ != BODY) {
    std::string line;
    while (!buf.getline(line).readFailed()) {
      if (use_line(line)) {
        if (error_ != 0)
          return true;
        break;
      }
    }
  }
  if (buf.readFailed() || state_ != BODY)
    return false;
  if (request_.getBodySize() == 0)
    if (request_.getMethod() == Request::POST)
      error_ = 411; // Length required
  // Todo: read body (or BodyReader task...)
  return true;
}

void ReadRequest::onDone(Connection& connection) {
  if (error_ != 0)
    connection.addTask(new SendResponse(ErrorResponse(error_)));
  else {
    RequestHandler rq(request_);
    rq.execRequest();
    connection.addTask(new SendResponse(rq.getResponse()));
  }
  // else do something with what we learnt from the request
  // find correct route? read the body in a special way? That sounds mildly sexual
}

// return true if we should stop
bool ReadRequest::use_line(std::string& line) {
  switch (state_) {
    case MSG:
      std::cout << "IN: " << line;
      if (!request_.setMessage(line)) {
        error_ = 400; // Bad Request, note 505 for version? also uri length etc
        return true;
      }
      state_ = HEADERS;
      break;
    case HEADERS:
      if (line == "\r\n" || line == "\n") {
        state_ = BODY;
        return true;
      } else {
        request_.addHeader(line); // HeaderVisitors
      }
      break;
    case BODY: // unreachable
      return true;
  }
  return false;
}
