#pragma once
#include <cstring>
#include <exception>
#include <string>

class IOException : public std::exception {
 private:
  std::string msg_;

 public:
  explicit IOException(std::string msg) : msg_(std::move(msg)) {};
  IOException(const std::string& msg, int err) {
    msg_ = msg + ": " + std::strerror(err) + '\n';
  }
  IOException(const IOException& other) noexcept : msg_(other.msg_) {}
  ~IOException() noexcept override = default;
  IOException() = delete;
  IOException& operator=(const IOException& rhs) = delete;

  const char* what() const noexcept override { return msg_.c_str(); }
};
