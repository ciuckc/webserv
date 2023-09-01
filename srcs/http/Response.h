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
  static ResponseBuilder builder();
};

class Response::ResponseBuilder {
 private:
  Response response_;
 public:
  ResponseBuilder& message(int status);
  ResponseBuilder& content_length(size_t length);
  ResponseBuilder& header(const std::string& key, const std::string& val);
  ResponseBuilder& header(const std::string& header_str);
  Response&& build();
};