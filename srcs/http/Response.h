#pragma once
#include <ostream>
#include <string>

#include "Headers.h"

class Response {
 private:
  std::string ver_;
  std::string code_;
  std::string reason_;

  Headers headers_;

  char* body_;
  size_t body_size_;

 public:
  Response();
  ~Response();
  Response(const Response& other);
  Response& operator=(const Response& rhs);

  void addHeader(const std::string& key, const std::string& val);

  void write(std::ostream& out) const;
};

std::ostream& operator<<(std::ostream& out, const Response& res);
