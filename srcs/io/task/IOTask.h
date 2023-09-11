#pragma once

#include "io/Connection.h"

class IOTask {
 protected:
  virtual ~IOTask() = default;

 public:
  IOTask() = default;
  IOTask(IOTask&& other) noexcept = default;
  IOTask& operator=(IOTask&& rhs) noexcept = default;
  virtual bool operator()(Connection& connection) = 0;
  virtual void onDone(Connection& connection) = 0;
};

// Input task
class ITask : public IOTask {
 public:
  using ptr_type = std::unique_ptr<ITask>;
};
// Output task
class OTask : public IOTask {
 public:
  using ptr_type = std::unique_ptr<OTask>;
};
