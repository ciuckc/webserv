#include "ConfigParse.h"

#include <sys/_types/_size_t.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>

namespace parse_utils {
void tokenize(std::ifstream& config_file, std::vector<std::string>& tokens);
}

ConfigParse::ConfigParse() {}

ConfigParse::ConfigParse(const std::string& file_name_)
    : config_file_(file_name_.c_str(), std::ios_base::in) {
  if (!config_file_.is_open())
    throw std::exception();
}

ConfigParse::~ConfigParse() {}

void parse_utils::tokenize(std::ifstream& config_file,
                           std::vector<std::string>& tokens) {
  std::string buffer;

  while (true) {
    std::getline(config_file, buffer);
    if (config_file.eof())
      break;
    if (buffer.empty())
      continue;
    size_t start_idx = 0;
    size_t end_idx = buffer.find_first_not_of("\t ", start_idx);
    while (end_idx != buffer.npos) {
      start_idx = buffer.find_first_not_of("\t ", end_idx);
      end_idx = buffer.find_first_of("\t ", start_idx);
      std::string token = buffer.substr(start_idx, end_idx - start_idx);
      tokens.push_back(token);
    }
  }
}

bool ConfigParse::parse(Config& config) {
  (void)config;
  std::vector<std::string> tokens;
  parse_utils::tokenize(this->config_file_, tokens);
  for (size_t i = 0; i < tokens.size(); ++i)
    std::cout << tokens[i] << std::endl;
  return true;
}
