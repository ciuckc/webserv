#pragma once
#include <sys/socket.h>
#include <unistd.h>

#include <string>

class Socket {
 private:
  Socket(const Socket& other);           // = delete
  Socket& operator=(const Socket& rhs);  // = delete

  int fd_;

 public:
  Socket();
  ~Socket();
  explicit Socket(int fd);

  void bind(const char* host, const char* port) const;
  void listen(int backlog) const;
  int accept() const;

  int get_fd() const;

  void flush();
  ssize_t write(char* buf, ssize_t len, size_t offs = 0) const;
  ssize_t write(const std::string& str, size_t offs = 0) const;
  template <class Iter> Iter& write(Iter& begin, Iter& end, size_t &str_off);
};
