#pragma once
#include <set>
#include <chrono>
#include <unordered_map>

#include "config/Config.h"
#include "io/BufferPool.h"
#include "io/Connection.h"
#include "io/EventQueue.h"
#include "io/Socket.h"
#include "server/VServer.h"

class Server {
 private:
  EventQueue evqueue_;
  // This should probably be moved to every vserver
  std::vector<Socket> listen_sockets_;
  std::unordered_map<int, Connection> connections_;
  int listen_start_;

  BufferPool<> buffer_manager_;
  // These are all the server { } blocks in config file, should be sorted on
  // address/port? think we also need to check if the server_name matches the
  // request perfectly so maybe another map for that?
  // std::multiset<VServer, std::less<VServer> > vservers_;

  using timep_t =  std::chrono::time_point<std::chrono::system_clock>;
  timep_t last_purge_;

  void accept_connection(const EventQueue::event_t& event);
  void handle_connection(EventQueue::event_t& event);

 public:
  Server();
  ~Server();
  explicit Server(const Config& config);

  void loop();
  void purge_connections();
};
