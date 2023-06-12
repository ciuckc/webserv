#ifndef ACASE_H
#define ACASE_H
#include "Request.h"

class ACase {
  public:
    virtual ~ACase();
    virtual bool test(Request& req) const = 0;
    virtual void act(Request& req) const = 0;
};

#endif
