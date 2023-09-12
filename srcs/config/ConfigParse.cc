#include "ConfigParse.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Config.h"
#include "ConfigServer.h"
#include "util/Log.h"

namespace {
enum { EMPTY };

bool isPositiveInt(const std::string& str) {
  if (str.find_first_not_of("0123456789") != std::string::npos) {
    return false;
  }
  if (str.size() > 10) {
    return false;
  }
  auto num = std::stoul(str);
  if (num > std::numeric_limits<int>::max()) {
    return false;
  }
  return true;
}

bool portParse(const std::string& portstr, uint16_t& port) {
  if (portstr.size() > 5 || portstr.empty()) {
    Log::error("Port number size is invalid.\n");
    return false;
  }
  if (portstr.find_first_not_of("0123456789") != std::string::npos) {
    Log::error("Invalid character found in port number.\n");
    return false;
  }
  auto num = std::stoul(portstr);
  if ((num != 80 && num != 8080) && (num > std::numeric_limits<uint16_t>::max() || num < 49152)) {
    Log::error("Port must be 80 or 8080 or be in range of 49152 to 65535.\n");
    return false;
  }
  port = num;
  return true;
}
}  // namespace

const char* ConfigParse::InvalidDirective::what() const throw() {
  return this->reason_.c_str();
}

ConfigParse::ConfigParse(const Tokens& file_data) : tokens_(file_data), map_() {
  this->map_ = {{"listen", &ConfigParse::listenParse},
                {"server_name", &ConfigParse::serverNameParse},
                {"client_max_body_size", &ConfigParse::clientMaxBodySizeParse},
                {"error_page", &ConfigParse::errorPageParse},
                {"location", &ConfigParse::locationParse}};
}

Config ConfigParse::parse() {
  this->tokens_ = splitOnWhiteSpace(this->tokens_);
  this->tokens_ = splitOnSymbols(this->tokens_);
  if (this->tokens_.empty())
    throw std::invalid_argument("Configuration file is empty");
  try {
    return semanticParse(this->tokens_);
  } catch (const InvalidDirective& e) {
    Log::error("Exception caught:", e.what(), "\n");
    throw std::exception();
  }
}

Config ConfigParse::semanticParse(const Tokens& tokens) {
  Config cfg;

  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    if (!serverParse(it, tokens.end(), cfg)) {
      throw ConfigParse::InvalidDirective("Invalid config");
    }
  }
  return cfg;
}

bool ConfigParse::isDirective(const TokensConstIter& it) {
  std::unordered_map<std::string, bool> directives = {
      {"listen", true}, {"server_name", true}, {"location", true}, {"error_page", true}, {"client_max_body_size", true},
  };
  return directives[*it];
}

bool ConfigParse::serverParse(TokensConstIter& curr, const TokensConstIter end, Config& cfg) {
  if (*curr != "server") {
    Log::error("Expecting \"server\" directive.\n");
    return false;
  }
  ++curr;
  if (curr == end) {
    Log::error("Expecting \"{\" in server block.\n");
    return false;
  }
  if (*curr != "{") {
    Log::error("Expecting \"{\" in server block.\n");
    return false;
  }
  ++curr;
  if (curr == end) {
    Log::error("Expecting \"}\" in server block.\n");
    return false;
  }
  ConfigServer cfg_server;
  if (!directivesParse(curr, end, cfg_server))
    return false;
  if (curr == end)
    return false;
  if (*curr != "}") {
    Log::error("Expecting \"}\" in server block.\n");
    return false;
  }
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
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end of \"listen\" directive.\n");
    return false;
  }
  uint16_t port;
  if (!portParse(*curr, port)) {
    Log::error("Invalid listen argument.\n");
    return false;
  }
  curr++;
  if (curr == end) {
    Log::error("Unexpected end of \"listen\" directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"listen\" directive.\n");
    return false;
  }
  cfg_server.setPort(port);
  return true;
}

bool ConfigParse::serverNameParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in server_name directive.\n");
    return false;
  }
  do {
    const std::string& name = *curr;
    ++curr;
    if (curr == end) {
      Log::error("Unexpected end in server_name directive.\n");
      return false;
    }
    cfg_server.addServerName(name);
  } while (*curr != ";");
  return true;
}

bool ConfigParse::rootParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  (void)cfg_server;
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in root directive.\n");
    return false;
  }
  std::string new_root = *curr;  // I am not sure if I need to make sure if the root is a valid path?
  ++curr;                        // Probs it s the responsibility of the user?
  if (curr == end) {
    Log::error("Unexpected end in root directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"root\" directive.\n");
    return false;
  }
  // cfg_server.setRoot(new_root); // todo: this needs to go to
  return true;
}

bool ConfigParse::clientMaxBodySizeParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in client_max_body_size directive.\n");
    return true;
  }
  if (curr->find_first_not_of("0123456789") != std::string::npos) {
    Log::error("Unexpected character in client_max_body_size directive argument.\n");
    return false;
  }
  std::size_t number = std::stoull(*curr);
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in client_max_body_size directive.\n");
    return true;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"client_max_body_size\" directive.\n");
    return false;
  }
  cfg_server.setClientBodyMaxSize(number);
  return true;
}

bool ConfigParse::errorPageParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in \"error_page\" directive.\n");
    return false;
  }
  std::vector<int> error_codes{};
  for (; curr != end && isPositiveInt(*curr); ++curr) {
    auto number = static_cast<int>(std::stoul(*curr));
    if (number < 300 || number > 599) {
      Log::error("Value in \"error_page\" directive must be between 300 and 599.\n");
      return false;
    }
    error_codes.emplace_back(number);
  }
  if (curr == end) {
    Log::error("Unexpected end in \"error_page\" directive.\n");
    return false;
  }
  if (!error_codes.size()) {
    Log::error("No error codes provided.\n");
    return false;
  }
  for (auto code : error_codes) {
    cfg_server.addErrorPage(code, *curr);
  }
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"error_page\" directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"error_page\" directive.\n");
    return false;
  }
  return true;
}

bool ConfigParse::indexParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"index\" directive.\n");
    return false;
  }
  std::vector<std::string> files{};
  for (; curr != end && *curr != ";"; ++curr) {
    for (auto file : files) {
      if (*curr == file) {
        Log::error("Can not have the same index file multiple times.\n");
        return false;
      }
    }
    files.emplace_back(*curr);
  }
  if (curr == end) {
    Log::error("Unexpected end in \"index\" directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"error_page\" directive.\n");
    return false;
  }
  cfg_server.addIndexFiles(files);
  return true;
}

bool ConfigParse::autoIndexParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"autoindex\" directive.\n");
    return false;
  }
  if (*curr != "true" && *curr != "false") {
    Log::error("Value of \"autoindex\" directive must be either true or false.\n");
    return false;
  }
  bool autoindex{*curr == "true" ? true : false};
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"index\" directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"autoindex\" directive.\n");
    return false;
  }
  cfg_server.setAutoIndex(autoindex);
  return true;
}

bool ConfigParse::locationParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer&) {
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"location\" directive.\n");
    return false;
  }
  std::string path = *curr;
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"location\" directive.\n");
    return false;
  }
  if (*curr != "{") {
    Log::error("Expected \"{\" in \"location\" directive.\n");
    return false;
  }
  for (; curr != end && *curr != "}"; ++curr) {}
  if (curr == end) {
    Log::error("Unexpected end in \"location\" directive.\n");
    return false;
  }
  if (*curr != "}") {
    Log::error("Expected \"}\" in \"location\" directive.\n");
    return false;
  }
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
