#include "Socket.h"

#include <fcntl.h>
#include <netdb.h>

#include <arpa/inet.h>

#include "util/Log.h"
#include "util/WebServ.h"
#include "util/String.h"

Socket::Socket() {
  fd_ = socket(AF_INET, SOCK_STREAM | O_NONBLOCK | O_CLOEXEC, IPPROTO_TCP);
  if (fd_ == -1)
    throw IOException("Opening socket failed", errno);
}

Socket::~Socket() {
  if (fd_ != -1)
    close();
}

Socket::Socket(int fd) : fd_(fd) {}

Socket::Socket(int fd, std::string addr, std::string name)
    : fd_(fd), addr_(std::move(addr)), name_(std::move(name)) {}

Socket::Socket(Socket&& other) noexcept {
  fd_ = other.fd_;
  other.fd_ = -1;
  name_ = std::move(other.name_);
  addr_ = std::move(other.addr_);
}

Socket& Socket::operator=(Socket&& other) noexcept {
  fd_ = other.fd_;
  other.fd_ = -1;
  name_ = std::move(other.name_);
  addr_ = std::move(other.addr_);
  return *this;
}

void Socket::bind(std::uint16_t port) {
  addrinfo* bind_info;

  // uncomment the next 3 lines if you wanna be able to rerun the program quickly
  int yes = 1;
  setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
  setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  {
    std::string port_str = std::to_string(port);
    name_ = Str::join(util::terminal_colours[fd_ % 8], "[", port_str, "]", util::RESET);
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(nullptr, port_str.c_str(), &hints, &bind_info);
    if (status != 0)
      throw IOException(Str::join("getaddrinfo: ", gai_strerror(status), "\n"));
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
  int fd =   accept4(fd_, reinterpret_cast<sockaddr*>(&addr), &len, SOCK_NONBLOCK);
  if (fd < 0)
    throw IOException("Failed to handle request", errno);

  std::string remote_addr = inet_ntoa(addr.sin_addr);
  std::string addrstr = Str::join(util::terminal_colours[fd % 8], "[", remote_addr, ":",
                                  std::to_string(ntohs(addr.sin_port)), "]", util::RESET);
  Log::info(name_, "\tAccepting incoming\t", addrstr, "->[fd ", fd, "]\n");
  return {fd, remote_addr, Str::join(name_, "->", std::move(addrstr))};
}

int Socket::get_fd() const {
  return fd_;
}

const std::string& Socket::getName() const {
  return name_;
}

const std::string& Socket::getAddress() const {
  return addr_;
}

void Socket::shutdown(int channel) const {
  ::shutdown(fd_, channel);
}

void Socket::close() {
  Log::debug(name_, "\tSocket destroyed\n");
  ::close(fd_);
  fd_ = -1;
}
