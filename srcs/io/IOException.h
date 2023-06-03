#pragma once
#include <exception>
#include <string>

class IOException : public std::exception {
 private:
  IOException(); // = delete;
  IOException& operator=(const IOException& rhs); // = delete;

  const std::string msg_;

 public:
  explicit IOException(const std::string& msg) : msg_(msg) {};
  ~IOException() throw() {};
  IOException(const IOException& other) throw(): msg_(other.msg_) {}

  const char* what() throw() { return msg_.c_str(); }
};
