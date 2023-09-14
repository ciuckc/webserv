#include "Config.h"

#include <utility>

#include "config/ConfigFile.h"
#include "config/ConfigParse.h"

Config::Config(const char* file_name) : servers_() {
  ConfigFile config_file(file_name);
  ConfigParse parser(config_file.getFileData());
  parser.parse(*this);
}

void Config::addServer(const ConfigServer& server) {
  this->servers_.push_back(server);
}

void Config::addServer(ConfigServer&& server) {
  this->servers_.emplace_back(std::forward<ConfigServer>(server));
}
