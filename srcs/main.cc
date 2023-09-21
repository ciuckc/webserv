#include <sys/wait.h>

#include "Server.h"
#include "config/ConfigFile.h"
#include "config/ConfigParse.h"

#define DEFAULT_CONFIG_FILE "./webserv.conf"

Config parseConfig(const char* file) {
  ConfigFile config_file(file);
  ConfigParse parser(config_file.getFileData());
  return parser.parse();
}

static Config fakeConfig() {
  Config cfg;
  ConfigServer srv;
  ConfigRoute route;
  route.setRoot("/home/mbatstra/webserv/html/");
  // route.addIndexFile("index.html");
  route.addAcceptedMethod(HTTP::GET);
  route.addAcceptedMethod(HTTP::DELETE);
  route.setAutoIndex(true);
  srv.setPort(6969);
  srv.addRoute("/", std::move(route));
  srv.addServerName("localhost");
  srv.addServerName("127.0.0.1");
  srv.addRoute("/poop/", {});
  cfg.addServer(srv);
  srv.setPort(8080);
  cfg.addServer(srv);
  srv.setPort(12345);
  cfg.addServer(srv);

  return cfg;
}

int main(int argc, char* argv[]) {
  (void) argc, (void)argv;
  // constexpr const char* default_cfg_file = "./webserv.conf";
  //signal(SIGPIPE, SIG_IGN);
  try {
    Config cfg = fakeConfig();//parseConfig(argc == 2 ? argv[1] : default_cfg_file);
    Server server(cfg);
    server.loop();
  } catch (const std::exception& ex) {
    Log::error("Uncaught exception: ", ex.what());
  }
}
