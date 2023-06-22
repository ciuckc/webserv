#pragma once
#include <istream>
#include <string>

#include "Headers.h"
#include "Message.h"

class Request : public Message {
 public:
  enum Method {
    INVALID = 0,
    GET = 1,
    POST = 2,
  };

 private:
  Method method_ = INVALID;
  std::string uri_;

 public:
  Request() = default;
  ~Request() override = default;
  Request(const Request& other);
  Request& operator=(const Request& rhs);

  bool setMessage(const std::string& msg);
  Method getMethod() const;
  const std::string& getUri() const;
  const std::string getPath() const;
  const std::string* getHeader(const std::string& key);
  // todo: getVersion()? Are we ?

};
