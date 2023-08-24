#pragma once

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

struct SocketAddress {
  SocketAddress() : address("0.0.0.0"), port("8080") {}
  std::string address;
  std::string port;
};

class ConfigServer {
 public:
  ConfigServer();
  ConfigServer(ConfigServer&&) = default;
  ConfigServer(const ConfigServer&) = default;
  ConfigServer& operator=(ConfigServer&&) = default;
  ConfigServer& operator=(const ConfigServer&) = default;
  ~ConfigServer() = default;

  void setListen(const SocketAddress& endpoint);
  void setServerName(const std::string& server_name);
  void setRoot(const std::string& root);
  void setClientBodyMaxSize(const std::size_t& size);

 private:
  std::string hostname_;
  std::string root_;
  SocketAddress endpoint_;
  std::size_t client_max_body_size_;
};
