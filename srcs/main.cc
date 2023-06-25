#include <csignal>

#include "Server.h"

#define DEFAULT_CONFIG_FILE "./webserv.conf"

int main() {
  signal(SIGPIPE, SIG_IGN);
  try {
    Server server;
    server.loop();
  } catch (const std::exception& ex) {
    Log::error("Uncaught exception: ", ex.what());
  }
}