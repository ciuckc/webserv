#include "ConfigParse.h"

#include <sys/_types/_size_t.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

namespace parse_utils {
enum {
  EMPTY
};
void split_on_white_space(std::ifstream& config_file, std::vector<std::string>& tokens);
void split_on_symbols(std::vector<std::string>& tokens);
}  // namespace parse_utils

ConfigParse::ConfigParse() {}

ConfigParse::ConfigParse(const std::string& file_name_)
    : config_file_(file_name_.c_str(), std::ios_base::in) {
  if (!config_file_.is_open())
    throw std::exception();
}

ConfigParse::~ConfigParse() {}

void parse_utils::split_on_white_space(std::ifstream& config_file, std::vector<std::string>& tokens) {
  std::string buffer;
  std::string token;
  size_t start_idx = 0;
  size_t end_idx = 0;

  while (true) {
    std::getline(config_file, buffer);
    if (config_file.eof())
      break;
    if (buffer.empty())
      continue;
    buffer = buffer.substr(0, buffer.find("#"));
    if (buffer.empty())
      continue;
    start_idx = 0;
    end_idx = buffer.find_first_not_of("\t ", start_idx);
    while (end_idx != buffer.npos) {
      start_idx = buffer.find_first_not_of("\t ", end_idx);
      end_idx = buffer.find_first_of("\t ", start_idx);
      token = buffer.substr(start_idx, end_idx - start_idx);
      tokens.push_back(token);
    }
  }
}

// We will split a string if we find delimiters and then erase that string or if
// we don't have any delimiter on which we can split, we just continue
// forward iterating
void parse_utils::split_on_symbols(std::vector<std::string>& tokens) {
  size_t symbol_idx = 0;
  size_t start_idx = 0;
  std::string word;
  const std::string delimiters = "{}=;";
  auto it = tokens.begin();

  while (it != tokens.end()) {
    symbol_idx = it->find_first_of(delimiters);
    if (symbol_idx == it->npos) {
      ++it;
      continue;
    }
    start_idx = 0;
    while (symbol_idx != it->npos) {
      if (start_idx < symbol_idx) {
        word = it->substr(start_idx, symbol_idx - start_idx);
        tokens.insert(it, word);
      }
      word = it->substr(symbol_idx, 1);
      tokens.insert(it, word);
      start_idx = symbol_idx + 1;
      symbol_idx = it->find_first_of(delimiters, start_idx);
    }
    if (start_idx < it->length()) {
      word = it->substr(start_idx);
      tokens.insert(it, word);
    }
    it = tokens.erase(it);
  }
}

bool ConfigParse::parse(Config& config) {
  (void)config;
  std::vector<std::string> tokens;
  parse_utils::split_on_white_space(config_file_, tokens);
  if (tokens.size() == parse_utils::EMPTY) {
    std::cout << "Configuration file is empty!" << std::endl;
    exit(1);
  }
  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    std::cout << *it << std::endl;
  }
  parse_utils::split_on_symbols(tokens);
  return true;
}
