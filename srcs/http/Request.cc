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

static bool next_word(std::string& word, const std::string& str, size_t& end) {
  size_t start = str.find_first_not_of(" \t\r\n", end);
  if (start == std::string::npos)
    return false;
  end = str.find_first_of(" \t\r\n", start);
  word = str.substr(start, end - start);
  return true;
}

bool Request::setMessage(const std::string& msg) {
  message_ = msg;

  size_t pos = 0;
  std::string word;
  if (!next_word(word, msg, pos))
    return false;
  if (word == "GET")
    method_ = HTTP::GET;
  else if (word == "POST")
    method_ = HTTP::POST;
  else
    return false;

  if (!next_word(uri_, msg, pos))
    return false;

  if (!next_word(word, msg, pos))
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

const std::string& Request::getContentType() const {
  return content_type_;
}

void Request::setUri(const std::string& uri) {
  this->uri_ = uri;
}

void Request::setContentType(const std::string& str) {
  content_type_ = str;
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
