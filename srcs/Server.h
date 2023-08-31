#pragma once
#include <set>
#include <chrono>
#include <unordered_map>

#include "config/Config.h"
#include "io/Connection.h"
#include "io/EventQueue.h"
#include "io/Socket.h"

class Server {
 private:
  using host_map_t = std::map<std::string, ConfigServer&>;

  EventQueue evqueue_;
  // All the sockets we are listening on
  // These sockets may be shared between multiple ConfigServers
  std::vector<Socket> listen_sockets_;
  // This map maps listen socket fd to server config directives (mapped to hostname)
  std::map<int, host_map_t> socket_map_;
  // This map maps connection fd to Connection object
  std::unordered_map<int, Connection> connections_;
  // The lowest file descriptor that's a listening port
  int listen_start_ = -1;

  using timep_t =  std::chrono::time_point<std::chrono::system_clock>;
  timep_t last_purge_;

  int create_listen_sock(uint16_t port);
  void accept_connection(const EventQueue::event_t& event);
  void handle_connection(EventQueue::event_t& event);

 public:
  explicit Server(Config& config);
  ~Server();

  void loop();
  void purge_connections();
};
