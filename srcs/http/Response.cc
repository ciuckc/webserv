#include "Response.h"

#include <cstring>
#include <sstream>

#include "Status.h"

Response::Response() : body_(), body_size_() {}

Response::~Response() {}

Response::Response(const Response &other) : body_(), body_size_() { *this = other; }

Response &Response::operator=(const Response &rhs) {
  if (this == &rhs) return *this;
  message_ = rhs.message_;
  headers_ = rhs.headers_;
  body_size_ = rhs.body_size_;
  delete[] body_;
  body_ = (rhs.body_ == NULL) ? NULL : new char[body_size_];
  if (body_) std::memcpy(body_, rhs.body_, body_size_);
  return *this;
}

void Response::setMessage(int status) {
  std::stringstream str;
  str << "HTTP/1.1 " << status << " " << http::getStatus(status) << "\r\n";
  message_ = str.str();
}

const std::string &Response::getMessage() const { return message_; }

void Response::addHeader(const std::string &key, const std::string &val) {
  Headers::iterator header = headers_.find_or_create(key);
  header->values_.push_back(val);
}

void Response::setBody(char *body, size_t body_size) {
  delete[] body_;
  body_ = body;
  body_size_ = body_size;
}

void Response::write(std::ostream &out) const {
  out << message_;
  out << headers_;
  if (body_) out.write(body_, body_size_);
}

std::ostream &operator<<(std::ostream &out, const Response &res) {
  res.write(out);
  return out;
}
