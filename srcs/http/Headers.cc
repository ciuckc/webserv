#include "Headers.h"

#include <istream>

Headers::Headers() : headers_() {}

Headers::~Headers() = default;

Headers::Headers(const Headers& other) : headers_(other.headers_.begin(), other.headers_.end()) {}

Headers& Headers::operator=(const Headers& rhs) {
  if (this == &rhs)
    return *this;
  headers_ = std::vector<Header>(rhs.headers_.begin(), rhs.headers_.end());
  return *this;
}

Headers::iterator Headers::find(const std::string& key) {
  iterator i;

  for (i = headers_.begin(); i < headers_.end(); i++)
    if (i->keyMatches(key))
      break;
  return i;
}

Headers::iterator Headers::begin() {
  return headers_.begin();
}

Headers::iterator Headers::end() {
  return headers_.end();
}

Headers::citerator Headers::cbegin() const {
  return headers_.begin();
}

Headers::citerator Headers::cend() const {
  return headers_.end();
}

Headers::iterator Headers::find_or_create(const std::string& key) {
  iterator found = find(key);
  if (found != headers_.end())
    return found;
  return headers_.insert(headers_.end(), Header(key));
}

Headers::Header::Header(const std::string& key) : key_(key), values_() {
  for (size_t i = 0; i < key_.length(); ++i)
    key_[i] = static_cast<char>(std::tolower(key[i]));
}

bool Headers::Header::keyMatches(const std::string& key) const {
  if (key.length() != key_.length())
    return false;
  for (size_t i = 0; i < key.length(); ++i)
    if (key_[i] != std::tolower(key[i]))
      return false;
  return true;
}

static void parseValues(Headers::Header& header, const std::string& line, size_t start) {
  size_t end;
  do {
    if (line[start] == '\r')
      return;

    start = line.find_first_not_of(" \t", start);
    end = line.find_first_of(",\r", start);

    size_t len = (end == std::string::npos) ? end : end - start;
    header.values_.push_back(line.substr(start, len));
    start = end + 1;
  } while (end < line.length() - 1);
}

void Headers::parse(std::istream& in) {
  iterator header = headers_.end();

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line == "\r")
      break;

    size_t colon = line.find(':');
    bool continuation = (colon == std::string::npos);
    if (!continuation) {
      header = find_or_create(line.substr(0, colon));
    } else if (header == headers_.end() || line.find_first_not_of(" \t\r") == std::string::npos) {
      throw std::exception();  // Invalid header
    }

    parseValues(*header, line, continuation ? 0 : colon + 1);
  }
}

std::ostream& operator<<(std::ostream& out, const Headers& headers) {
  for (Headers::citerator i = headers.cbegin(); i < headers.cend(); ++i) {
    out << i->operator std::string();
  }
  out << "\r\n";
  return out;
}
