#include "ConfigParse.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Config.h"
#include "ConfigFile.h"
#include "ConfigServer.h"
#include "util/Log.h"

namespace {
enum { EMPTY };

bool addressParse(const std::string& address) {
  constexpr std::string::size_type kMaxAddressLength = 15;
  if (address.size() > kMaxAddressLength || address.size() < 7) {
    Log::error("Invalid address size.\n");
    return false;
  }
  {
    size_t dot_count = 0;
    for (auto it = address.begin(); it != address.end(); ++it) {
      if (*it == '.') {
        dot_count++;
      }
    }
    if (dot_count != 3) {
      Log::error("Invalid number of address separators.\n");
      return false;
    }
    if (address.find_first_not_of(".0123456789") != std::string::npos) {  // if there are any other chars then we bail
      Log::error("Found unexpected character in address directive.\n");
      return false;
    }
  }
  {
    std::istringstream s_address(address);
    std::string number;
    std::vector<std::string> numbers;
    for (; std::getline(s_address, number, '.');) {
      if (!number.empty())
        numbers.push_back(number);
    }
    if (numbers.size() != 4) {
      Log::error("Invalid number of address fields.\n");
      return false;
    }
    for (auto number : numbers) {
      auto byte = std::stoul(number);
      if (byte > 255) {
        Log::error("Invalid address field. It must be in the range of 0 to 255.\n");
        return false;
      }
    }
  }
  return true;
}

bool portParse(const std::string& port) {
  if (port.size() > 5 || port.size() < 1) {
    Log::error("Port number size is invalid.\n");
    return false;
  }
  if (port.find_first_not_of("0123456789") != std::string::npos) {
    Log::error("Invalid character found in port number.\n");
    return false;
  }
  auto num = std::stoul(port);
  if ((num != 80 && num != 8080) && (num > std::numeric_limits<uint16_t>::max() || num < 49152)) {
    Log::error("Port must be 80 or 8080 or be in range of 49152 to 65535.\n");
    return false;
  }
  return true;
}

bool endpointParse(const std::string str, SocketAddress& endpoint) {
  auto idx = str.find(":");
  if (idx == str.npos) {
    Log::error("Invalid address/port.\n");
    return false;
  }
  std::string address;
  std::string port;
  try {
    address = str.substr(0, idx);
    port = str.substr(idx + 1);
  } catch (const std::out_of_range& e) {
    Log::error("Substr function in endpoint parser failed.\n");
    return false;
  }
  if (!addressParse(address) || !portParse(port)) {
    Log::error("Invalid listen argument.\n");
    return false;
  }
  endpoint.port = port;
  endpoint.address = address;
  return true;
}
}  // namespace

const char* ConfigParse::InvalidDirective::what() const throw() {
  return this->reason_.c_str();
}

ConfigParse::ConfigParse(const Tokens& file_data) : tokens_(file_data), map_() {
  this->map_ = {{"listen", &ConfigParse::listenParse},
                {"server_name", &ConfigParse::serverNameParse},
                {"root", &ConfigParse::rootParse},
                {"client_max_body_size", &ConfigParse::clientMaxBodySizeParse}};
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
      {"listen", true},   {"server_name", true}, {"root", true},
      {"location", true}, {"error_page", true},  {"client_max_body_size", true},
      {"index", true},    {"autoindex", true},   {"allowed_methods", true}};
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
  (void)cfg_server;
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end of \"listen\" directive.\n");
    return false;
  }
  SocketAddress endpoint;
  if (!endpointParse(*curr, endpoint)) {
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
  cfg_server.setListen(endpoint);
  return true;
}

bool ConfigParse::serverNameParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in server_name directive.\n");
    return false;
  }
  std::string name = *curr;
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in server_name directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"listen\" directive.\n");
    return false;
  }
  cfg_server.setServerName(name);
  return true;
}

bool ConfigParse::rootParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
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
  cfg_server.setRoot(new_root);
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
  // TODO: watch how big a body size can get. If the body size is bigger then what was set 413 is set. if
  // size is set to 0, the checking gets disabled.
  std::size_t number = std::stoull(*curr);
  if (number > 1024000000)
    return false;
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
