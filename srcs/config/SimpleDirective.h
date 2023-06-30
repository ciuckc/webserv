#pragma once

#include <string>
#include <vector>

#include "IDirective.h"

class SimpleDirective : public IDirective {
 public:
  SimpleDirective() = default;
  SimpleDirective(const SimpleDirective& rhs) = default;
  SimpleDirective& operator=(const SimpleDirective& rhs) = default;
  SimpleDirective(SimpleDirective&& rhs) noexcept = default;
  SimpleDirective& operator=(SimpleDirective&& rhs) noexcept = default;
  ~SimpleDirective() = default;

  void setDirectiveName(const std::string& name) override;
  void addDirectiveArg(const std::string& arg) override;
  const std::string& getDirectiveName() const override;
  const std::vector<std::string>& getDirectiveArgs() const override;

 private:
  std::string name_;
  std::vector<std::string> args_;
};
