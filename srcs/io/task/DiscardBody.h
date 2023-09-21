#pragma once

#include "IOTask.h"
#include "io/Connection.h"

class DiscardBody : public ITask {
 public:
  explicit DiscardBody(size_t n) : remaining_(n) {};

  WS::IOStatus operator()(Connection& connection) override {
    connection.getInBuffer().discard(remaining_);
    return remaining_ == 0 ? WS::IO_GOOD : WS::IO_AGAIN;
  }

 private:
  size_t remaining_;
};