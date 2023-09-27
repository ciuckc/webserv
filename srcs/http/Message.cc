#include "Message.h"
#include "util/String.h"

#include <ostream>

const std::string& Message::getMessage() const {
  return message_;
}

const std::vector<std::string>& Message::getHeaders() const {
  return headers_;
}

void Message::addHeader(const std::string& kv_pair) {
  headers_.push_back(kv_pair);
}

size_t Message::getContentLength() const {
  return content_length_;
}

void Message::setContentLength(size_t content_length) {
  content_length_ = content_length;
}

std::ostream& Message::write(std::ostream& out) const {
  out << message_;
  for (const auto& header : headers_)
    out << header;
  out << "\r\n";
  return out;
}

std::ostream& operator<<(std::ostream& out, const Message& msg) {
  msg.write(out);
  return out;
}
