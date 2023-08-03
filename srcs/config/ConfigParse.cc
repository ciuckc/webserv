#include "ConfigParse.h"

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

#include "ConfigFile.h"

namespace {
enum { EMPTY };

using tokens_t = std::vector<std::string>;

Config semantic_parse(tokens_t tokens) {
  for (auto it = tokens.begin(); it != tokens.end(); ++it) {}
  return Config();
}

}  // namespace

const char* ConfigParse::InvalidDirective::what() const throw() {
  return this->reason_.c_str();
}

ConfigParse::ConfigParse(const tokens_t& file_data) : tokens_(file_data) {}

Config ConfigParse::parse() {
  tokens_t tokens = split_on_white_space();
  tokens = split_on_symbols(tokens);
  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    std::cout << *it << std::endl;
  }
  Config cfg;
  try {
    cfg = semantic_parse(tokens);
  } catch (const InvalidDirective& e) {
    std::cerr << "Exception caught because of:" << e.what() << std::endl;
  }
  return cfg;
}

// Reading it now(01.08.2023) it sucks so bad how I implemented this.
// I should separate reading from manipulating the array (X.X)
// grrrrrr
tokens_t ConfigParse::split_on_white_space(const tokens_t& tokens) {
  struct StrPos {
    size_t start;
    size_t end;
  } str = {.start = 0, .end = 0};
  constexpr const char* white_space = "\t ";
  tokens_t new_tokens;

  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    std::string buffer = it->substr(0, it->find_first_of("#"));
    if (buffer.empty()) {
      continue;
    }
    str.start = 0;
    str.end = buffer.find_first_not_of(white_space, str.start);
    while (str.end != buffer.npos) {
      str.start = buffer.find_first_not_of(white_space, str.end);
      str.end = buffer.find_first_of(white_space, str.start);
      new_tokens.emplace_back(buffer.substr(str.start, str.end - str.start));
    }
  }
  return tokens;
}

tokens_t ConfigParse::split_on_symbols(const tokens_t& tokens) {
  tokens_t lexemes;
  constexpr const char* delimiters = "{}=;";

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
