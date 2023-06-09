#ifndef ACASE_H
#define ACASE_H
#include "Request.h"

class ACase {
  public:
    virtual bool test(Request& req) const = 0;
    virtual bool act(Request& req) const = 0;
};

#endif
