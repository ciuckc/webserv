#include "ConfigServer.h"

ConfigServer::ConfigServer() : hostname_("localhost"), root_("html"), endpoint_(), client_max_body_size_(0) {}

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
