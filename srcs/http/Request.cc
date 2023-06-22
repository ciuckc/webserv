#include "Request.h"

Request::Request(const Request& other) : Message(other) {
  method_ = other.method_;
  uri_ = other.uri_;
}

Request& Request::operator=(const Request& rhs) {
  if (this == &rhs)
    return *this;
  Message::operator=(rhs);
  method_ = rhs.method_;
  uri_ = rhs.uri_;
  return *this;
}

bool Request::setMessage(const std::string& msg) {
  message_ = msg;

  size_t ws_idx = msg.find_first_of(" \t");
  if (ws_idx == std::string::npos)
    return false;
  if (msg.compare(0, ws_idx, "GET") == 0) {
    method_ = GET;
  } else if (msg.compare(0, ws_idx, "POST") == 0) {
    method_ = POST;
  } else {
    return false; // 400 bad request
  }

  size_t val_start = msg.find_first_not_of(" \t", ws_idx);
  if (val_start == std::string::npos)
    return false;
  ws_idx = msg.find_first_of(" \t", val_start);
  uri_ = msg.substr(val_start, ws_idx - val_start);

  // todo: parse version?
  /*val_start = msg.find_first_not_of(" \t", ws_idx);
  if (val_start == std::string::npos)
    return false;
  ws_idx = msg.find_first_of(" \t\r\n");*/
  return true;
}

Request::Method Request::getMethod() const {
  return method_;
}

const std::string& Request::getUri() const {
  return uri_;
}

const std::string Request::getPath() const {
  return (uri_.substr(uri_.find('/'), uri_.find('?')));
}

const std::string* Request::getHeader(const std::string& key) {
  for (const auto& elem : headers_) {
    if (elem.find(key) != std::string::npos) {
      return (&elem);
    }
  }
  return (NULL);
}
