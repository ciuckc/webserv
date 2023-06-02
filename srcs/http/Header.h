#pragma once
#include <string>
#include <vector>

struct Header {
 public:
  std::string key_;
  std::vector<std::string> values_;

  explicit Header(const std::string& key);

  bool operator==(const Header& rhs) const;
  bool operator==(const std::string& rhs) const;
};
