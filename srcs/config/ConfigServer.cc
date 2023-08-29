#include "ConfigServer.h"

ConfigServer::ConfigServer() : hostname_("localhost"), endpoint_(), client_max_body_size_(1024) {}

void ConfigServer::setListen(const SocketAddress& endpoint) {
  this->endpoint_ = endpoint;
}

void ConfigServer::setServerName(const std::string& server_name) {
  this->hostname_ = server_name;
}

void ConfigServer::setClientBodyMaxSize(const std::size_t& size) {
  this->client_max_body_size_ = size;
}

const std::string& ConfigServer::getHostname() const {
  return hostname_;
}
const SocketAddress& ConfigServer::getEndpoint() const {
  return endpoint_;
}
std::size_t ConfigServer::getClientMaxBodySize() const {
  return client_max_body_size_;
}
const ConfigServer::routes_t& ConfigServer::getRoutes() const {
  return routes_;
}