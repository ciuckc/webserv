#include <algorithm>
#include "ConfigServer.h"

ConfigServer::ConfigServer()
  : hostnames_(), port_(8080), client_max_body_size_(1024) {}

void ConfigServer::setPort(uint16_t port) {
  this->port_ = port;
}

void ConfigServer::addServerName(const std::string& server_name) {
  this->hostnames_.push_back(server_name);
}

void ConfigServer::setClientBodyMaxSize(const std::size_t& size) {
  this->client_max_body_size_ = size;
}

const std::vector<std::string>& ConfigServer::getHostnames() const {
  return hostnames_;
}
uint16_t ConfigServer::getPort() const {
  return port_;
}
std::size_t ConfigServer::getClientMaxBodySize() const {
  return client_max_body_size_;
}
const ConfigServer::routes_t& ConfigServer::getRoutes() const {
  return routes_;
}

ConfigServer::routes_t::const_iterator ConfigServer::matchRoute(const std::string& path) const {
  const auto routeMatches = [&path](const std::pair<std::string, ConfigRoute>& route)->bool {
    return (!path.compare(0, route.first.length(), route.first));
  };
  return (std::find_if(routes_.begin(), routes_.end(), routeMatches));
}
