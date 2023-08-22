#include "ReadRequest.h"
#include "http/RequestHandler.h"
#include "SendResponse.h"
#include "http/ErrorResponse.h"
#include <algorithm>

// =========== ReadRequest ===========
ReadRequest::ReadRequest(Request& req) : request_(req) {}
bool ReadRequest::operator()(Connection& connection) {
  ConnectionBuffer& buf = connection.getBuffer();
  if (state_ != BODY) {
    std::string line;
    while (!buf.getline(line).readFailed()) {
      if (use_line(connection, line)) {
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
    connection.enqueueResponse(ErrorResponse(error_));
  else {
    RequestHandler rq(request_);
    rq.execRequest();
    connection.enqueueResponse(rq.getResponse());
  }
  // else do something with what we learnt from the request
  // find correct route? read the body in a special way? That sounds mildly sexual
  if (connection.keepAlive())
    connection.awaitRequest();
}

// return true if we should stop
bool ReadRequest::use_line(Connection& connection, std::string& line) {
  if ((req_len_ += line.size()) > WS::request_maxlen) {
    error_ = 413;
    return true;
  }

  switch (state_) {
    case MSG:
      return handle_msg(connection, line);
    case HEADERS:
      return handle_header(connection, line);
    case BODY:
      return true;
  }
  __builtin_unreachable();
}

bool ReadRequest::handle_msg(Connection& connection, std::string& line) {
  Log::info('[', connection.getSocket().get_fd(), "]\tIN:\t", line);

  if (!request_.setMessage(line)) {
    if (request_.getMethod() == Request::INVALID) {
      error_ = 405; // Method not allowed
    } else if (request_.getUri().empty()) {
      error_ = 400; // Bad Request
    }/* else if (request_.getVersion() != Request::VER_1_1) {
      error_ = 505; // Http version not supported
    } // We could have 414 URI too long here as well*/
    return true;
  }
  state_ = HEADERS;
  return false;
}



bool ReadRequest::handle_header(Connection& connection, std::string& line) {
  Log::trace('[', connection.getSocket().get_fd(), "]\tH:\t", line);
  if (line.size() > WS::header_maxlen) {
    error_ = 431;
    return true;
  } else if (line == "\r\n" || line == "\n") {
    state_ = BODY;
    return true;
  }

  auto kvpair = split_header(line);
  if (error_ != 0)
    return true;
  auto hooks = hhooks_.equal_range(kvpair.first);
  std::for_each(hooks.first, hooks.second,
                [&](const header_lambda_map::value_type& v) {
                  int err = v.second(*this, kvpair.second, connection);
                  if (err != 0) error_ = err;
                });
  request_.addHeader(line);
  return error_ != 0;
}

std::pair<std::string, std::string> ReadRequest::split_header(std::string& line) {
  const size_t sep = line.find_first_of(':');
  const size_t val_end = line.find_last_not_of(" \t\r\n");

  if (sep == std::string::npos) {
    size_t val_start = line.find_first_not_of(" \t");
    if (val_start != 0 && val_start <= val_end) // Continuation needs ws in front
      return std::make_pair(prev_key_, line.substr(val_start, val_end - val_start + 1));
  } else {
    size_t key_start = line.find_first_not_of(" \t");
    size_t key_end = line.find_last_not_of(" \t", sep - 1);
    size_t val_start = line.find_first_not_of(" \t", sep + 1);
    if (key_start <= key_end && val_start <= val_end) {
      std::transform(&line[key_start], &line[key_end + 1], &line[key_start], ::tolower);
      return std::make_pair(line.substr(key_start, key_end - key_start + 1),
                            line.substr(val_start, val_end - val_start + 1));
    }
  }
  error_ = 400;
  return {};
}

#define HEADER_HOOK(name, lambda) \
{ name, [](ReadRequest& request, const std::string& value, Connection& connection)->int lambda }
const ReadRequest::header_lambda_map ReadRequest::hhooks_ = {
    HEADER_HOOK("connection", {
      (void) request;
      if (strncasecmp(value.c_str(), "close", strlen("close")) == 0)
        connection.setKeepAlive(false);
      return 0;
    }),
};

