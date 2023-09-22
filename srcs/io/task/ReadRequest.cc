#include "ReadRequest.h"

#include <algorithm>

#include "config/ConfigServer.h"
#include "http/RequestHandler.h"

// =========== ReadRequest ===========
WS::IOStatus ReadRequest::operator()(Connection& connection) {
  RingBuffer& buf = connection.getInBuffer();
  std::string line;

  while(buf.getline(line) && error_ == 0) {
    error_ = use_line(connection, line);
    if (state_ == DONE)
      return WS::IO_GOOD;
  }
  if (buf.full() && error_ == 0)
    error_ = (state_ == HEADERS ? 431 : 414);
  if (error_ == 0)
    return WS::IO_AGAIN;
  error(connection);
  return WS::IO_FAIL;
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
  //else if (request_.getContentLength() > cfg_->getClientMaxBodySize())
  //  error_ = 413; todo: this should be in route. route here?
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
    auto path = request_.getPath();
    const auto route = cfg_->matchRoute(path);
    if (route != cfg_->getRoutes().end()) {
      return rq.execRequest(path, route->second);
    }
    error_ = 404;
    rq.handleError_(error_);
  }
}

int ReadRequest::use_line(Connection& connection, std::string& line) {
  if ((req_len_ += line.size()) > WS::request_maxlen)
    return 413;

  if (state_ == MSG)
    return handle_msg(connection, line);
  else
    return handle_header(connection, line);
}

int ReadRequest::handle_msg(Connection& connection, std::string& line) {
  Log::info(connection, "IN: \t", util::without_crlf(line), '\n');

  if (request_.setMessage(line)) {
    state_ = HEADERS;
    return 0;
  } else {
    return 400;
  }
}

int ReadRequest::handle_header(Connection& connection, std::string& line) {
  if (line.size() > WS::header_maxlen) {
    return 431;
  } else if (line == "\r\n" || line == "\n") {
    state_ = DONE;
    return 0;
  }

  Log::trace(connection, "H:  \t", util::without_crlf(line), '\n');
  auto kvpair = split_header(line);
  if (error_ == 0) {
    auto hooks = hhooks_.equal_range(kvpair.first);
    std::for_each(hooks.first, hooks.second,
                  [&](const header_lambda_map::value_type& v) {
                    int err = v.second(*this, kvpair.second, connection);
                    if (err != 0) error_ = err;
                  });
    request_.addHeader(line);
  }
  return error_;
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

void ReadRequest::error(Connection& connection) {
  if (cfg_) {
    RequestHandler rh(connection, *cfg_, request_);
    rh.handleError_(error_);
  } else {
    connection.enqueueResponse(
        std::forward<Response>(
            Response::builder().message(error_).build()));
  }
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
      if ((size_t)(pos - value.data()) != value.size())
        return 400;
      request.request_.setContentLength(content_length);
      // todo: make sure this body length is not too big onDone
      return 0;
    })}, WS::case_cmp_less};

// if-modified-since
    // transfer-encoding

