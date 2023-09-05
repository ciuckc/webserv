#pragma once

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cerrno>
#include <string>

#include "IOException.h"
#include "util/Log.h"

class Socket {
 private:
  int fd_;
  std::string name_;

 public:
  Socket();
  ~Socket();

  explicit Socket(int fd);
  Socket(int fd, std::string name);
  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;

  Socket(const Socket& other) = delete;
  Socket& operator=(const Socket& rhs) = delete;


  void bind(uint16_t port);
  void listen(int backlog) const;
  Socket accept() const;

  int get_fd() const;
  const std::string& getName() const;

  ssize_t write(char* buf, ssize_t len, size_t offs = 0) const;
  ssize_t write(const std::string& str, size_t offs = 0) const;
  ssize_t read(char* buf, ssize_t len, size_t offs = 0) const;

  void shutdown(int channel);

  inline void close() {
    Log::debug(name_, "\tSocket destroyed\n");
    ::close(fd_);
    fd_ = -1;
  }
};
