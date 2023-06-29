#pragma once

// often used stuff
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
}  // namespace WS