#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "BlockDirective.h"

class Config {
 public:
  Config() = default;
  Config(const Config& rhs) = delete;
  Config& operator=(const Config& rhs) = delete;
  ~Config() = default;

  void addServerBlock(std::unique_ptr<BlockDirective> server_block);
  std::vector<std::unique_ptr<BlockDirective> >::iterator begin();
  std::vector<std::unique_ptr<BlockDirective> >::iterator end();
  size_t vServerCount() const;

 private:
  std::vector<std::unique_ptr<BlockDirective> > server_blocks_;
};
