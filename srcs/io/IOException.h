#pragma once
#include <exception>
#include "util/String.h"

class IOException : public std::exception {
 private:
  std::string msg_;

 public:
  explicit IOException(std::string msg) : msg_(std::move(msg)) {};
  IOException(const std::string& msg, int err) {
    msg_ = Str::join(msg, ": ", std::strerror(err), "\n");
  }
  IOException(const IOException& other) noexcept : msg_(other.msg_) {}
  ~IOException() noexcept override = default;
  IOException() = delete;
  IOException& operator=(const IOException& rhs) = delete;

  [[nodiscard]] const char* what() const noexcept override { return msg_.c_str(); }
};
