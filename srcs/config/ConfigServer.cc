#include "ConfigServer.h"

ConfigServer::ConfigServer() : hostname_("localhost") {}

void ConfigServer::addHostname(const std::string hostname) {
  this->hostname_ = hostname;
}
