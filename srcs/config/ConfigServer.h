#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <sstream>
#include <string>

#include "ConfigRoute.h"
#include "util/Log.h"

/**
 * @brief The ConfigServer class is a container for all the server related data.
 */
class ConfigServer {
 public:
  ConfigServer();
  ConfigServer(ConfigServer&&) = default;
  ConfigServer(const ConfigServer&) = default;
  ConfigServer& operator=(ConfigServer&&) = default;
  ConfigServer& operator=(const ConfigServer&) = default;
  ~ConfigServer() = default;

  using routes_t = std::map<std::string, ConfigRoute, std::greater<>>;
  using index_files_t = std::vector<std::string>;

  void setPort(uint16_t port);
  void addServerName(std::string&& server_name);
  void setClientBodyMaxSize(size_t size);
  void addRoute(std::string&& loc, ConfigRoute&& route);
  void addErrorPage(int error, std::string&& path);
  void addIndexFiles(const std::vector<std::string>& files);
  void setAutoIndex(bool value);

  [[nodiscard]] const std::vector<std::string>& getHostnames() const;
  [[nodiscard]] uint16_t getPort() const;
  [[nodiscard]] std::size_t getClientMaxBodySize() const;
  [[nodiscard]] const routes_t& getRoutes() const;
  [[nodiscard]] const std::map<int, std::string>& getErrorPages() const;
  [[nodiscard]] const std::vector<std::string>& getIndexFiles() const;
  [[nodiscard]] bool getAutoIndex() const;

 private:
  routes_t routes_{};
  std::map<int, std::string> error_pages_{};
  index_files_t files_{};
  std::vector<std::string> server_name_{};
  std::size_t client_max_body_size_{std::numeric_limits<size_t>::max()};
  uint16_t port_{8080};
  bool auto_index_{false};
};
