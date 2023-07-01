#include "ConfigParse.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>

namespace parse_utils {
enum { EMPTY };
std::vector<std::string> split_on_white_space(std::ifstream& config_file);
std::vector<std::string> split_on_symbols(const std::vector<std::string>& tokens);
}  // namespace parse_utils

ConfigParse::ConfigParse() {}

ConfigParse::ConfigParse(const std::string& file_name_) : config_file_(file_name_.c_str(), std::ios_base::in) {
  if (!config_file_.is_open())
    throw std::exception();
}

ConfigParse::~ConfigParse() {}

std::vector<std::string> parse_utils::split_on_white_space(std::ifstream& config_file) {
  std::string buffer;
  std::string token;
  std::vector<std::string> tokens;
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
  return tokens;
}

std::vector<std::string> parse_utils::split_on_symbols(const std::vector<std::string>& tokens) {
  std::vector<std::string> lexemes;
  const std::string delimiters = "{}=;";

  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    std::size_t symbol_idx = it->find_first_of(delimiters);
    if (symbol_idx == it->npos) {
      lexemes.push_back(*it);
      continue;
    }

    std::size_t start_idx = 0;
    while (symbol_idx != it->npos) {
      if (start_idx < symbol_idx) {
        lexemes.push_back(it->substr(start_idx, symbol_idx - start_idx));
      }
      lexemes.push_back(it->substr(symbol_idx, 1));
      start_idx = symbol_idx + 1;
      symbol_idx = it->find_first_of(delimiters, start_idx);
    }
    if (start_idx < it->length()) {
      lexemes.push_back(it->substr(start_idx));
    }
  }
  return lexemes;
}

bool ConfigParse::parse(Config& config) {
  (void)config;
  std::vector<std::string> tokens = parse_utils::split_on_white_space(config_file_);
  if (tokens.size() == parse_utils::EMPTY) {
    std::cout << "Configuration file is empty!" << std::endl;
    exit(1);
  }
  tokens = parse_utils::split_on_symbols(tokens);
  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    std::cout << *it << std::endl;
  }
  return true;
}
