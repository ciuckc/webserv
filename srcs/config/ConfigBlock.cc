#include "ConfigBlock.h"

#include <string>
#include <vector>

ConfigBlock::ConfigBlock() : directive_name_(), args_(), blocks_() {}

ConfigBlock::ConfigBlock(const ConfigBlock& rhs)
    : directive_name_(rhs.directive_name_), args_(rhs.args_), blocks_(rhs.blocks_) {}

ConfigBlock::~ConfigBlock() {}

ConfigBlock& ConfigBlock::operator=(const ConfigBlock& rhs) {
  if (this == &rhs)
    return *this;
  this->directive_name_ = rhs.directive_name_;
  this->args_ = rhs.args_;
  this->blocks_ = rhs.blocks_;
  return *this;
}

void ConfigBlock::setDirectiveName(const std::string& directive_name) {
  this->directive_name_ = directive_name;
}

void ConfigBlock::setArgs(const std::vector<std::string>& args) {
  this->args_ = args;
}

void ConfigBlock::addConfigBlock(const ConfigBlock& new_block) {
  this->blocks_.push_back(new_block);
}
