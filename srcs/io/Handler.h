#pragma once

#include <chrono>

#include "Server.h"

class Handler {
 public:
  using filt_t = EventQueue::filt_t;
  using ms_t = std::chrono::milliseconds;
  using timep_t = std::chrono::system_clock::time_point;

  Handler(int fd, filt_t f, int timeout_ms);
  virtual ~Handler();

  bool handle(const EventQueue::event_t& event);
  virtual bool handleTimeout(Server& server, bool src) = 0;
  virtual void updateTimeout(const timep_t& wait);

  void enableFilter(EventQueue& evq, filt_t f);
  void updateFilter(EventQueue& evq);

  [[nodiscard]] virtual const std::string& getName() const = 0;
  [[nodiscard]] int getFD() const;
  [[nodiscard]] uint32_t getIndex() const;
  [[nodiscard]] filt_t getFilter() const;
  [[nodiscard]] virtual const timep_t& getExpiry() const;
  void setIndex(uint32_t idx);

 protected:
  int fd_;
  filt_t filter_;
  ms_t timeout_;
  timep_t expires_;

  virtual bool handleRead() = 0;
  virtual bool handleWrite() = 0;
  virtual bool handleWHup() = 0;
  virtual bool handleRHup() = 0;
  virtual bool handleError() = 0;

  void addFilter(filt_t f);
  void delFilter(filt_t f);

 private:
  uint32_t index_ = -1;
  filt_t prev_filter_ = 0;
};

inline std::ostream& operator<<(std::ostream& stream, const Handler& h) {
  return stream << h.getName() << "\t";
}
