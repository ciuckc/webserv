#include "Response.h"

#include <fstream>
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

void Response::makeBody(const char* type, const std::string& path)
{
  std::ios_base::openmode openmode = std::ios::in;
  if (type) {
    std::string type_str(type);
    if (type_str.find("text") != std::string::npos)
      openmode |= std::ios::binary;
  }
  std::ifstream file(path, openmode);
  if (!file.is_open()) {
    throw (ErrorResponse(500));
  }
  std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();
  size_t body_size = str.length();
  char* body;
  try {
    body = new char[body_size + 1];
  }
  catch (std::exception&) {
    throw (ErrorResponse(500));
  }
  str.copy(body, body_size);
  body[body_size] = '\0';
  this->setBody(body, body_size);
}
