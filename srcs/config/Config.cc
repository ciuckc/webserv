#include "Config.h"

#include <cstddef>
#include <memory>
#include <vector>

#include "BlockDirective.h"

void Config::addServerBlock(std::unique_ptr<BlockDirective> server_block) {
  server_blocks_.push_back(std::move(server_block));
}

std::vector<std::unique_ptr<BlockDirective> >::iterator Config::begin() {
  return server_blocks_.begin();
}

std::vector<std::unique_ptr<BlockDirective> >::iterator Config::end() {
  return server_blocks_.end();
}

size_t Config::vServerCount() const {
  return server_blocks_.size();
}
