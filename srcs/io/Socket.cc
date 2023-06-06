#include "Socket.h"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>

#include "IOException.h"

const protoent* Socket::tcp = getprotobyname("tcp");

Socket::Socket() {
  fd_ = socket(AF_INET, SOCK_STREAM, tcp->p_proto);
  if (fd_ == -1) throw IOException("Opening socket failed", errno);
  if (fcntl(fd_, F_SETFL, O_NONBLOCK) == -1) throw IOException("Failed to set fd to NONBLOCK", errno);
}

Socket::~Socket() { close(fd_); }

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
