#include "ConfigFile.h"

#include <string>

ConfigFile::InvalidConfigFile::InvalidConfigFile(const std::string arg) throw() : reason_(arg) {}

const char* ConfigFile::InvalidConfigFile::what() const throw() {
  return this->reason_.c_str();
}

ConfigFile::ConfigFile(const std::string file_name) : config_file_(file_name.c_str(), std::ios_base::in) {
  constexpr int equal = 0;
  if (!this->config_file_.is_open()) {
    throw ConfigFile::InvalidConfigFile("couldn't open file");
  }
  if (file_name.length() <= 5) {
    throw ConfigFile::InvalidConfigFile("wrong config file name");
  }
  if (file_name.compare(file_name.length() - 5, 5, ".conf") != equal) {
    throw ConfigFile::InvalidConfigFile("wrong file format");
  }
  file_data_ = readFile();
}

ConfigFile::tokens_t ConfigFile::getFileData() const {
  return this->file_data_;
}

ConfigFile::tokens_t ConfigFile::readFile() {
  std::string buffer;
  tokens_t lines;

  while (true) {
    std::getline(this->config_file_, buffer);
    if (this->config_file_.eof()) {
      break;
    } else if (buffer.empty()) {
      continue;
    }
    lines.push_back(buffer);
  }
  return lines;
}
