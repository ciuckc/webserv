#include "Config.h"

void Config::addServer(const ConfigServer& server) {
  this->servers_.push_back(server);
}
