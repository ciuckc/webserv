#pragma once

#include <cstdint>
#include <string>

class Socket {
 private:
  int fd_;
  std::string addr_;
  std::string name_;

 public:
  Socket();
  ~Socket();

  explicit Socket(int fd);
  Socket(int fd, std::string addr, std::string name);
  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;

  Socket(const Socket& other) = delete;
  Socket& operator=(const Socket& rhs) = delete;


  void bind(std::uint16_t port);
  void listen(int backlog) const;
  [[nodiscard]] Socket accept() const;

  [[nodiscard]] int get_fd() const;
  [[nodiscard]] const std::string& getName() const;
  [[nodiscard]] const std::string& getAddress() const;

  void shutdown(int channel) const;
  void close();
};
