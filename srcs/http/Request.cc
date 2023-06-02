#include "Request.h"
#include <cstring>
#include <string>
#include <cstdlib>

Request::Request()
    : method_(),
      uri_(),
      ver_(),
      headers_(),
      body_(),
      body_size_() {}

Request::~Request() {
  delete[] body_;
}

Request::Request(const Request& other)
    : body_() {
  *this = other;
}

Request& Request::operator=(const Request &rhs) {
  if (this == &rhs)
    return *this;
  method_ = rhs.method_;
  uri_ = rhs.uri_;
  ver_ = rhs.ver_;
  headers_ = std::vector<Header>(rhs.headers_.begin(), rhs.headers_.end());
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

void Request::parse(std::ifstream& in) {
  parseStatus(in);
  parseHeaders(in);

  for (std::vector<Header>::iterator i = headers_.begin(); i < headers_.end(); ++i) {
    if (i->key_ == "Content-Length") {
      char *end;
      body_size_ = std::strtol(i->values_.front().c_str(), &end, 10);
      if (*end != '\0')
        throw std::exception(); // invalid content-length?
      break;
    }
  }

  if (body_size_ != 0) {
    delete[] body_;
    body_ = new char[body_size_];
    if (!in.get(body_, static_cast<std::streamsize>(body_size_ + 1)))
      throw std::exception(); // todo: another error
  }
}

static Request::Method parseMethod(std::string method_str) {
  for (size_t i = 0; i < method_str.length(); i++)
    method_str[i] = static_cast<char>(std::toupper(method_str[i]));

  if (method_str == "GET")
    return Request::GET;
  if (method_str == "POST")
    return Request::POST;

  throw std::exception(); // Right? can't do much with wrong method
}

void Request::parseStatus(std::ifstream &in) {
  std::string line;
  if (!std::getline(in, line))
    throw std::exception(); // todo

  size_t sep_1 = line.find(' ');
  if (sep_1 == std::string::npos)
    throw std::exception();
  method_ = parseMethod(line.substr(0, sep_1));

  size_t sep_2 = line.find(' ', sep_1 + 1);
  if (sep_2 == std::string::npos)
    throw std::exception();
  uri_ = line.substr(sep_1 + 1, sep_2 - sep_1 - 1);

  size_t ver_len = line.length() - sep_2 - 1;
  if (line[line.length() - 1] == '\r')
    --ver_len;
  if (ver_len < 1)
    throw std::exception();
  ver_ = line.substr(sep_2 + 1, ver_len);
}

void Request::parseHeaders(std::ifstream &in) {
  std::string line;
  typedef std::vector<Header>::iterator header_iterator;
  header_iterator header = headers_.end();

  while (std::getline(in, line)) {
    if (line.empty() || line == "\r")
      break;

    size_t colon = line.find(':');
    bool continuation = colon == std::string::npos;
    if (!continuation) {
      header = headers_.end();
      std::string key = line.substr(0, colon);

      for (header_iterator i = headers_.begin(); i != headers_.end(); ++i)
        if (i->key_ == key)
          header = i;

      if (header == headers_.end())
        header = headers_.insert(header, Header(key));

    } else if (header == headers_.end()) {
      // It was a continuation, but on the first one
      throw std::exception();
    }

    size_t val_start = continuation ? 0 : colon;
    size_t val_end;
    do {
      val_start = line.find_first_not_of(' ', val_start + 1);
      val_end = line.find_first_of(",\r");

      size_t len = (val_end == std::string::npos) ? val_end : val_end - val_start;
      header->values_.push_back(line.substr(val_start, len));
      val_start = val_end;
    } while (val_end < line.length() - 1);
  }
}

void Request::write(std::ostream& out) const {
  const char *methods[3] = {
      "UNKNOWN",
      "GET",
      "POST"
  };

  out << methods[method_] << ' ' << uri_ << ' ' << ver_ << "\r\n";
  for (std::vector<Header>::const_iterator i = headers_.begin(); i < headers_.end(); ++i) {
    out << i->key_ << ':';
    for (std::vector<std::string>::const_iterator j = i->values_.begin(); j < i->values_.end(); ++j) {
      out << ' ' << *j;
      if (j != i->values_.end() - 1)
        out << ' ';
    }
    out << "\r\n";
  }
  out << "\r\n";
  if (body_ != NULL)
    out.write(body_, static_cast<std::streamsize>(body_size_));
}

std::ostream& operator<<(std::ostream& out, const Request& req) {
  req.write(out);
  return out;
}

std::ifstream& operator>>(std::ifstream& in, Request& req) {
  req.parse(in);
  return in;
}
