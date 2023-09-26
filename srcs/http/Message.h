#pragma once

#include <string>
#include <vector>
#include "util/String.h"

class Message {
 public:
  typedef std::vector<std::string> header_t;

 protected:
  std::string message_;
  header_t headers_;
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
  void addHeader(const std::string& kv_pair);
  template<class S1, class S2> void addHeader(const S1& key, const S2& val);

  std::ostream& write(std::ostream& out) const;
};

template<class S1, class S2> void Message::addHeader(const S1& key, const S2& val) {
  headers_.push_back(Str::join(key, ": ", val, "\r\n"));
}

std::ostream& operator<<(std::ostream& out, const Message& msg);
