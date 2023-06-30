#include "SimpleDirective.h"

#include <string>
#include <vector>

#include "IDirective.h"

void SimpleDirective::setDirectiveName(const std::string& name) {
  name_ = name;
}

void SimpleDirective::addDirectiveArg(const std::string& arg) {
  args_.push_back(arg);
}

const std::string& SimpleDirective::getDirectiveName() const {
  return name_;
}

const std::vector<std::string>& SimpleDirective::getDirectiveArgs() const {
  return args_;
}
