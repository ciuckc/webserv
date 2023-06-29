#include "Response.h"

#include <sstream>

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
