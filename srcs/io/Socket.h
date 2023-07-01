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

 public:
  Socket();
  ~Socket();

  explicit Socket(int fd);
  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;

  Socket(const Socket& other) = delete;
  Socket& operator=(const Socket& rhs) = delete;


  void bind(const char* host, const char* port) const;
  void listen(int backlog) const;
  int accept() const;

  int get_fd() const;

  void flush();
  ssize_t write(char* buf, ssize_t len, size_t offs = 0) const;
  ssize_t write(const std::string& str, size_t offs = 0) const;
  ssize_t read(char* buf, ssize_t len, size_t offs = 0) const;

  void shutdown(int channel);

  inline void close() {
    Log::debug('[', fd_, "]\tSocket destroyed\n");
    ::close(fd_);
    fd_ = -1;
  }
};
