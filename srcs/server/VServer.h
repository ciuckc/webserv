#pragma once
#include <stdint.h>

#include <string>
#include <vector>

#include "Route.h"

// one of the server { } blocks in the config file
class VServer {
 private:
  // listening on:
  // uint32_t addr_;
  // uint16_t port_;
  // hostname:
  std::string server_name_;
  // locations:
  std::vector<Route> routes_;
};
