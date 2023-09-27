#pragma once
#include <exception>
#include <fstream>
#include <map>
#include <string>
#include <utility>

#include "Config.h"
#include "ConfigFile.h"
#include "config/ConfigRoute.h"
#include "config/ConfigServer.h"

/**
 * @brief Parser and generator of the Config class
 *
 * This will remove the WS then split it on the special symbols.
 * Then it will attempt to parse it according to the BNF rules.
 */
class ConfigParse {
 public:
  using Lines = std::vector<std::string>;
  using Tokens = std::vector<std::string_view>;
  using TokensConstIter = Tokens::const_iterator;
  using TokensIter = Tokens::iterator;
  using DirectiveFuncPtr = bool (ConfigParse::*)(TokensConstIter&, const TokensConstIter&, ConfigServer&);
  using DirectiveFuncMap = std::map<std::string_view, DirectiveFuncPtr>;
  using LocDirectiveFuncPtr = bool (ConfigParse::*)(TokensConstIter&, const TokensConstIter&, ConfigRoute&);
  using LocDirectiveMap = std::map<std::string_view, LocDirectiveFuncPtr>;

 public:
  class InvalidDirective : public std::exception {
   public:
    explicit InvalidDirective(std::string argument = "") noexcept : reason_(std::move(argument)){};
    const char* what() const noexcept override;

   private:
    std::string reason_;
  };

 public:
  explicit ConfigParse(const Lines& file_data);
  ConfigParse(const ConfigParse&) = default;
  ~ConfigParse() = default;

  Config& parse(Config& cfg);

 private:
  void splitOnWhiteSpace(const Lines& lines, Tokens& tokens);
  void splitOnSymbols(Tokens& tokens);
  Config& semanticParse(const Tokens& tokens, Config& cfg);

  bool isServerDirective(const TokensConstIter& curr);
  bool isLocationDirective(const TokensConstIter& curr);

  template <typename T, typename Map>
  bool dispatchFunc(TokensConstIter& curr, const TokensConstIter&, T& cfg, const Map& map);

  // aCtUaL pArSiNg
  bool serverParse(TokensConstIter& curr, const TokensConstIter& end, Config& cfg);
  bool directivesParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);

  // Server parser
  bool locationParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);
  bool listenParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);
  bool serverNameParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);
  bool clientMaxBodySizeParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);
  bool errorPageParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);

  // Location parser
  bool indexParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& location);
  bool autoIndexParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& location);
  bool rootParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& location);
  bool allowedMethodsParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& location);
  bool redirectParse(TokensConstIter& curr, const TokensConstIter& end, ConfigRoute& location);

  const Lines& lines_;
  DirectiveFuncMap map_;
  LocDirectiveMap loc_map_;
};
