#include <iostream>
#include <fstream>
#include "config/Config.h"
#include "Server.h"
#include "http/Request.h"

#define DEFAULT_CONFIG_FILE "./webserv.conf"

int main(int argc, char **argv, char **envp) {
 // Config config((argc >= 2) ? argv[1] : DEFAULT_CONFIG_FILE);
//
 // (void) config;
  (void) argc;
  (void) argv;
  (void) envp;
  std::ifstream str("../header.txt");
  Request req;
  str >> req;
  std::cout << req;
}