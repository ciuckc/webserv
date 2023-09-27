#pragma once

#include <vector>

#include "ConfigServer.h"

/**
 * @brief The Config class is a container for ConfigServer objects
 *
 * This class is used to store all the parsed data from the config file.
 * @see ConfigServer
 */
class Config {
 public:
  explicit Config(const char* file_name = "./webserv.conf");
  Config(const Config& rhs) = default;
  Config& operator=(const Config& rhs) = default;
  ~Config() = default;

  void addServer(ConfigServer&& server);
  [[nodiscard]] std::vector<ConfigServer>& getServers();

 private:
  std::vector<ConfigServer> servers_;
};
