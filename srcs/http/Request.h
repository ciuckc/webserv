#pragma once
#include <istream>
#include <string>

#include "Headers.h"

class Request {
 public:
  enum Method {
    INVALID = 0,
    GET = 1,
    POST = 2,
  };

 private:
  Method method_;
  std::string uri_;
  std::string ver_;

  Headers headers_;

  char* body_;
  size_t body_size_;

  void parseStatus(std::istream& in);

 public:
  Request();
  ~Request();
  Request(const Request& other);
  Request& operator=(const Request& rhs);

  void parse(std::istream& in);
  void write(std::ostream& out) const;
  Method GetMethod() const;
};

std::ostream& operator<<(std::ostream& out, const Request& req);
std::istream& operator>>(std::istream& in, Request& req);
