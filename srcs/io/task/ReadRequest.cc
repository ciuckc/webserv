#include "ReadRequest.h"
#include "http/RequestHandler.h"
#include <algorithm>

// =========== ReadRequest ===========
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
  if (request_.getContentLength() == 0)
    if (request_.getMethod() == HTTP::POST)
      error_ = 411; // Length required
  // Todo: read body (or BodyReader task...)
  return true;
}

void ReadRequest::onDone(Connection& connection) {
  if (cfg_ == nullptr && error_ == 0)
    error_ = 400; // No host header..
  RequestHandler rq(connection, *cfg_, request_);
  if (error_ == 0)
    rq.execRequest();
  else
    rq.handleError_(error_);
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
      return handle_body(connection, line);
    case DONE:
      return true;
  }
  __builtin_unreachable();
}

bool ReadRequest::handle_msg(Connection& connection, std::string& line) {
  Log::info('[', connection.getSocket().get_fd(), "]\tIN:\t", std::string_view(line.data(), line.find_last_not_of("\n\r") + 1), '\n');

  if (!request_.setMessage(line)) {
    if (request_.getMethod() == HTTP::INVALID) {
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
  if (line.size() > WS::header_maxlen) {
    error_ = 431;
    return true;
  } else if (line == "\r\n" || line == "\n") {
    state_ = BODY;
    return true;
  }

  auto kvpair = split_header(line);
  Log::trace('[', connection.getSocket().get_fd(), "]\tH:\t", kvpair.first, ": ", kvpair.second, '\n');
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

bool ReadRequest::handle_body(Connection& connection, std::string& line) {
  (void) connection;
  (void) line;
  state_ = DONE;
  return true;
}

std::pair<std::string, std::string_view> ReadRequest::split_header(std::string& line) {
  const size_t sep = line.find_first_of(':');
  const size_t val_end = line.find_last_not_of(" \t\r\n");
  // this will always work (std::string::npos + 1 == 0)
  const size_t val_start = line.find_first_not_of(" \t", sep + 1);

  if (val_start > val_end) {
    error_ = 400;
    return {};
  }
  if (sep != std::string::npos) {
    size_t key_start = line.find_first_not_of(" \t");
    size_t key_end = line.find_last_not_of(" \t", sep - 1);
    if (key_start > key_end) {
      error_ = 400;
      return {};
    }
    header_key_ = line.substr(key_start, key_end - key_start + 1);
  } else if (val_start == 0) {
    error_ = 400;
    return {};
  }
  return {header_key_, {&line[val_start], val_end - val_start + 1}};
}

#define HEADER_HOOK(name, lambda) \
{ name, [](ReadRequest& request, const std::string_view& value, Connection& connection)->int lambda }
const ReadRequest::header_lambda_map ReadRequest::hhooks_ = {{
    HEADER_HOOK("connection", {
      (void) request;
      if (strncasecmp(value.data(), "close", strlen("close")) == 0)
        connection.setKeepAlive(false);
      return 0;
    }),
    HEADER_HOOK("host", {
      if (request.cfg_ != nullptr)
        return 400; // duplicate host header

      std::string hostname { value.substr(0, value.find(':')) };
      auto& host_map = connection.getHostMap();
      auto found_cfg = host_map.find(hostname);
      if (found_cfg == host_map.end()) {
        found_cfg = host_map.find("");
        if (found_cfg == host_map.end())
          return 400;
      }
      request.cfg_ = &found_cfg->second;
      return 0;
    }),
    HEADER_HOOK("content-length", {
      (void)connection;
      char *pos;
      size_t content_length = std::strtoul(value.data(), &pos, 10);
      if (value.find_first_not_of(" \t\r\n", pos - value.data()) != std::string::npos)
        return 400;
      request.request_.setContentLength(content_length);
      // todo: make sure this body length is not too big onDone
      return 0;
    }),
    HEADER_HOOK("content-type", {
      (void)connection;
      request.request_.setContentType(std::string(value));
      return 0;
    })}, WS::case_cmp_less};

    // if-modified-since
    // transfer-encoding

