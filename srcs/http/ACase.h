#ifndef ACASE_H
#define ACASE_H
#include "Request.h"
#include "Response.h"

class ACase {
  public:
    ACase() {}
    virtual ~ACase() {}
    virtual bool     test(Request& req) const = 0;
    virtual Response act(Request& req) const = 0;
};

#endif
