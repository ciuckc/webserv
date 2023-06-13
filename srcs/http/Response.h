#pragma once
#include <ostream>
#include <string>

#include "Headers.h"
#include "io/Socket.h"

class Response {
 private:
  std::string message_;
  Headers headers_;

  // todo: some kinda fstream that's triggered by event?
  char* body_;
  size_t body_size_;

 public:
  Response();
  ~Response();
  Response(const Response& other);
  Response& operator=(const Response& rhs);

  void setMessage(int status);
  const std::string& getMessage() const;
  void addHeader(const std::string& key, const std::string& val);
  void setBody(char* body, size_t body_size);

  void write(std::ostream& out) const;
  void write(Socket& socket) const;
};

std::ostream& operator<<(std::ostream& out, const Response& res);
