#include "Response.h"

#include <sstream>

#include "Status.h"

// very big class like this i love it so far
Response::Response() = default;
Response::~Response() = default;
Response::Response(const Response &other) = default;
Response &Response::operator=(const Response &rhs) = default;

void Response::setMessage(int status) {
  message_ = "HTTP/1.1 " + std::to_string(status) + " " + http::getStatus(status) + "\r\n";
}
