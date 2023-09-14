#include <csignal>

#include "Server.h"
#include "config/ConfigFile.h"
#include "config/ConfigParse.h"

int main(int argc, char* argv[]) {
  constexpr const char* default_cfg_file = "./webserv.conf";
  signal(SIGPIPE, SIG_IGN);
  try {
    Config cfg(argc == 2 ? argv[1] : default_cfg_file);
    Server server(cfg);
    server.loop();
  } catch (const std::exception& ex) {
    Log::error("Uncaught exception: ", ex.what());
  }
}
