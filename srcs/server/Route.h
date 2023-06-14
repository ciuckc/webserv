#pragma once
#include "http/Request.h"
#include "http/Response.h"

class Route {
 public:
  virtual Response handle(const Request& request) = 0;
  virtual ~Route();
};
