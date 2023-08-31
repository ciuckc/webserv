#pragma once

#include <memory>
#include "IOTask.h"

class SimpleBody : public OTask {
 public:
  SimpleBody(std::unique_ptr<char[]>&& data, size_t len)
    : data_(std::forward<std::unique_ptr<char[]> >(data)), len_(len), ofs_() {}

  bool operator()(Connection& connection) override {
    auto& buf = connection.getBuffer();
    size_t to_write = std::min(len_ - ofs_, buf.outAvailable());
    buf << std::string_view(data_.get() + ofs_, to_write);
    ofs_ += to_write;
    return ofs_ == len_;
  }

 private:
  std::unique_ptr<char[]> data_;
  size_t len_;
  size_t ofs_;
};