#pragma once

#include <exception>
#include <fstream>
#include <string>
#include <vector>
class ConfigFile {
 public:
  using tokens_t = std::vector<std::string>;

 public:
  class InvalidConfigFile : public std::exception {
   public:
    InvalidConfigFile(const std::string arg) throw();
    const char* what() const throw() override;

   private:
    std::string reason_;
  };

 public:
  ConfigFile(const std::string file_name);
  ConfigFile(const ConfigFile&) = delete;
  ConfigFile& operator=(const ConfigFile&) = delete;
  ~ConfigFile() = default;

  tokens_t getFileData() const;

 private:
  tokens_t readFile();

  std::ifstream config_file_;
  tokens_t file_data_;
};
