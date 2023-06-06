#include "Server.h"

Server::Server() {
  listen_socket_.bind(NULL, "6969");
  listen_socket_.listen(128);
  evqueue_.add(listen_socket_.get_fd(), &listen_socket_, IN);
}

Server::~Server() {}

void Server::loop() {
  while (true) {
    EventQueue::event event = evqueue_.getNext();
    EventQueue::Data& data = EventQueue::
  }
}
