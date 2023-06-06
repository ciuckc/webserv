#pragma once
#include <netdb.h>
#include <sys/socket.h>

#include <string>

class Socket {
 private:
  static const protoent* tcp;

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
};
