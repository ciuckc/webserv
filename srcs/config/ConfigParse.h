#pragma once
#include <fstream>

#include "Config.h"

class ConfigParse {
 public:
  ConfigParse();
  ConfigParse(const ConfigParse&) = delete;
  ConfigParse operator=(const ConfigParse&) = delete;
  explicit ConfigParse(const std::string& file_name);
  ~ConfigParse();

  bool parse(Config& config);

 private:
  std::ifstream config_file_;
};
