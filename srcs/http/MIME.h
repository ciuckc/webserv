#pragma once

#include <map>
#include <string>
#include <fstream>
namespace http {
  static struct MIME {
   public:
    std::string_view getType(const std::string& extension) {
      if (!have_read_file_)
        readTypes();
      if (types_.empty())
        return unknown_mime;
      auto it = types_.find(extension);
      if (it == types_.end())
        return unknown_mime;
    }
   private:
    static constexpr const char* unknown_mime = "";

    bool have_read_file_ = false;
    std::map<std::string, std::string> types_;
    void readTypes() {
      have_read_file_ = true;

      std::ifstream mime_stream("/etc/mime.types");
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
          types_.emplace(line.substr(val_start, val_end - val_start + 1), mime);
          if (val_end == std::string::npos)
            break;
        }
      }
    }
  } mime;
}