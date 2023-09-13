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

static bool next_word(std::string_view& view, std::string_view& word) {
  size_t start = view.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
    return false;
  view.remove_prefix(start);
  size_t len = view.find_first_of(" \t\r\n");
  word = view.substr(0, len);
  view.remove_prefix(len);
  return true;
}

bool Request::setMessage(const std::string& msg) {
  message_ = msg;
  std::string_view view(msg);

  std::string_view word;
  if (!next_word(view, word))
    return false;
  if (word == "GET")
    method_ = HTTP::GET;
  else if (word == "POST")
    method_ = HTTP::POST;
  else if (word == "DELETE")
    method_ = HTTP::DELETE;

  if (!next_word(view, word))
    return false;
  uri_ = word;

  if (!next_word(view, word))
    return false;
  if (word != "HTTP/1.1")
    version_ = HttpVersion::VER_INVALID;
  else
    version_ = HttpVersion::VER_1_1;
  return true;
}

HTTP::Method Request::getMethod() const {
  return method_;
}

const std::string& Request::getUri() const {
  return uri_;
}

void Request::setUri(const std::string& uri) {
  this->uri_ = uri;
}

const std::string Request::getPath() const {
  return (uri_.substr(uri_.find('/'), uri_.find('?')));
}

const char* Request::getHeader(const std::string& key) const {
  for (const auto& elem : headers_) {
    if (elem.find(key) != std::string::npos) {
      return (elem.c_str());
    }
  }
  return nullptr;
}
Request::HttpVersion Request::getVersion() const {
  return version_;
}
