#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include "ConfigRoute.h"
#include "util/Log.h"

class ConfigServer {
 public:
  ConfigServer();
  ConfigServer(ConfigServer&&) = default;
  ConfigServer(const ConfigServer&) = default;
  ConfigServer& operator=(ConfigServer&&) = default;
  ConfigServer& operator=(const ConfigServer&) = default;
  ~ConfigServer() = default;

  using routes_t = std::multimap<std::string, ConfigRoute, std::greater<>>;

  void setPort(uint16_t port);
  void addServerName(const std::string& server_name);
  void setClientBodyMaxSize(const std::size_t& size);
  void addRoute(const std::string& loc, ConfigRoute&& route) {
    routes_.emplace(loc, std::forward<ConfigRoute>(route));
  }
  void addErrorPage(int error, const std::string& path) {
    if (!error_pages_.emplace(error, path).second)
      Log::warn("Duplicate error page for error ", error, ": ", path, '\n');
  }

  [[nodiscard]] const std::vector<std::string>& getHostnames() const;
  [[nodiscard]] uint16_t getPort() const;
  [[nodiscard]] std::size_t getClientMaxBodySize() const;
  [[nodiscard]] const routes_t& getRoutes() const;
  [[nodiscard]] const std::map<int, std::string>& getErrorPages() const {
    return error_pages_;
  }

 private:
  std::vector<std::string> hostnames_;
  // std::string root_; todo: move to ConfigRoute
  uint16_t port_;
  std::size_t client_max_body_size_;
  routes_t routes_;
  std::map<int, std::string> error_pages_;
};
