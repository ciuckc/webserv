#include "BlockDirective.h"

#include <memory>

#include "IDirective.h"

void BlockDirective::setDirectiveName(const std::string& name) {
  name_ = name;
}

void BlockDirective::addDirectiveArg(const std::string& arg) {
  args_.push_back(arg);
}

const std::string& BlockDirective::getDirectiveName() const {
  return name_;
}

const std::vector<std::string>& BlockDirective::getDirectiveArgs() const {
  return args_;
}
