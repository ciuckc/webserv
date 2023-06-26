#pragma once
#include <exception>

#include "Response.h"

class ErrorResponse : public Response, public std::exception {
 private:
  static constexpr const char* errpage_template =       "<!DOCTYPE html>"
                                "<html><head><title>%d %s</title></head>"
  "<body><h1>%s</h1><p>(Google what it means yourself)</p></body></html>";

 public:
  explicit ErrorResponse(int error = 500) noexcept;
  ~ErrorResponse() noexcept override;
  ErrorResponse(const ErrorResponse& other) noexcept;
  ErrorResponse& operator=(const ErrorResponse& rhs);

  const char* what() const noexcept override;
};
