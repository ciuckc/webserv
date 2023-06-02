#include "Header.h"

Header::Header(const std::string& key): key_(key), values_() {}

bool Header::operator==(const Header& rhs) const {
  return key_ == rhs.key_;
}

bool Header::operator==(const std::string& rhs) const {
  return key_ == rhs;
}
