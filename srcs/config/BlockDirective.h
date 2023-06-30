#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IDirective.h"

class BlockDirective : public IDirective {
 public:
  BlockDirective() = default;
  BlockDirective(const BlockDirective& rhs) = default;
  BlockDirective& operator=(const BlockDirective& rhs) = default;
  BlockDirective(BlockDirective&& rhs) noexcept = default;
  BlockDirective& operator=(BlockDirective&& rhs) noexcept = default;
  ~BlockDirective() = default;

  void setDirectiveName(const std::string& name);
  void addDirectiveArg(const std::string& arg);
  const std::string& getDirectiveName() const;
  const std::vector<std::string>& getDirectiveArgs() const;

 private:
  std::string name_;
  std::vector<std::string> args_;
  std::vector<std::unique_ptr<IDirective> > directives_;
};
