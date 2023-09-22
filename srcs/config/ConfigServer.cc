#include "ConfigServer.h"

#include <algorithm>
#include <limits>

ConfigServer::ConfigServer()
    : server_name_{"localhost"},
      port_(8080),
      client_max_body_size_(std::numeric_limits<size_t>::max()),
      routes_(),
      error_pages_(),
      files_{},
      auto_index_(false) {}

void ConfigServer::setPort(uint16_t port) {
  this->port_ = port;
}

void ConfigServer::addServerName(std::string&& server_name) {
  this->server_name_.push_back(std::forward<std::string>(server_name));
}

void ConfigServer::setClientBodyMaxSize(std::size_t size) {
  this->client_max_body_size_ = size;
}

void ConfigServer::addRoute(std::string&& loc, ConfigRoute&& route) {
  routes_.emplace(std::forward<std::string>(loc), std::forward<ConfigRoute>(route));
}

void ConfigServer::addErrorPage(int error, std::string&& path) {
  if (!error_pages_.emplace(error, path).second)
    Log::warn("Duplicate error page for error ", error, ": ", path, '\n');
}

const std::vector<std::string>& ConfigServer::getHostnames() const {
  return server_name_;
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

const std::map<int, std::string>& ConfigServer::getErrorPages() const {
  return error_pages_;
}

ConfigServer::routes_t::const_iterator ConfigServer::matchRoute(const std::string& path) const {
  const auto routeMatches = [&path](const std::pair<std::string, ConfigRoute>& route)->bool {
    return (!path.compare(0, route.first.length(), route.first));
  };
  return (std::find_if(routes_.begin(), routes_.end(), routeMatches));
}
