#include "ConfigParse.h"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "Config.h"
#include "ConfigFile.h"

namespace {
enum { EMPTY };
}  // namespace

const char* ConfigParse::InvalidDirective::what() const throw() {
  return this->reason_.c_str();
}

ConfigParse::ConfigParse(const Tokens& file_data) : tokens_(file_data), map_() {
  this->map_ = {{"listen", &ConfigParse::listenParse}, {"server_name", &ConfigParse::serverNameParse}};
}

Config ConfigParse::parse() {
  this->tokens_ = splitOnWhiteSpace(this->tokens_);
  this->tokens_ = splitOnSymbols(this->tokens_);
  for (auto token : this->tokens_) {
    std::cout << token << std::endl;
  }
  if (this->tokens_.empty())
    throw std::invalid_argument("Configuration file is empty");
  Config cfg;
  try {
    cfg = semanticParse(this->tokens_);
  } catch (const InvalidDirective& e) {
    std::cerr << "Exception caught because of:" << e.what() << std::endl;
    std::abort();
  }
  return cfg;
}

Config ConfigParse::semanticParse(const Tokens& tokens) {
  Config cfg;

  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    if (!serverParse(it, tokens.end(), cfg)) {
      throw ConfigParse::InvalidDirective("Invalid directive");
    }
  }
  return cfg;
}

bool ConfigParse::isDirective(const TokensConstIter& it) {
  std::unordered_map<std::string, bool> directives = {{"listen", true},     {"hostname", true},
                                                      {"root", true},       {"location", true},
                                                      {"error_page", true}, {"client_max_body_size", true},
                                                      {"index", true},      {"alias", true},
                                                      {"autoindex", true},  {"allowed_methods", true}};
  return directives[*it];
}

bool ConfigParse::serverParse(TokensConstIter& curr, const TokensConstIter end, Config& cfg) {
  if (*curr != "server")
    return false;
  ++curr;
  if (curr == end)
    return false;
  if (*curr != "{")
    return false;
  ++curr;
  if (curr == end)
    return false;
  ConfigServer cfg_server;
  if (!directivesParse(curr, end, cfg_server))
    return false;
  ++curr;
  if (curr == end)
    return false;
  if (*curr != "}")
    return false;
  cfg.addServer(cfg_server);
  return true;
}

bool ConfigParse::directivesParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  for (; curr != end && isDirective(curr); ++curr) {
    if (!dispatchDirectiveParse(curr, end, cfg_server))
      return false;
  }
  return true;
}

bool ConfigParse::dispatchDirectiveParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  auto function_pointer_it = map_.find(*curr);
  if (function_pointer_it == map_.end())
    return false;
  auto function_pointer = function_pointer_it->second;
  return (this->*function_pointer)(curr, end, cfg_server);
}

bool ConfigParse::listenParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  (void)cfg_server;
  ++curr;
  if (curr == end)
    return false;
  curr++;
  {}
  if (curr == end)
    return false;
  if (*curr != ";")
    return false;
  return true;
}

bool ConfigParse::serverNameParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  ++curr;
  if (curr == end)
    return false;
  std::string name = *curr;
  ++curr;
  if (curr == end)
    return false;
  if (*curr != ";")
    return false;
  cfg_server.addHostname(name);
  return true;
}

ConfigParse::Tokens ConfigParse::splitOnWhiteSpace(const Tokens& tokens) {
  struct StrPos {
    size_t start;
    size_t end;
  } str = {.start = 0, .end = 0};
  constexpr const char* white_space = "\t ";
  Tokens new_tokens;

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
  return new_tokens;
}

ConfigParse::Tokens ConfigParse::splitOnSymbols(const Tokens& tokens) {
  Tokens lexemes;
  constexpr const char* delimiters = "{};";

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
