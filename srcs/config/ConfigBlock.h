#pragma once

#include <string>
#include <vector>

class ConfigBlock {
 public:
  ConfigBlock();
  ConfigBlock(const ConfigBlock& rhs);
  ~ConfigBlock();
  ConfigBlock& operator=(const ConfigBlock& rhs);

  void setDirectiveName(const std::string& directive_name);
  void setArgs(const std::vector<std::string>& args);
  void addConfigBlock(const ConfigBlock& new_block);

 private:
  std::string directive_name_;
  std::vector<std::string> args_;
  std::vector<ConfigBlock> blocks_;
};
