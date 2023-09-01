#include <algorithm>
#include "Server.h"

#include "io/IOException.h"
#include "util/Log.h"

Server::Server(Config& config) {
  std::map<uint16_t, int> port_to_fd;

  for (auto& srvcfg : config.getServers()) {
    int fd;
    const uint16_t port = srvcfg.getPort();
    auto it = port_to_fd.find(port);
    if (it == port_to_fd.end())
      port_to_fd[port] = fd = create_listen_sock(port);
    else
      fd = it->second;

    host_map_t& host_map = socket_map_[fd];
    auto& hostnames = srvcfg.getHostnames();
    if (hostnames.empty()) {
      if (!host_map.try_emplace("", srvcfg).second)
        Log::warn("Trying to add duplicate host:port config :", port, " to map!\n");
      continue;
    }
    for (auto& name : hostnames) {
      auto pos = host_map.find(name);
      if (pos == host_map.end())
        host_map.insert({name, srvcfg});
      else
        Log::warn("Trying to add duplicate host:port config ", name, ':', port, " to map!\n");
    }
  }
}

Server::~Server() = default;

int Server::create_listen_sock(uint16_t port) {
  Socket& sock = listen_sockets_.emplace_back();
  sock.bind(port);
  sock.listen(SOMAXCONN);
  int fd = sock.get_fd();
  evqueue_.add(fd);
  if (listen_start_ == -1)
    listen_start_ = fd;
  return fd;
}

void Server::accept_connection(const EventQueue::event_t& event) {
  if (EventQueue::isWrHangup(event) || EventQueue::isError(event) || EventQueue::isWrite(event))
    throw IOException("Hangup/err/write on listen socket?! The world has gone mad..\n");
  const Socket& socket = listen_sockets_[EventQueue::getFileDes(event) - listen_start_];
  int conn_fd = socket.accept();
  connections_.emplace(std::piecewise_construct,
                       std::forward_as_tuple(conn_fd),
                       std::forward_as_tuple(conn_fd, evqueue_, socket_map_[socket.get_fd()]));
}

void Server::purge_connections() {
  timep_t now = std::chrono::system_clock::now();
  if (now - last_purge_ < std::chrono::milliseconds(5000))
    return;
  last_purge_ = now;
  const time_t now_secs = std::time(nullptr);
  for (auto& connection : connections_) {
    if (connection.second.stale(now_secs)) {
      if (!connection.second.idle())
        connection.second.timeout();
      else // We're already sending another response but client doesn't want em
        connection.second.shutdown();
    }
  }
}


void Server::handle_connection(EventQueue::event_t& event) {
  const int fd = EventQueue::getFileDes(event);
  auto found_connection = connections_.find(fd);
  if (found_connection == connections_.end()) {
    Log::warn('[', fd, "]\tGot event on nonexistent connection?\n");
  } else if (EventQueue::isError(event)) {
    Log::warn('[', fd, "]\tError event?!?!\n");
    connections_.erase(found_connection);
  } else if (found_connection->second.handle(event)) {
    connections_.erase(found_connection);
  }
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
