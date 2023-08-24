#include "ConfigServer.h"

ConfigServer::ConfigServer() : hostname_("localhost"), root_("html"), endpoint_(), client_max_body_size_(1024) {}

void ConfigServer::setListen(const SocketAddress& endpoint) {
  this->endpoint_ = endpoint;
}

void ConfigServer::setServerName(const std::string& server_name) {
  this->hostname_ = server_name;
}

void ConfigServer::setRoot(const std::string& root) {
  this->root_ = root;
}

void ConfigServer::setClientBodyMaxSize(const std::size_t& size) {
  this->client_max_body_size_ = size;
}
