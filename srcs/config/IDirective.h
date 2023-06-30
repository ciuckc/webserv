#pragma once

#include <string>

class IDirective {
 public:
  virtual void setDirectiveName(const std::string& name) = 0;
  virtual void addDirectiveArg(const std::string& arg) = 0;
  virtual const std::string& getDirectiveName() const = 0;
  virtual const std::vector<std::string>& getDirectiveArgs() const = 0;
  virtual ~IDirective() = default;
};
