#include "ConfigParse.h"

#include <sys/_types/_size_t.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

namespace parse_utils {
void split_on_white_space(std::ifstream& config_file, std::list<std::string>& tokens);
void split_on_symbols(std::list<std::string>& tokens);
void tokenize(std::list<std::string>& tokens);
}  // namespace parse_utils

ConfigParse::ConfigParse() {}

ConfigParse::ConfigParse(const std::string& file_name_)
    : config_file_(file_name_.c_str(), std::ios_base::in) {
  if (!config_file_.is_open()) throw std::exception();
}

ConfigParse::~ConfigParse() {}

void parse_utils::split_on_white_space(std::ifstream& config_file, std::list<std::string>& tokens) {
  std::string buffer;
  std::string token;
  size_t start_idx = 0;
  size_t end_idx = 0;

  while (true) {
    std::getline(config_file, buffer);
    if (config_file.eof()) break;
    if (buffer.empty()) continue;
    buffer = buffer.substr(0, buffer.find("#"));  // ignore comments
    if (buffer.empty()) continue;
    start_idx = 0;                                         // start of line
    end_idx = buffer.find_first_not_of("\t ", start_idx);  // star of the first word on that line
    while (end_idx != buffer.npos) {                       // loop that splits the line at white spaces
      start_idx = buffer.find_first_not_of("\t ", end_idx);
      end_idx = buffer.find_first_of("\t ", start_idx);
      token = buffer.substr(start_idx, end_idx - start_idx);
      tokens.push_back(token);
    }
  }
}

// i lost my skills in string operations:(
void parse_utils::split_on_symbols(std::list<std::string>& tokens) {
  size_t symbol_idx = 0;
  size_t start_idx = 0;
  std::string word;
  std::list<std::string>::iterator it = tokens.begin();

  while (it != tokens.end()) {
    symbol_idx = it->find_first_of("{}=;");
    if (symbol_idx == it->npos) {
      ++it;
      continue;
    }
    start_idx = 0;
    while (symbol_idx != it->npos) {
      if (start_idx != symbol_idx) {
        word = it->substr(start_idx, symbol_idx - start_idx);
        tokens.insert(it, word);
      }
      start_idx = symbol_idx;
      symbol_idx = it->find_first_of("{}=;", symbol_idx + 1);
    }
    if (start_idx != symbol_idx) {
      word = it->substr(start_idx);
      tokens.insert(it, word);
    }
    it = tokens.erase(it);
  }
}

void parse_utils::tokenize(std::list<std::string>& tokens) { (void)tokens; }

bool ConfigParse::parse(Config& config) {
  (void)config;
  std::list<std::string> tokens;
  parse_utils::split_on_white_space(this->config_file_, tokens);
  if (!tokens.size()) {
    std::cout << "Configuration file is empty! Default config..." << std::endl;
    exit(1);
  }
  parse_utils::split_on_symbols(tokens);
  for (std::list<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
    std::cout << *it << std::endl;
  }
  return true;
}
