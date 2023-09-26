#pragma once
#include <array>
#include <string_view>

namespace HTTP {
enum Method {
  INVALID,
  GET,
  POST,
  PUT,
  DELETE,
  TOTAL_METHODS
};

static inline Method parseMethod(const std::string_view& word) {
  constexpr static std::string_view methods[TOTAL_METHODS] = {
      "INVALID",
      "GET",
      "POST",
      "PUT",
      "DELETE"
  };
  for (auto i = INVALID + 1; i < TOTAL_METHODS; ++i)
    if (word == methods[i])
      return static_cast<Method>(i); // wtf c++
  return INVALID;
}
}