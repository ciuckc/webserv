#pragma once
#include <exception>
#include <string>

class IOException : public std::exception {
 private:
  IOException(); // = delete;
  IOException& operator=(const IOException& rhs); // = delete;

  std::string msg_;

 public:
  explicit IOException(const std::string& msg) : msg_(msg) {};
  IOException(const std::string& msg, int err) {
    std::string str = msg;
    str += ": ";
    str += std::strerror(err);
    str += '\n';
    msg_ = str;
  }
  ~IOException() throw() {};
  IOException(const IOException& other) throw(): msg_(other.msg_) {}

  const char* what() const throw() { return msg_.c_str(); }
};
