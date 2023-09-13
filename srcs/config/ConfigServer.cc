#include "ConfigServer.h"

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

void ConfigServer::addServerName(const std::string& server_name) {
  this->server_name_.push_back(server_name);
}

void ConfigServer::setClientBodyMaxSize(const std::size_t& size) {
  this->client_max_body_size_ = size;
}

void ConfigServer::addRoute(const std::string& loc, ConfigRoute&& route) {
  routes_.emplace(loc, std::forward<ConfigRoute>(route));
}

void ConfigServer::addErrorPage(int error, const std::string& path) {
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
