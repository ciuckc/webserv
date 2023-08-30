#pragma once

#include "IOTask.h"

class DiscardBody : public ITask {
 public:
  explicit DiscardBody(size_t n) : remaining_(n) {};

  bool operator()(Connection& connection) override {
    remaining_ -= connection.getBuffer().discard(remaining_);
    return remaining_ == 0;
  }
  void onDone(Connection&) override {}

 private:
  size_t remaining_;
};