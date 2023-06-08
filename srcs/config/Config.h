#pragma once

#include <string>
#include <vector>

#include "ConfigBlock.h"

class Config {
 public:
  Config();
  Config(const Config& rhs);
  ~Config();
  Config& operator=(const Config& rhs);
  void addServerDirective(const ConfigBlock& server);

 private:
  std::vector<ConfigBlock> server_directives_;
};
