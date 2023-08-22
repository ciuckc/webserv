#pragma once

// often used stuff
#include <ctime>

namespace WS {
enum Dir {
  IN = 1,
  OUT = 2,
  BOTH = 3
};
enum IOStatus {
  IO_GOOD,
  IO_WAIT,
  IO_FAIL
};

// Maximum length of a http request (message and headers)
static constexpr size_t request_maxlen = 32768;

// Max http header length (max length of a single line)
// this limit is not set by the rfc, 8k is the most common value I see
static constexpr size_t header_maxlen = 8192;

// How many seconds will we keep idle connections alive for?
static constexpr uint32_t timeout = 5;

// How many requests do we want to handle per connection?
static constexpr uint32_t max_requests = 10;

static inline std::string get_date_header() {
  // strlen("date: Mon, 01 Jan 0000 00:00:00 GMT\r\n") + 1;
  static constexpr size_t date_header_len = 37;
  char out[date_header_len] = {};
  time_t time = std::time(nullptr);
  std::strftime(out, date_header_len, "date: %a, %d %b %Y %T GMT\r\n", gmtime(&time));
  return { out, date_header_len };
}
}  // namespace WS