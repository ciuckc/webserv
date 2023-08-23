#include <csignal>

#include "ConfigFile.h"
#include "ConfigParse.h"
#include "Server.h"

#define DEFAULT_CONFIG_FILE "./webserv.conf"

int main() {
  signal(SIGPIPE, SIG_IGN);
  try {
    ConfigFile config_file(argc == 2 ? argv[1] : default_cfg_file);
    ConfigParse parser(config_file.getFileData());
    Config cfg = parser.parse();
    (void)cfg;
    std::abort();
    Server server;
    server.loop();
  } catch (const std::exception& ex) {
    Log::error("Uncaught exception: ", ex.what());
  }
}
