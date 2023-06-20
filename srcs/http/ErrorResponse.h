#pragma once
#include <exception>

#include "Response.h"

class ErrorResponse : public Response, public std::exception {
 private:
  static const std::string& errpage_template;

 public:
  explicit ErrorResponse(int error = 500) throw();
  ~ErrorResponse() throw();
  ErrorResponse(const ErrorResponse& other) throw();
  ErrorResponse& operator=(const ErrorResponse& rhs);

  const char* what() const throw();
};
