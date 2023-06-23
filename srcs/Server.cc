#include "Server.h"

#include <iostream>

#include "io/IOException.h"

Server::Server() {
  const int one = 1;
  setsockopt(listen_socket_.get_fd(), SOL_SOCKET, SO_REUSEADDR, &one, 1);
  setsockopt(listen_socket_.get_fd(), SOL_SOCKET, SO_REUSEPORT, &one, 1);
  listen_socket_.bind(nullptr, "6969");
  listen_socket_.listen(128);
  // This is weird for now, as the socket in here will share the file descriptor
  // with the socket inside the Data struct inside the event queue, so when
  // the event queue gets destructed this file descriptor will be invalidated
  // twice, in the socket here and in the queue.
  // Server::addListenSocket() could work? We're gonna need multiple anyway
  evqueue_.add(listen_socket_.get_fd(), EventQueue::in);
}

Server::~Server() = default;

void Server::open_connection(const EventQueue::event_t& event) {
  if (EventQueue::isHangup(event) || EventQueue::isError(event) || EventQueue::isWrite(event))
    throw IOException("Hangup/err/write on listen socket?! The world has gone mad..\n");
  int conn_fd = listen_socket_.accept();
  connections_.emplace(std::piecewise_construct, std::forward_as_tuple(conn_fd),
                       std::forward_as_tuple(conn_fd, evqueue_, buffer_manager_));
}

void Server::loop() {
  std::cout << "[Server] Entering main loop!\n";
  while (true) {
    try {
      EventQueue::event_t& ev = evqueue_.getNext();
      int ev_fd = EventQueue::getFileDes(ev);
      if (ev_fd == listen_socket_.get_fd()) {
        open_connection(ev);
        continue;
      }
      auto found_connection = connections_.find(ev_fd);
      if (found_connection == connections_.end())
        continue;  // This should be impossible
      if (EventQueue::isHangup(ev) || EventQueue::isError(ev)) {
        std::cout << "Closed connection on socket " << ev_fd << '\n';
        connections_.erase(found_connection);
        continue;
      }
      Connection& conn = found_connection->second;
      conn.handle(ev);
      if (conn.shouldClose()) {
        conn.getSocket().shutdown();
        evqueue_.mod(ev_fd, 0);
      }
    } catch (const IOException& err) {
      std::cout << "IOException: " << err.what();
    }
  }
}
