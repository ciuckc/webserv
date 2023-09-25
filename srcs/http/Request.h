#pragma once
#include <istream>
#include <string>

#include "Message.h"
#include "Method.h"

class Request : public Message {
 public:
  enum HttpVersion {
    VER_INVALID,
    VER_1_1
  };

 private:
  HTTP::Method method_ = HTTP::INVALID;
  std::string uri_;
  HttpVersion version_ = VER_INVALID;

 public:
  Request() = default;
  ~Request() override = default;
  Request(const Request& other) = default;
  Request& operator=(const Request& rhs) = default;

  bool setMessage(const std::string& msg);
  void setMethod(HTTP::Method method);
  void setVersion(HttpVersion version);
  HTTP::Method getMethod() const;
  const std::string& getUri() const;
  void setUri(const std::string&);
  std::string getPath() const;
  const char* getHeader(const std::string& key) const;
  HttpVersion getVersion() const;
};
