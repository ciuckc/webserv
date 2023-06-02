#pragma once
#include <set>
#include "config/Config.h"
#include "server/VServer.h"

class Server {
 private:
  // These are all the server { } blocks in config file, should be sorted on address/port?
  // think we also need to check if the server_name matches the request perfectly
  // so maybe another map for that?
  std::multiset<VServer, std::less<VServer> > vservers_;
 public:
  explicit Server(const Config &config);
};