#include "Config.h"

#include <utility>

void Config::addServer(const ConfigServer& server) {
  this->servers_.push_back(server);
}

void Config::addServer(ConfigServer&& server) {
  this->servers_.emplace_back(std::forward<ConfigServer>(server));
}
