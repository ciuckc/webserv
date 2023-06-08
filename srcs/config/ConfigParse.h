#pragma once
#include <fstream>

#include "Config.h"

class ConfigParse {
 public:
  ConfigParse();
  explicit ConfigParse(const std::string& file_name);
  ~ConfigParse();

  bool parse(Config& config);

 private:
  ConfigParse(const ConfigParse&);            // = delete
  ConfigParse operator=(const ConfigParse&);  // = delete

  std::ifstream config_file_name_;
};
