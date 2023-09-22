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
#include "config/ConfigRoute.h"
#include "util/Log.h"

namespace {

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
  if (num == 0 || num > std::numeric_limits<uint16_t>::max()) {
    Log::error("Port must be in range of 1 to 65535.\n");
    return false;
  }
  port = num;
  return true;
}
}  // namespace

const char* ConfigParse::InvalidDirective::what() const noexcept {
  return this->reason_.c_str();
}

ConfigParse::ConfigParse(const Lines& file_data) : lines_(file_data), map_(), loc_map_() {
  this->map_ = {{"listen", &ConfigParse::listenParse},
                {"server_name", &ConfigParse::serverNameParse},
                {"client_max_body_size", &ConfigParse::clientMaxBodySizeParse},
                {"error_page", &ConfigParse::errorPageParse},
                {"location", &ConfigParse::locationParse}};

  this->loc_map_ = {{"index", &ConfigParse::indexParse},       {"autoindex", &ConfigParse::autoIndexParse},
                    {"root", &ConfigParse::rootParse},         {"allowed_methods", &ConfigParse::allowedMethodsParse},
                    {"redirect", &ConfigParse::redirectParse}, {"cgi_upload_dir", &ConfigParse::uploadDirParse}};
}

Config& ConfigParse::parse(Config& cfg) {
  Tokens tokens;
  splitOnWhiteSpace(lines_, tokens);
  splitOnSymbols(tokens);
  if (lines_.empty())
    throw std::invalid_argument("Configuration file is empty");
  try {
    return semanticParse(tokens, cfg);
  } catch (const InvalidDirective& e) {
    Log::error("Exception caught: ", e.what(), "\n");
    std::exit(1);
  }
}

Config& ConfigParse::semanticParse(const Tokens& tokens, Config& cfg) {
  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    if (!serverParse(it, tokens.end(), cfg)) {
      throw ConfigParse::InvalidDirective("Invalid config");
    }
  }
  return cfg;
}

bool ConfigParse::isDirective(const TokensConstIter& curr) {
  std::unordered_map<std::string_view, bool> directives = {
      {"listen", true}, {"server_name", true}, {"location", true}, {"error_page", true}, {"client_max_body_size", true},
  };
  return directives[*curr];
}

bool ConfigParse::isLocationDirective(const TokensConstIter& curr) {
  std::unordered_map<std::string_view, bool> location_directives = {{"index", true},    {"autoindex", true},
                                                               {"root", true},     {"allowed_methods", true},
                                                               {"redirect", true}, {"cgi_upload_dir", true}};
  return location_directives[*curr];
}

template <typename T, typename Map>
bool ConfigParse::dispatchFunc(TokensConstIter& curr, const TokensConstIter& end, T& cfg, const Map& map) {
  auto function_pointer_it = map.find(*curr);
  if (function_pointer_it == map.end())
    return false;
  auto function_pointer = function_pointer_it->second;
  return (this->*function_pointer)(curr, end, cfg);
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
  ConfigServer cfg_server{};
  if (!directivesParse(curr, end, cfg_server)) {
    Log::error("Invalid synthax inside server directive.\n");
    return false;
  }
  if (curr == end) {
    Log::error("Unexpected end in \"server\" block.\n");
    return false;
  }
  if (*curr != "}") {
    Log::error("Expecting \"}\" in server block.\n");
    return false;
  }
  if (cfg_server.getPort() == 0) {
    Log::error("Missing \"listen\" field in server block.\n");
    return false;
  }
  cfg.addServer(std::move(cfg_server));
  return true;
}

bool ConfigParse::directivesParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  for (; curr != end && isDirective(curr); ++curr) {
    if (!dispatchFunc<ConfigServer, DirectiveFuncMap>(curr, end, cfg_server, map_)) {
      if (curr == end) {
        Log::error("Dispatch returned false for token [ NULL ]!\n");
        return false;
      }
      Log::error("Dispatch returned false for token [ ", *curr, " ]!\n");
      return false;
    }
  }
  return true;
}

bool ConfigParse::listenParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end of \"listen\" directive.\n");
    return false;
  }
  uint16_t port;
  if (!portParse(std::string(*curr), port)) {
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
    std::string name = std::string(*curr);
    ++curr;
    if (curr == end) {
      Log::error("Unexpected end in server_name directive.\n");
      return false;
    }
    cfg_server.addServerName(std::move(name));
  } while (*curr != ";");
  return true;
}

bool ConfigParse::clientMaxBodySizeParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in \"client_max_body_size\" directive.\n");
    return true;
  }
  if (curr->find_first_not_of("0123456789") != std::string::npos) {
    Log::error("Unexpected value [", *curr, "] in \"client_max_body_size\" directive.\n");
    return false;
  }
  std::size_t number = std::stoull(std::string(*curr));
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in \"client_max_body_size\" directive.\n");
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
  for (; curr != end && isPositiveInt(std::string(*curr)); ++curr) {
    auto number = static_cast<int>(std::stoul(std::string(*curr))); // todo: don't parse value multiple times
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
  if (*curr == ";") {
    Log::error("Expected path to error_page in \"error_page\" directive.\n");
    return false;
  }
  if (error_codes.empty()) {
    Log::error("No error codes provided in \"error_page\" directive.\n");
    return false;
  }
  for (auto code : error_codes) {
    cfg_server.addErrorPage(code, std::string(*curr));
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

bool ConfigParse::locationParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server) {
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"location\" directive.\n");
    return false;
  }
  auto path = *curr;
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"location\" directive.\n");
    return false;
  }
  if (*curr != "{") {
    Log::error("Expected \"{\" in \"location\" directive.\n");
    return false;
  }
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"location\" directive.\n");
    return false;
  }
  ConfigRoute new_route;
  for (; curr != end && isLocationDirective(curr); ++curr) {
    if (!dispatchFunc<ConfigRoute, LocDirectiveMap>(curr, end, new_route, loc_map_)) {
      return false;
    }
  }
  cfg_server.addRoute(std::string(path), std::move(new_route));
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

bool ConfigParse::indexParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& location) {
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"index\" directive.\n");
    return false;
  }
  std::vector<std::string_view> files;
  for (; curr != end && *curr != ";"; ++curr) {
    for (auto& file : files) {
      if (*curr == file) {
        Log::error("Can not have the same index file multiple times.\n");
        return false;
      }
    }
    files.emplace_back(*curr);
    location.addIndexFile(std::string(*curr));
  }
  if (curr == end) {
    Log::error("Unexpected end in \"index\" directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"index\" directive.\n");
    return false;
  }
  return true;
}

bool ConfigParse::autoIndexParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& lcoation) {
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"autoindex\" directive.\n");
    return false;
  }
  if (*curr != "true" && *curr != "false") {
    Log::error("Value of \"autoindex\" directive must be either true or false.\n");
    return false;
  }
  bool autoindex = *curr == "true";
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"autoindex\" directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"autoindex\" directive.\n");
    return false;
  }
  lcoation.setAutoIndex(autoindex);
  return true;
}

bool ConfigParse::rootParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& location) {
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in \"root\" directive.\n");
    return false;
  }
  location.setRoot(std::string(*curr));
  ++curr;
  if (curr == end) {
    Log::error("Unexpected end in \"root\" directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"root\" directive.\n");
    return false;
  }
  return true;
}

bool ConfigParse::allowedMethodsParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& location) {
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"allowed_methods\" directive.\n");
    return false;
  }
  for (; curr != end && *curr != ";"; ++curr) {
    if (*curr == "GET") {
      location.addAcceptedMethod(HTTP::GET);
    } else if (*curr == "DELETE") {
      location.addAcceptedMethod(HTTP::DELETE);
    } else if (*curr == "POST") {
      location.addAcceptedMethod(HTTP::POST);
    } else {
      Log::error("Unknown value in \"allowed_methods\" directive.\n");
      return false;
    }
  }
  if (curr == end) {
    Log::error("Unexpected end in \"allowed_methods\" directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"allowed_methods\" directive.\n");
    return false;
  }
  return true;
}

bool ConfigParse::redirectParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& location) {
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"redirect\" directive.\n");
    return false;
  }
  location.setRedirect(std::string(*curr));
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"redirect\" directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"redirect\" directive.\n");
    return false;
  }
  return true;
}

bool ConfigParse::uploadDirParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& location) {
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"upload_dir\" directive.\n");
    return false;
  }
  location.setUploadDir(std::string(*curr));
  curr++;
  if (curr == end) {
    Log::error("Unexpected end in \"upload_dir\" directive.\n");
    return false;
  }
  if (*curr != ";") {
    Log::error("Expected \";\" in \"upload_dir\" directive.\n");
    return false;
  }
  return true;
}

void ConfigParse::splitOnWhiteSpace(const Lines& lines, Tokens& tokens) {
  constexpr const char* white_space = "\t ";
  std::string_view cur;
  for (auto& line : lines) {
    cur = line;
    cur = cur.substr(0, cur.find_first_of('#'));
    size_t idx;
    while ((idx = cur.find_first_not_of(white_space)) != std::string::npos) {
      cur.remove_prefix(idx);
      size_t length = cur.find_first_of(white_space);
      tokens.push_back(cur.substr(0, length));
      if (length == std::string::npos)
        break;
      cur.remove_prefix(length);
    }
  }
}

void ConfigParse::splitOnSymbols(Tokens& tokens) {
  Tokens lexemes;
  constexpr const char* delimiters = "{};";

  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    size_t symbol_idx;
    while (it->size() > 1 && (symbol_idx = it->find_first_of(delimiters)) != std::string::npos) {
      if (symbol_idx != 0) {
        it = tokens.insert(it, it->substr(0, symbol_idx)) + 1;
        it->remove_prefix(symbol_idx);
      }
      if (it->size() > 1) {
        it = tokens.insert(it, it->substr(0, 1)) + 1;
        it->remove_prefix(1);
      }
    }
  }
}
