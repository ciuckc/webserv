#include "Request.h"

#include <cstdlib>
#include <cstring>

Request::Request() : method_(), body_(), body_size_() {}

Request::~Request() {
  delete[] body_;
}

Request::Request(const Request& other) : body_() {
  *this = other;
}

Request& Request::operator=(const Request& rhs) {
  if (this == &rhs)
    return *this;
  method_ = rhs.method_;
  uri_ = rhs.uri_;
  ver_ = rhs.ver_;
  headers_ = rhs.headers_;
  body_size_ = rhs.body_size_;
  delete[] body_;
  if (rhs.body_ == NULL) {
    body_ = NULL;
  } else {
    body_ = new char[body_size_];
    std::memcpy(body_, rhs.body_, body_size_);
  }
  return *this;
}

void Request::parse(std::istream& in) {
  parseStatus(in);
  headers_.parse(in);

  {
    Headers::iterator cl_idx = headers_.find("content-length");
    if (cl_idx != headers_.end()) {
      char* end;
      body_size_ = std::strtol(cl_idx->values_.front().c_str(), &end, 10);
      if (*end != '\0')
        throw std::exception();  // invalid content-length?
    }
  }

  if (body_size_ != 0) {
    delete[] body_;
    body_ = new char[body_size_];
    if (!in.get(body_, static_cast<std::streamsize>(body_size_ + 1)))
      throw std::exception();  // todo: another error
  }
}

static Request::Method parseMethod(std::string method_str) {
  for (size_t i = 0; i < method_str.length(); i++)
    method_str[i] = static_cast<char>(std::toupper(method_str[i]));

  if (method_str == "GET")
    return Request::GET;
  if (method_str == "POST")
    return Request::POST;

  throw std::exception();  // Right? can't do much with wrong method
}

void Request::parseStatus(std::istream& in) {
  std::string line;
  if (!std::getline(in, line))
    throw std::exception();  // todo

  size_t start = 0;
  size_t end = line.find(' ');
  if (end == std::string::npos)
    throw std::exception();
  method_ = parseMethod(line.substr(start, end - start));

  start = end + 1;
  end = line.find(' ', start);
  if (end == std::string::npos)
    throw std::exception();
  uri_ = line.substr(start, end - start);

  start = end + 1;
  end = line.find('\r', start);
  if (start >= line.length())
    throw std::exception();
  ver_ = line.substr(start, end - start);
}

void Request::write(std::ostream& out) const {
  const char* methods[3] = {"UNKNOWN", "GET", "POST"};

  out << methods[method_] << ' ' << uri_ << ' ' << ver_ << "\r\n";
  out << headers_;
  if (body_ != NULL)
    out.write(body_, static_cast<std::streamsize>(body_size_));
}

Request::Method Request::GetMethod() const {
  return (this->method_);
}

std::ostream& operator<<(std::ostream& out, const Request& req) {
  req.write(out);
  return out;
}

std::istream& operator>>(std::istream& in, Request& req) {
  req.parse(in);
  return in;
}
