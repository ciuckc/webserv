#pragma once

#include <sys/epoll.h>

#include <ostream>
#include <chrono>

class Server;

class EventQueue {
 public:
  EventQueue();
  ~EventQueue();
  EventQueue(const EventQueue& other) = delete;
  EventQueue& operator=(const EventQueue& rhs) = delete;

  using event_t = struct epoll_event;
  using timep_t = std::chrono::system_clock::time_point;
  using filt_t = uint32_t;
  using flag_t = uint32_t;

  static constexpr filt_t in = EPOLLIN;
  static constexpr filt_t out = EPOLLOUT;
  static constexpr filt_t both = EPOLLIN | EPOLLOUT;
  static constexpr flag_t err = EPOLLERR;
  static constexpr flag_t w_hup = EPOLLHUP; // Connection is completely closed
  static constexpr flag_t r_hup = EPOLLRDHUP; // Peer closed their side of connection

  /**
   * Add a new file descriptor we want events for
   * @param fd The file descriptor
   * @param direction The direction we want events for
   */
  void add(int fd, uint32_t idx, filt_t direction = in) const;
  /**
   * Change the direction we want events for
   * @param fd The file descriptor we want to modify
   * @param new_direction The new direction we want events for
   */
  void mod(int fd, uint32_t idx, filt_t new_direction = both) const;
  /**
   * Remove a certain file descriptor
   * @param fd
   */
  void del(int fd) const;

  event_t& getNext(Server& server);

  static inline uint32_t getIndex(const event_t& ev) {
    return ev.data.u32;
  };
  inline const timep_t& lastWait() const {
    return last_wait_;
  }

 private:
  static constexpr auto max_events = 64;

  int queue_fd_;

  event_t events_[max_events];
  int event_count_;
  int event_index_;
  timep_t last_wait_;

  static event_t create_event(uint32_t idx, filt_t direction);
};

inline std::ostream& operator<<(std::ostream& out, const EventQueue::event_t& ev) {
  out << "EVENT:\t";

  static constexpr auto filt_n = 5;
  static constexpr std::pair<EventQueue::filt_t, const char*> filters[filt_n] = {
      {EventQueue::in, "IN"}, {EventQueue::out, "OUT"}, {EventQueue::err, "ERR"},
      {EventQueue::w_hup, "HUP"}, {EventQueue::r_hup, "RDHUP"}
  };
  bool first = true;
  for (const auto& filter : filters) {
    if (ev.events & filter.first) {
      if (first)
        first = false;
      else
        out << " | ";
      out << filter.second;
    }
  }
  return out << " (" << ev.events << ")\tfd: " << ev.data.fd;
}
