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
  std::string content_type_;

 public:
  Request() = default;
  ~Request() override = default;
  Request(const Request& other);
  Request& operator=(const Request& rhs);

  bool setMessage(const std::string& msg);
  HTTP::Method getMethod() const;
  const std::string& getUri() const;
  const std::string& getContentType() const;
  void setUri(const std::string&);
  void setContentType(const std::string& str);
  const std::string getPath() const;
  const char* getHeader(const std::string& key) const;
  HttpVersion getVersion() const;
  // todo: getVersion()? Are we ?

};
