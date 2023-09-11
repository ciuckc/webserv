#pragma once

#include <string>
#include <fstream>
#include <unordered_map>

static struct MIME {
 public:
  const std::string& getType(const std::string& extension) {
    static const std::string empty;
    if (!have_read_file_)
      readTypes();
    if (types_.empty())
      return empty;
    auto it = types_.find(extension);
    if (it == types_.end())
      return empty;
    return it->second;
  }
 private:
  std::unordered_map<std::string, std::string> types_;
  bool have_read_file_ = false;

  void readTypes() {
    have_read_file_ = true;

    std::ifstream mime_stream(WS::mimes_file);
    std::string line;
    while (std::getline(mime_stream, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      size_t key_len = line.find_first_of(" \t");
      if (key_len == std::string::npos) // just key
        continue;
      std::string mime = line.substr(0, key_len);

      size_t val_end = key_len;
      size_t val_start;
      while ((val_start = line.find_first_not_of(" \t", val_end)) != std::string::npos) {
        val_end = line.find_first_of(" \t", val_start);
        size_t len = val_end == std::string::npos ? std::string::npos : val_end - val_start;
        types_[line.substr(val_start, len)] = mime;
        if (val_end == std::string::npos)
          break;
      }
    }
  }
} MIME;
