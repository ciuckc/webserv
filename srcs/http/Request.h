#pragma once
#include <istream>
#include <string>

#include "Message.h"

class Request : public Message {
 public:
  enum Method {
    INVALID = 0,
    GET = 1,
    POST = 2,
  };
  enum HttpVersion {
    VER_INVALID,
    VER_1_1
  };

 private:
  Method method_ = INVALID;
  std::string uri_;
  HttpVersion version_ = VER_INVALID;

 public:
  Request() = default;
  ~Request() override = default;
  Request(const Request& other);
  Request& operator=(const Request& rhs);

  bool setMessage(const std::string& msg);
  Method getMethod() const;
  const std::string& getUri() const;
  void setUri(const std::string&);
  const std::string getPath() const;
  const char* getHeader(const std::string& key);
  HttpVersion getVersion() const;
  // todo: getVersion()? Are we ?

};
