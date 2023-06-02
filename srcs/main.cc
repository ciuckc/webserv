#include "config/Config.h"
#include "Server.h"

#define DEFAULT_CONFIG_FILE "./webserv.conf"

int main(int argc, char **argv, char **envp) {
  Config config((argc >= 2) ? argv[1] : DEFAULT_CONFIG_FILE);
}