#include "ConfigFile.h"

#include <string>
#include <utility>

ConfigFile::InvalidConfigFile::InvalidConfigFile(std::string arg) noexcept : reason_(std::move(arg)) {}

const char* ConfigFile::InvalidConfigFile::what() const noexcept {
  return this->reason_.c_str();
}

ConfigFile::ConfigFile(const char* file_name) : config_file_(file_name, std::ios_base::in) {
  constexpr int equal = 0;
  if (!this->config_file_.is_open()) {
    throw ConfigFile::InvalidConfigFile("Ccouldn't open file");
  }
  std::string_view file = file_name;
  if (file.length() <= 5 || file.compare(file.size() - 5, 5, ".conf") != equal) {
    throw ConfigFile::InvalidConfigFile("Config file should end in .conf");
  }
  file_data_ = readFile();
}

ConfigFile::tokens_t&& ConfigFile::getFileData() {
  return std::move(this->file_data_);
}

ConfigFile::tokens_t ConfigFile::readFile() {
  std::string buffer;
  tokens_t lines;

  while (std::getline(this->config_file_, buffer))
    if (!buffer.empty())
      lines.push_back(buffer);

  if (!config_file_.eof())
    throw ConfigFile::InvalidConfigFile("Error while reading config file");
  return lines;
}
