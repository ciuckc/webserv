#include <algorithm>
#include "Server.h"

#include "io/IOException.h"
#include "util/Log.h"

// todo: feed listen socket info
Server::Server() {
  Socket sock;
  sock.bind(nullptr, "6969");
  sock.listen(128);
  listen_start_ = sock.get_fd();
  listen_sockets_.push_back(std::move(sock));
  for (const auto& s : listen_sockets_)
    evqueue_.add(s.get_fd(), EventQueue::in);
}

Server::~Server() = default;

void Server::accept_connection(const EventQueue::event_t& event) {
  if (EventQueue::isWrHangup(event) || EventQueue::isError(event) || EventQueue::isWrite(event))
    throw IOException("Hangup/err/write on listen socket?! The world has gone mad..\n");
  const Socket& socket = listen_sockets_[EventQueue::getFileDes(event) - listen_start_];
  int conn_fd = socket.accept();
  connections_.emplace(std::piecewise_construct, std::forward_as_tuple(conn_fd),
                       std::forward_as_tuple(conn_fd, evqueue_, buffer_manager_));
}

void Server::purge_connections() {
  timep_t now = std::chrono::system_clock::now();
  if (now - last_purge_ < std::chrono::milliseconds(5000))
    return;
  last_purge_ = now;
  const time_t now_secs = std::time(nullptr);
  for (auto& connection : connections_)
    if (connection.second.stale(now_secs))
      connection.second.timeout();
}


void Server::handle_connection(EventQueue::event_t& event) {
  const int fd = EventQueue::getFileDes(event);
  auto found_connection = connections_.find(fd);
  if (found_connection == connections_.end()) {
    // This should be impossible
    Log::warn('[', fd, "]\tGot event on nonexistent connection?\n");
    return;
  } else if (EventQueue::isError(event)) {
    Log::warn('[', fd, "]\tError event?!?!\n");
    connections_.erase(found_connection);
    return;
  }
  if (found_connection->second.handle(event))
    connections_.erase(found_connection);
}

void Server::loop() {
  Log::info("[Server] Entering main loop!\n");
  while (true) {
    try {
      EventQueue::event_t& ev = evqueue_.getNext(*this);
      if (EventQueue::getFileDes(ev) < listen_start_ + (int)listen_sockets_.size())
        accept_connection(ev);
      else
        handle_connection(ev);
    } catch (const IOException& err) {
      Log::warn("IOException: ", err.what());
    }
  }
}
