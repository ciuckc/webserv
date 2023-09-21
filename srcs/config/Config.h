#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "ConfigServer.h"

class Config {
 public:
  Config(const char* file_name = "./webserv.conf");
  Config(const Config& rhs) = default;
  Config& operator=(const Config& rhs) = default;
  ~Config() = default;

  void addServer(ConfigServer&& server);
  [[nodiscard]] std::vector<ConfigServer>& getServers();

 private:
  std::vector<ConfigServer> servers_;
};
