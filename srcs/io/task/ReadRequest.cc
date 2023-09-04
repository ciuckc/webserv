#include "ReadRequest.h"
#include "http/RequestHandler.h"
#include <algorithm>

// =========== ReadRequest ===========
bool ReadRequest::operator()(Connection& connection) {
  ConnectionBuffer& buf = connection.getBuffer();
  std::string line;
  while (!buf.getline(line).readFailed())
    if (use_line(connection, line))
      return true;
  return false;
}

bool ReadRequest::checkError(Connection& connection) {
  if (error_ != 0) {
    // because we errored halfway through reading request we
    // need to reset the connection unless we want to find the next request
    // by digging through invalid request
    connection.setKeepAlive(false);
    return true;
  }
  if (cfg_ == nullptr)
    error_ = 400;
  else if (request_.getPath().find("..") != std::string::npos)
    error_ = 403; // don't escape root, forbidden
  else if (request_.getMethod() == HTTP::INVALID)
    error_ = 405; // Method not allowed
  else if (request_.getMethod() == HTTP::POST && request_.getContentLength() == 0)
    error_ = 411;
  else if (request_.getUri().size() > WS::uri_maxlen)
    error_ = 414;
  else if (request_.getVersion() != Request::VER_1_1)
    error_ = 505; // Http version not supported
  else
   return false;
  return true;
}

void ReadRequest::onDone(Connection& connection) {
  RequestHandler rq(connection, *cfg_, request_);
  if (!checkError(connection)) {
    const auto route = cfg_->getRoutes().lower_bound(request_.getPath());
    if (route != cfg_->getRoutes().end())
      return rq.execRequest(route->second);
    else
      error_ = 404;
  }
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
    case DONE:
      return true;
  }
  __builtin_unreachable();
}

bool ReadRequest::handle_msg(Connection& connection, std::string& line) {
  Log::info(connection, "IN: \t", std::string_view(line.data(), line.find_last_not_of("\n\r") + 1), '\n');

  if (request_.setMessage(line)) {
    state_ = HEADERS;
    return false;
  } else {
    error_ = 400;
    return true;
  }
}

bool ReadRequest::handle_header(Connection& connection, std::string& line) {
  if (line.size() > WS::header_maxlen) {
    error_ = 431;
    return true;
  } else if (line == "\r\n" || line == "\n") {
    return true;
  }

  auto kvpair = split_header(line);
  Log::trace(connection, "H:\t\t", kvpair.first, ": ", kvpair.second, '\n');
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
      if (*pos)
        return 400;
      request.request_.setContentLength(content_length);
      // todo: make sure this body length is not too big onDone
      return 0;
    })}, WS::case_cmp_less};

    // if-modified-since
    // transfer-encoding

