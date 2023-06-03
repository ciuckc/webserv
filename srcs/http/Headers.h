#pragma once
#include <string>
#include <vector>

class Headers {
 public:
  struct Header {
    std::string key_;
    std::vector<std::string> values_;

    explicit Header(const std::string& key);
    bool keyMatches(const std::string& key) const;
  };

 private:
  std::vector<Header> headers_;

 public:
  Headers();
  ~Headers();
  Headers(const Headers& other);
  Headers& operator=(const Headers& rhs);

  typedef std::vector<Header>::iterator iterator;
  typedef std::vector<Header>::const_iterator citerator;

  iterator begin();
  iterator end();
  citerator cbegin() const;
  citerator cend() const;

  iterator find(const std::string& key);
  iterator find_or_create(const std::string& key);

  void parse(std::istream& in);
};

std::ostream& operator<<(std::ostream& out, const Headers& headers);
