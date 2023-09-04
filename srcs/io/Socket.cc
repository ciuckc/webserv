#include "Socket.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <cerrno>
#include <arpa/inet.h>

#include "IOException.h"
#include "util/Log.h"
#include "util/WebServ.h"
#include "util/String.h"

Socket::Socket() {
  fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd_ == -1)
    throw IOException("Opening socket failed", errno);
  if (fcntl(fd_, F_SETFL, O_NONBLOCK) == -1)
    throw IOException("Failed to set fd to NONBLOCK", errno);
}

Socket::~Socket() {
  if (fd_ != -1) {
    close();
  }
}

Socket::Socket(int fd) : fd_(fd) {}

Socket::Socket(int fd, std::string name) : fd_(fd), name_(std::move(name)) {}

Socket::Socket(Socket&& other) noexcept {
  fd_ = other.fd_;
  other.fd_ = -1;
  name_ = std::move(other.name_);
}

Socket& Socket::operator=(Socket&& other) noexcept {
  fd_ = other.fd_;
  other.fd_ = -1;
  name_ = std::move(other.name_);
  return *this;
}

void Socket::bind(uint16_t port) {
  addrinfo* bind_info;

  int yes = 1;
  setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
  setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  {
    std::string port_str = std::to_string(port);
    name_ = '[' + port_str + ']';
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(nullptr, port_str.c_str(), &hints, &bind_info);
    if (status != 0) {
      std::string str = "getaddrinfo: ";
      str += gai_strerror(status);
      str += '\n';
      throw IOException(str);
    }
  }

  bool bound = false;
  for (addrinfo* i = bind_info; i != nullptr; i = i->ai_next) {
    if (::bind(fd_, i->ai_addr, i->ai_addrlen) == 0) {
      bound = true;
      break;
    }
  }

  freeaddrinfo(bind_info);
  if (!bound)
    throw IOException("Failed to bind to host", errno);
}

void Socket::listen(int backlog) const {
  if (::listen(fd_, backlog))
    throw IOException("Failed to set socket to listen", errno);
}

Socket Socket::accept() const {
  sockaddr_in addr = {};
  socklen_t len = sizeof(sockaddr_in);
  int fd = ::accept(fd_, reinterpret_cast<sockaddr*>(&addr), &len);
  if (fd < 0)
    throw IOException("Failed to handle request", errno);
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
    throw IOException("Failed to set fd to NONBLOCK", errno);

  std::string addrstr = Str::join(inet_ntoa(addr.sin_addr), ":", std::to_string(ntohs(addr.sin_port)));
  std::string name = Str::join(util::terminal_colours[fd % 8], name_, "->[", addrstr, "]", util::RESET);
  Log::info(name_, "==[Listening]\t\t\t\tAccept ", addrstr, "-> fd ", fd, '\n');
  return {fd, std::move(name)};
}

int Socket::get_fd() const {
  return fd_;
}

const std::string& Socket::getName() const {
  return name_;
}

ssize_t Socket::write(char* buf, ssize_t len, size_t offs) const {
  return ::write(fd_, buf + offs, len);
}

ssize_t Socket::write(const std::string& str, size_t offs) const {
  return ::write(fd_, str.c_str() + offs, str.length() - offs);
}

ssize_t Socket::read(char* buf, ssize_t len, size_t offs) const {
  return ::read(fd_, buf + offs, len - offs);
}

void Socket::shutdown(int channel) {
  ::shutdown(fd_, channel);
}
