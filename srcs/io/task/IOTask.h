#pragma once

#include "util/WebServ.h"

class Connection;
class IOTask {
 public:
  IOTask() = default;
  virtual ~IOTask() = default;
  IOTask(IOTask&& other) noexcept = default;
  IOTask& operator=(IOTask&& rhs) noexcept = default;

  /**
   * Called whenever there's Socket data ready
   *
   * for ITask this means there's data in the input buffer
   * for OTask this means the output buffer can be written into
   *
   * Return true if the task is done
   */
  virtual WS::IOStatus operator()(Connection& connection) = 0;

  /**
   * Called after this task is done, but before it's destructed
   */
  virtual void onDone(Connection&) {};
};

// Input task
class ITask : public IOTask {};
// Output task
class OTask : public IOTask {};
