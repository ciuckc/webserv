#pragma once

#include <string>
#include <vector>

class Message {
 public:
  typedef std::vector<std::string> header_t;

 protected:
  std::string message_;
  header_t headers_;
  char* body_;
  size_t content_length_ = 0;

 public:
  Message() = default;
  virtual ~Message() = default;
  Message(const Message& other) = default;
  Message& operator=(const Message& rhs) = default;

  const std::string& getMessage() const;
  const header_t& getHeaders() const;
  size_t getContentLength() const;
  void setContentLength(size_t content_length);
  void addHeader(const std::string& key, const std::string& val);
  void addHeader(const std::string& kv_pair);

  std::ostream& write(std::ostream& out) const;
};

std::ostream& operator<<(std::ostream& out, const Message& msg);
