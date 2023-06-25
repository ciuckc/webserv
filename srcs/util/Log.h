#pragma once
#include <iostream>

namespace Log {
enum Level {
  ERROR,
  WARN,
  INFO,
  DEBUG,
  TRACE
};

static constexpr Level log_level = DEBUG;

template<Level lvl>
using log_enable_t = typename std::enable_if<(lvl <= log_level), void>::type;
template<Level lvl>
using log_disable_t = typename std::enable_if<(lvl > log_level), void>::type;

// Using the info/warn/debug/error/trace functions may be easier, but info is default
template<Level lvl = INFO, typename... Ts>
static inline log_enable_t<lvl> log(const Ts&... args) {
  if (log_level < lvl)
    return;
  constexpr std::ostream& out = (lvl == ERROR || lvl == WARN) ? std::cerr : std::cout;
  // This looks and feels dirty... The , is the comma operator so this just creates an empty array
  // but prints all arguments as a byproduct
  __attribute__((unused)) int unused[] = {((void) (out << args), 0)...};
}

// noop
template<Level lvl = INFO, typename... Ts>
static inline log_disable_t<lvl> log(const Ts&...) {}

template<typename... Ts> static inline void error(const Ts&... args) { log<ERROR>(args...); }
template<typename... Ts> static inline void warn(const Ts&... args) { log<WARN>(args...); }
template<typename... Ts> static inline void info(const Ts&... args) { log<INFO>(args...); }
template<typename... Ts> static inline void debug(const Ts&... args) { log<DEBUG>(args...); }
template<typename... Ts> static inline void trace(const Ts&... args) { log<TRACE>(args...); }
}
