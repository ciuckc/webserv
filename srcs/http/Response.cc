#include "Response.h"

#include "Status.h"
#include "util/WebServ.h"

// very big class like this i love it so far
Response::Response() {
  addHeader(WS::get_date_header());
}
Response::~Response() = default;
Response::Response(const Response &other) = default;
Response &Response::operator=(const Response &rhs) = default;

void Response::setMessage(int status) {
  message_ = "HTTP/1.1 " + std::to_string(status) + " " + http::getStatus(status) + "\r\n";
}

void Response::setKeepAlive(uint32_t timeout, uint32_t max_requests = 0) {
  std::string val = "timeout=" + std::to_string(timeout);
  if (max_requests != 0) {
    val += ", max=" + std::to_string(max_requests);
  }
  addHeader("keep-alive", val);
}

Response::ResponseBuilder Response::builder() {
  return {};
}

Response::ResponseBuilder& Response::ResponseBuilder::message(int status) {
  response_.setMessage(status);
  return *this;
}

Response::ResponseBuilder& Response::ResponseBuilder::content_length(size_t length) {
  response_.setContentLength(length);
  return header("Content-Length", std::to_string(length));
}

Response::ResponseBuilder& Response::ResponseBuilder::header(const std::string& key, const std::string& val) {
  response_.addHeader(key, val);
  return *this;
}

Response::ResponseBuilder& Response::ResponseBuilder::header(const std::string& header_str) {
  response_.addHeader(header_str);
  return *this;
}

Response&& Response::ResponseBuilder::build() {
  return std::move(response_);
}
