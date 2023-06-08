#include "ConfigParse.h"

#include <exception>
#include <ios>
#include <iostream>
#include <string>

ConfigParse::ConfigParse() {}

ConfigParse::ConfigParse(const std::string& file_name_)
    : config_file_name_(file_name_.c_str(), std::ios_base::in) {
  if (!config_file_name_.is_open())
    throw std::exception();
}

ConfigParse::~ConfigParse() {}

bool ConfigParse::parse(Config& config) {
  (void)config;
  {}
  return true;
}
