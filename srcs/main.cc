#include <iostream>

#include "ConfigParse.h"
#include "Server.h"

#define DEFAULT_CONFIG_FILE "./webserv.conf"

int main(int argc, char* argv[]) {
  try {
    ConfigParse parser(argc > 1 ? argv[1] : DEFAULT_CONFIG_FILE);
    Server server;
    server.loop();
  } catch (const std::exception& ex) {
    std::cerr << "Uncaught exception: " << ex.what() << std::endl;
  }
}
