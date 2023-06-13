#include "Socket.h"

#include <fcntl.h>
#include <netdb.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>
#include <netinet/in.h>

#include "IOException.h"

Socket::Socket() {
  fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd_ == -1) throw IOException("Opening socket failed", errno);
  if (fcntl(fd_, F_SETFL, O_NONBLOCK) == -1) throw IOException("Failed to set fd to NONBLOCK", errno);

  // Wait until uncorked to send partial packets, require flush!
  int cork = true;
  setsockopt(fd_, IPPROTO_TCP, CORK_OPT, &cork, sizeof(cork));
}

Socket::~Socket() { close(fd_); }

Socket::Socket(int fd) : fd_(fd) {}

void Socket::bind(const char* host, const char* port) const {
  addrinfo* bind_info;

  {
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(host, port, &hints, &bind_info);
    if (status != 0) {
      std::string str = "getaddrinfo: ";
      str += gai_strerror(status);
      str += '\n';
      throw IOException(str);
    }
  }

  bool bound = false;
  for (addrinfo* i = bind_info; i != NULL; i = i->ai_next) {
    if (::bind(fd_, i->ai_addr, i->ai_addrlen) == 0) {
      bound = true;
      break;
    }
  }

  freeaddrinfo(bind_info);
  if (!bound) throw IOException("Failed to bind to host", errno);
}

void Socket::listen(int backlog) const {
  if (::listen(fd_, backlog)) throw IOException("Failed to set socket to listen", errno);
}

int Socket::accept() const {
  sockaddr_in addr = {};
  socklen_t len = sizeof(sockaddr_in);
  int fd = ::accept(fd_, reinterpret_cast<sockaddr*>(&addr), &len);
  if (fd < 0) throw IOException("Failed to accept request", errno);
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) throw IOException("Failed to set fd to NONBLOCK", errno);

  char* ip_pointer = reinterpret_cast<char*>(&addr.sin_addr.s_addr);
  std::cout << "Accept";
  for (int i = 0; i < 4; i++) std::cout << ' ' << static_cast<int>(*ip_pointer++);
  std::cout << ':' << addr.sin_port << '\n';

  return fd;
}

int Socket::get_fd() const { return fd_; }

void Socket::flush() {
  int cork = false;
  setsockopt(fd_, IPPROTO_TCP, CORK_OPT, &cork, sizeof(cork));
  cork = !cork;
  setsockopt(fd_, IPPROTO_TCP, CORK_OPT, &cork, sizeof(cork));
}

ssize_t Socket::write(char* buf, ssize_t len, size_t offs) const { return ::write(fd_, buf + offs, len); }

ssize_t Socket::write(const std::string& str, size_t offs) const {
  return ::write(fd_, str.c_str() + offs, str.length() - offs);
}

template <class Iter> Iter& Socket::write(Iter& begin, Iter& end, size_t& str_off) {
  const typename Iter::diff_t len = end - begin;
  ssize_t total_len = 0;
  iovec vec[len];
  size_t idx = 0;

  for (Iter& cur = begin; cur < end; ++cur) {
    const std::string& str = (std::string)*cur;
    const char *str_ptr = str.c_str();
    size_t str_len = str.length();
    if (cur == begin && str_off != 0)
      str_ptr += str_off, str_len -= str_off;
    total_len += (ssize_t)str_len;
    vec[idx++] = (iovec){(void*)str_ptr, str_len};
  }

  ssize_t written = writev(fd_, vec, len);
  if (written == -1) throw IOException("Error writing to socket", errno);
  if (written == total_len) return end;
  while (total_len > written)
    total_len -= (ssize_t)vec[--idx].iov_len;
  str_off = written - total_len;
  return begin + idx;
}
