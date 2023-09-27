#pragma once

#include <memory>

#include "IOTask.h"

class SimpleBody : public OTask {
 private:
  std::unique_ptr<char[]> data_;
  size_t len_;
  size_t ofs_;

 public:
  SimpleBody(std::unique_ptr<char[]>&& data, size_t len);

  WS::IOStatus operator()(Connection& connection) override;
  void onDone(Connection& connection) override;
};
