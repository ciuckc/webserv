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
    explicit InvalidConfigFile(std::string arg) noexcept;
    const char* what() const noexcept override;

   private:
    std::string reason_;
  };

 public:
  explicit ConfigFile(const char* file_name);
  ConfigFile(const ConfigFile&) = delete;
  ConfigFile& operator=(const ConfigFile&) = delete;
  ~ConfigFile() = default;

  tokens_t&& getFileData();

 private:
  tokens_t readFile();

  std::ifstream config_file_;
  tokens_t file_data_;
};
