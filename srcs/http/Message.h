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
  size_t content_length_;

 public:
  Message();
  virtual ~Message();
  Message(const Message& other);
  Message& operator=(const Message& rhs);

  const std::string& getMessage() const;
  const header_t& getHeaders() const;
  std::string getBody() const;
  size_t getContentLength() const;
  void setContentLength(size_t content_length);
  void addHeader(const std::string& key, const std::string& val);
  void addHeader(const std::string& kv_pair);
  void setBody(char* body, size_t body_size);  // ? This will change

  std::ostream& write(std::ostream& out) const;
};

std::ostream& operator<<(std::ostream& out, const Message& msg);
