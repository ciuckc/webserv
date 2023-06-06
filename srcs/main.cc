#include <iostream>

#include "Server.h"

#define DEFAULT_CONFIG_FILE "./webserv.conf"

int main() {
  try {
    Server server;
    server.loop();
  } catch (const std::exception& ex) {
    std::cerr << "Uncaught exception: " << ex.what();
  }
}