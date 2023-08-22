#pragma once

#include <cstdint>
#include <sstream>
#include <string>

struct SocketAddress {
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

  void addHostname(const std::string hostname);

 private:
  std::string hostname_;
  SocketAddress endpoint_;
};
