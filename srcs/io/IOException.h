#pragma once
#include <cstring>
#include <exception>
#include <string>

class IOException : public std::exception {
 private:
  std::string msg_;

 public:
  explicit IOException(const std::string& msg) : msg_(msg){};
  IOException(const std::string& msg, int err) {
    std::string str = msg;
    str += ": ";
    str += std::strerror(err);
    str += '\n';
    msg_ = str;
  }
  IOException(const IOException& other) noexcept : msg_(other.msg_) {}
  ~IOException() noexcept override = default;
  IOException() = delete;
  IOException& operator=(const IOException& rhs) = delete;

  const char* what() const noexcept override { return msg_.c_str(); }
};
