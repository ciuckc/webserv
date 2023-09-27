#include "Server.h"
#include "config/Config.h"

int main(int argc, char* argv[]) {
  constexpr const char* default_cfg_file = "./webserv.conf";
  try {
    Config cfg(argc == 2 ? argv[1] : default_cfg_file);
    Server server(cfg);
    server.loop();
  } catch (const std::exception& ex) {
    Log::error("Uncaught exception: ", ex.what());
    return 1;
  }
}
