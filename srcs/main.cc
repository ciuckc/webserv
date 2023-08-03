#include <cstdlib>
#include <iostream>

#include "ConfigFile.h"
#include "ConfigParse.h"
#include "Server.h"

int main(int argc, char* argv[]) {
  constexpr const char* default_cfg_file = "./webserv.conf";
  try {
    ConfigFile config_file(argc == 2 ? argv[1] : default_cfg_file);
    ConfigParse parser(config_file.getFileData());
    Config cfg = parser.parse();
    (void)cfg;
    std::abort();
    Server server;
    server.loop();
  } catch (const std::exception& ex) {
    std::cerr << "Uncaught exception: " << ex.what() << std::endl;
  }
}
