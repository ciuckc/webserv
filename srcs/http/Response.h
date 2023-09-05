#pragma once
#include <ostream>
#include <string>

#include "Message.h"
#include "io/Socket.h"

class Response : public Message {
 public:
  Response();
  ~Response() override;
  Response(const Response& other);
  Response& operator=(const Response& rhs);

  void setMessage(int status);
  void setKeepAlive(uint32_t timeout, uint32_t max_requests);

  class ResponseBuilder;
  static inline ResponseBuilder builder();
};

class Response::ResponseBuilder {
 private:
  Response response_;
 public:
  inline ResponseBuilder& message(int status);
  inline ResponseBuilder& content_length(size_t length);
  inline ResponseBuilder& header(const std::string& key, const std::string& val);
  inline ResponseBuilder& header(const std::string& header_str);
  inline Response&& build();
};

inline Response::ResponseBuilder Response::builder() {
  return {};
}

inline Response::ResponseBuilder& Response::ResponseBuilder::message(int status) {
  response_.setMessage(status);
  return *this;
}

inline Response::ResponseBuilder& Response::ResponseBuilder::content_length(size_t length) {
  response_.setContentLength(length);
  return header("Content-Length", std::to_string(length));
}

inline Response::ResponseBuilder& Response::ResponseBuilder::header(const std::string& key, const std::string& val) {
  response_.addHeader(key, val);
  return *this;
}

inline Response::ResponseBuilder& Response::ResponseBuilder::header(const std::string& header_str) {
  response_.addHeader(header_str);
  return *this;
}

inline Response&& Response::ResponseBuilder::build() {
  return std::move(response_);
}
