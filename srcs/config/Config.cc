#include "Config.h"

Config::Config() : server_directives_() {}

Config::Config(const Config& rhs)
    : server_directives_(rhs.server_directives_) {}

Config::~Config() {}

Config& Config::operator=(const Config& rhs) {
  if (this == &rhs)
    return *this;
  this->server_directives_ = rhs.server_directives_;
  return *this;
}

void Config::addServerDirective(const ConfigBlock& server) {
  server_directives_.push_back(server);
}
