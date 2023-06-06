#pragma once
#include <set>
#include <map>

#include "config/Config.h"
#include "io/EventQueue.h"
#include "io/Socket.h"
#include "server/VServer.h"

class Server {
 private:
  EventQueue evqueue_;
  // This should probably be moved to every vserver
  Socket listen_socket_;
  //std::map<int,

  // These are all the server { } blocks in config file, should be sorted on
  // address/port? think we also need to check if the server_name matches the
  // request perfectly so maybe another map for that?
  std::multiset<VServer, std::less<VServer> > vservers_;

 public:
  Server();
  ~Server();
  explicit Server(const Config& config);

  void loop();
};
