#pragma once

#include <sys/stat.h>
#include <unistd.h>

// often used stuff
#include <ctime>

#include "io/IOException.h"

namespace WS {
enum IOStatus {
  IO_EOF = -3,
  IO_FAIL = -2, // IO error
  IO_BLOCKED = -1, // Something is not ready
  IO_AGAIN = 0, // Everything went ok, not done yet
  IO_GOOD = 1, // Everything went ok, done
};

// Maximum length of a http request (message and headers)
static constexpr auto request_maxlen = 32768;

// Max http header length (max length of a single line)
// this limit is not set by the rfc, 8k is the most common value I see
static constexpr auto header_maxlen = 8192;

static constexpr auto uri_maxlen = 8192;

// How many seconds will we keep idle connections alive for?
static constexpr auto timeout = 30;

// How many requests do we want to handle per connection?
static constexpr auto max_requests = 100;

// File containing mime types
static constexpr const char* mimes_file = "./mime.types";

static inline std::string get_date_header() {
  // strlen("date: Tue, 22 Aug 2023 19:39:33 GMT\r\n\0")
  static constexpr size_t date_header_len = 38;
  char out[date_header_len] = {};
  time_t time = std::time(nullptr);
  std::strftime(out, date_header_len, "date: %a, %d %b %Y %T GMT\r\n", gmtime(&time));
  return { out, date_header_len - 1 };
}

static const struct CaseCmpL {
  template<class T1, class T2> bool operator()(const T1& a, const T2& b) const noexcept {
    const size_t size_a = a.size(), size_b = b.size();
    if (size_a != size_b)
      return size_a < size_b;
    return strncasecmp(a.data(), b.data(), size_a) < 0;
  }
} case_cmp_less;
static const struct CaseCmpG {
  template<class T1, class T2> bool operator()(const T1& a, const T2& b) const noexcept {
    const size_t size_a = a.size(), size_b = b.size();
    if (size_a != size_b)
      return size_a > size_b;
    return strncasecmp(a.data(), b.data(), size_a) > 0;
  }
} case_cmp_greater;

}  // namespace WS

namespace util {
  // iT's A cOoPeRaTiVe PrOtOcOl!!1!
  static inline size_t find_header_end(const std::string& str)
  {
    size_t nn = str.find("\n\n");
    size_t rn = str.find("\r\n\r\n");
    return (std::min(nn, rn));
  }

  static inline std::string_view without_crlf(std::string_view sv) {
    sv.remove_suffix(1 + (sv[sv.size() - 2] == '\r'));
    return sv;
  }

  struct FileInfo {
    FileInfo() = default;
    struct stat s;

    bool open(const char* path) {
      return stat(path, &s) == 0;
    }
    bool isDir() const {
      return (s.st_mode & S_IFMT) == S_IFDIR;
    }
    bool isFile() const {
      return (s.st_mode & S_IFMT) == S_IFREG;
    }
    size_t size() const {
      return s.st_size;
    }
  };
  static inline std::string getExtension(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) {
      return {};
    }
    return path.substr(dot + 1);
  }

  static constexpr const char* RESET = "\033[0m";
  static constexpr const char* BLACK = "\033[30m";
  static constexpr const char* RED = "\033[31m";
  static constexpr const char* GREEN = "\033[32m";
  static constexpr const char* YELLOW = "\033[33m";
  static constexpr const char* BLUE = "\033[34m";
  static constexpr const char* MAGENTA = "\033[35m";
  static constexpr const char* CYAN = "\033[36m";
  static constexpr const char* WHITE = "\033[37m";
  static constexpr const char* terminal_colours[8] = {
      BLACK, RED, GREEN, YELLOW,
      BLUE, MAGENTA, CYAN, WHITE
  };
}
