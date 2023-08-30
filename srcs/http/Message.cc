#include "Message.h"

#include <cstring>
#include <ostream>

Message::Message() : body_(), content_length_() {}

Message::~Message() {
  delete[] body_;
}

Message::Message(const Message& other) : body_(), content_length_() {
  *this = other;
}

Message& Message::operator=(const Message& rhs) {
  if (this == &rhs)
    return *this;
  message_ = rhs.message_;
  headers_ = rhs.headers_;
  content_length_ = rhs.content_length_;
  delete[] body_;
  body_ = (rhs.body_ == nullptr) ? nullptr : new char[content_length_];
  if (body_)
    std::memcpy(body_, rhs.body_, content_length_);
  return *this;
}

const std::string& Message::getMessage() const {
  return message_;
}

const std::vector<std::string>& Message::getHeaders() const {
  return headers_;
}

void Message::addHeader(const std::string& kv_pair) {
  headers_.push_back(kv_pair);
}

void Message::addHeader(const std::string& key, const std::string& val) {
  headers_.push_back(key + ": " + val + "\r\n");
}

std::string Message::getBody() const {
  return {body_, content_length_};
}

size_t Message::getContentLength() const {
  return content_length_;
}

void Message::setContentLength(size_t content_length) {
  content_length_ = content_length;
}

void Message::setBody(char* body, size_t body_size) {
  delete[] body_;
  body_ = body;
  content_length_ = body_size;
}

std::ostream& Message::write(std::ostream& out) const {
  out << message_;
  for (const auto& header : headers_)
    out << header;
  out << "\r\n";
  if (body_)
    out.write(body_, std::streamsize(content_length_));
  return out;
}

std::ostream& operator<<(std::ostream& out, const Message& msg) {
  msg.write(out);
  return out;
}
