#include "ErrorResponse.h"

#include <cstdio>

#include "Status.h"

ErrorResponse::ErrorResponse(int error) noexcept {
  setMessage(error);
  addHeader("server", "webserv");
  addHeader("content-type", "text/html");

  std::string reason = http::getStatus(error);
  size_t len = strlen(errpage_template) + (reason.length() * 2) + 3 - 6;
  {
    std::stringstream str;
    str << len;
    addHeader("content-length", str.str());
  }
  // todo: lazy initialize body when possible (while writing to socket)

  char* body = new char[len + 1];
  std::snprintf(body, len + 1, errpage_template, error, reason.c_str(), reason.c_str());
  setBody(body, len);
}

ErrorResponse::~ErrorResponse() noexcept = default;

ErrorResponse::ErrorResponse(const ErrorResponse& other) noexcept : Response(other) {}

ErrorResponse& ErrorResponse::operator=(const ErrorResponse& rhs) {
  this->Response::operator=(rhs);
  return *this;
}

const char* ErrorResponse::what() const noexcept {
  return getMessage().c_str();
}
