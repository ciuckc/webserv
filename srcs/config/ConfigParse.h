#pragma once
#include <exception>
#include <fstream>
#include <string>

#include "Config.h"
#include "ConfigFile.h"

class ConfigParse {
 public:
  using tokens_t = std::vector<std::string>;

 public:
  class InvalidDirective : public std::exception {
   public:
    InvalidDirective(const std::string argument = "") throw() : reason_(argument){};
    const char* what() const throw() override;

   private:
    std::string reason_;
  };

 public:
  explicit ConfigParse(const tokens_t& file_data);
  ConfigParse(const ConfigParse&) = default;
  ConfigParse& operator=(const ConfigParse&) = default;
  ~ConfigParse() = default;

  Config parse();

 private:
  tokens_t read_file();
  tokens_t split_on_white_space();
  tokens_t split_on_white_space(const tokens_t& tokens);
  tokens_t split_on_symbols(const tokens_t& tokens);

  tokens_t tokens_;
};
