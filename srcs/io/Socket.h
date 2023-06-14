#pragma once

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cerrno>
#include <string>

#include "IOException.h"

#ifdef __linux__
#define CORK_OPT TCP_CORK
#else
#define CORK_OPT TCP_NOPUSH
#endif

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
  template <class Iter>
  Iter write(Iter begin, Iter end, size_t& str_off) const;
};

template <class Iter>
Iter Socket::write(Iter begin, Iter end, size_t& str_off) const {
  const std::ptrdiff_t len = end - begin;
  ssize_t total_len = 0;
  iovec vec[len];
  msghdr message = {};
  message.msg_iovlen = len;
  message.msg_iov = vec;
  size_t idx = 0;

  for (Iter cur = begin; cur < end; ++cur) {
    const std::string& str = *cur;
    const char* str_ptr = str.c_str();
    size_t str_len = str.length();
    if (cur == begin && str_off != 0) str_ptr += str_off, str_len -= str_off;
    total_len += (ssize_t)str_len;
    vec[idx++] = (iovec){(void*)str_ptr, str_len};
  }

  ssize_t written = sendmsg(fd_, &message, MSG_NOSIGNAL);
  if (written == -1) throw IOException("Error writing to socket", errno);
  if (written == total_len) return end;
  while (total_len > written) total_len -= (ssize_t)vec[--idx].iov_len;
  str_off = written - total_len;
  return begin + idx;
}
