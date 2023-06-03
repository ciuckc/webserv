#include "Socket.h"
#include "IOException.h"
#include <cstring>
#include <cerrno>
#include <csignal>
#include <iostream>

const protoent* Socket::tcp = getprotobyname("tcp");

Socket::Socket() {
  int flags = SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK;
  fd_ = socket(AF_INET, flags, tcp->p_proto);
  if (fd_ == -1) {
    std::string str = "Opening socket failed: ";
    str += std::strerror(errno);
    str += '\n';
    throw IOException(str);
  }
}

Socket::~Socket() {
  close(fd_);
}

Socket::Socket(int fd) : fd_(fd) {}

void Socket::bind(const char* host, const char* port) const {
  addrinfo* bind_info;

  {
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = tcp->p_proto;
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
  if (!bound) {
    std::string str = "Failed to bind to host: ";
    str += std::strerror(errno);
    str += '\n';
    throw IOException(str);
  }
}

void Socket::listen(int backlog) const {
  if (::listen(fd_, backlog)) {
    std::string str = "Failed to set socket to listen: ";
    str += std::strerror(errno);
    str += '\n';
    throw IOException(str);
  }
}

int Socket::accept() const {
  sockaddr_in addr = {};
  socklen_t len = sizeof(sockaddr_in);
  int fd = ::accept(fd_, reinterpret_cast<sockaddr*>(&addr), &len);
  if (fd < 0) {
    std::string str = "Failed to accept request: ";
    str += std::strerror(errno);
    str += '\n';
    throw IOException(str);
  }
  char* ip_pointer = reinterpret_cast<char*>(&addr.sin_addr.s_addr);
  std::cout << "Accept";
  for (int i = 0; i < 4; i++)
    std::cout << ' ' << static_cast<int>(*ip_pointer++);
  std::cout << ':' << addr.sin_port << '\n';

  return fd;
}

int Socket::get_fd() const {
  return fd_;
}
