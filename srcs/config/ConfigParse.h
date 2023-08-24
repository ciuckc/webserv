#pragma once
#include <exception>
#include <fstream>
#include <map>
#include <string>

#include "Config.h"
#include "ConfigFile.h"

/**
 * @brief Parser and generator of the Config class
 *
 * This will remove the WS then split it on the special symbols.
 * Then it will attempt to parse it according to the BNF rules.
 */
class ConfigParse {
 public:
  using Tokens = std::vector<std::string>;
  using TokensConstIter = Tokens::const_iterator;
  using TokensIter = Tokens::iterator;
  using FunctionPointer = bool (ConfigParse::*)(TokensConstIter&, const TokensConstIter&, ConfigServer&);
  using DispatchFuncMap = std::map<std::string, FunctionPointer>;

 public:
  class InvalidDirective : public std::exception {
   public:
    InvalidDirective(const std::string argument = "") throw() : reason_(argument){};
    const char* what() const throw() override;

   private:
    std::string reason_;
  };

 public:
  explicit ConfigParse(const Tokens& file_data);
  ConfigParse(const ConfigParse&) = default;
  ConfigParse& operator=(const ConfigParse&) = default;
  ~ConfigParse() = default;

  Config parse();

 private:
  Tokens splitOnWhiteSpace(const Tokens& tokens);
  Tokens splitOnSymbols(const Tokens& tokens);
  Config semanticParse(const Tokens& tokens);
  bool isDirective(const TokensConstIter& curr);
  // aCtUaL pArSiNg
  bool serverParse(TokensConstIter& curr, const TokensConstIter end, Config& cfg);
  bool directivesParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);
  bool dispatchDirectiveParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);
  bool listenParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);
  bool serverNameParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);
  bool rootParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);
  bool clienMaxBodySizeParse(TokensConstIter& curr, const TokensConstIter& end, ConfigServer& cfg_server);

  Tokens tokens_;
  DispatchFuncMap map_;
};
