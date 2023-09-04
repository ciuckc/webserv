#pragma once

#include <string>
#include <cstring>
namespace Str {

template<typename... Strs> std::string join(Strs... str) {
  size_t total_len = (0 + ... + std::string_view(str).size());
  std::string ret;
  ret.reserve(total_len);
  (ret.append(str), ...);
  return ret;
}
}