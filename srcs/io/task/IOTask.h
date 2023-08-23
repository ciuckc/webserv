#pragma once

#include "io/Connection.h"

class IOTask {
 protected:
  virtual ~IOTask() = default;

 public:
  virtual bool operator()(Connection& connection) = 0;
  virtual void onDone(Connection& connection) = 0;
};

// Input task
class ITask : public IOTask {};
// Output task
class OTask : public IOTask {};
