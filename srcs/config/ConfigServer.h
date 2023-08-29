#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include "ConfigRoute.h"

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

  using routes_t = std::multimap<std::string, ConfigRoute, std::greater<>>;

  void setListen(const SocketAddress& endpoint);
  void setServerName(const std::string& server_name);
  void setClientBodyMaxSize(const std::size_t& size);
  void addRoute(const std::string& loc, ConfigRoute&& route) {
    routes_.emplace(loc, std::forward<ConfigRoute>(route));
  }

  [[nodiscard]] const std::string& getHostname() const;
  [[nodiscard]] const SocketAddress& getEndpoint() const;
  [[nodiscard]] std::size_t getClientMaxBodySize() const;
  [[nodiscard]] const routes_t& getRoutes() const;

 private:
  std::string hostname_;
  std::string root_;
  SocketAddress endpoint_;
  std::size_t client_max_body_size_;
  routes_t routes_;
};
