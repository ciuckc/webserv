#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "Header.h"

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
  std::vector<Header> headers_;
  char *body_;
  size_t body_size_;

  void parseStatus(std::ifstream& in);
  void parseHeaders(std::ifstream& in);

 public:
  Request();
  ~Request();
  Request(const Request& other);
  Request& operator=(const Request& rhs);

  void parse(std::ifstream& in);

  void write(std::ostream &out) const;
};

std::ostream& operator<<(std::ostream& out, const Request& req);
std::ifstream& operator>>(std::ifstream& in, Request& req);
