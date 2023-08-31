#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "ConfigServer.h"

class Config {
 public:
  Config() = default;
  Config(const Config& rhs) = default;
  Config& operator=(const Config& rhs) = default;
  ~Config() = default;

  void addServer(const ConfigServer& server);
  [[nodiscard]] std::vector<ConfigServer>& getServers() {
    return servers_;
  }

 private:
  std::vector<ConfigServer> servers_;
};
