#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <map>
#include <list>

static struct MIME {
 public:
  std::string_view getType(std::string_view extension) {
    if (!have_read_file_)
      readTypes();
    if (extension.empty() || mimes_.empty())
      return {};
    auto it = mimes_.find(extension);
    if (it == mimes_.end())
      return {};
    return it->second;
  }
 private:
  std::map<std::string, std::string_view, std::less<>> mimes_;
  std::list<std::string> types_; // so we don't need to duplicate types
  bool have_read_file_ = false;

  static bool skip_ws(std::string_view& view) {
    size_t ws_end = view.find_first_not_of(" \t");
    if (ws_end == std::string::npos)
      return false;
    view.remove_prefix(ws_end);
    return true;
  }
  void readTypes() {
    have_read_file_ = true;

    std::ifstream mime_stream(WS::mimes_file); // french clowns
    std::string line;
    while (std::getline(mime_stream, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      size_t mime_len = line.find_first_of(" \t");
      if (mime_len == std::string::npos) // just key
        continue;
      std::string_view mime = types_.emplace_back(line, 0, mime_len);
      std::string_view sv(line);
      sv.remove_prefix(mime_len);
      while (skip_ws(sv)) {
        size_t len = sv.find_first_of(" \t");
        mimes_.emplace(std::string(sv.substr(0, len)), mime);
        if (len == std::string::npos)
          break;
        sv.remove_prefix(len);
      }
    }
  }
} MIME;
