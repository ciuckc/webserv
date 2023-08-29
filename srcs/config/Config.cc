#include "Config.h"

#include <cstddef>
#include <memory>
#include <vector>

void Config::addServer(const ConfigServer& server) {
  this->servers_.push_back(std::move(server));
}
