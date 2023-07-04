#pragma once
#ifdef __linux__
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif

#include <vector>
#include <ostream>

#include "Socket.h"

#define MAX_EVENTS 32

class Server;

class EventQueue {
 private:
  struct Platform {
#ifdef __linux__
    typedef struct epoll_event event_t;
    typedef uint32_t filt_t;
    typedef uint32_t flag_t;

    static constexpr filt_t in = EPOLLIN;
    static constexpr filt_t out = EPOLLOUT;
    static constexpr filt_t both = EPOLLIN | EPOLLOUT;
    static constexpr flag_t err = EPOLLERR;
    static constexpr flag_t w_hup = EPOLLHUP;
    static constexpr flag_t r_hup = EPOLLRDHUP;

    static inline int create_queue() {
      return epoll_create1(EPOLL_CLOEXEC);
    }
    static inline void add(EventQueue& q, int fd, filt_t direction) {
      event_t event = Platform::create_event(fd, direction);
      if (epoll_ctl(q.queue_fd_, EPOLL_CTL_ADD, fd, &event) == -1)
        throw IOException("epoll add", errno);
    }
    static inline void mod(EventQueue& q, int fd, filt_t new_direction) {
      event_t event = Platform::create_event(fd, new_direction);
      if (epoll_ctl(q.queue_fd_, EPOLL_CTL_MOD, fd, &event) == -1)
        throw IOException("epoll mod", errno);
    }
    static inline void del(EventQueue& q, int fd, filt_t) {
      epoll_ctl(q.queue_fd_, EPOLL_CTL_DEL, fd, nullptr);
    }
    static inline event_t create_event(int fd, filt_t direction) {
      return {.events = direction | r_hup, .data = { .fd = fd } };
    }
    static inline int getFileDes(const event_t& ev) {
      return ev.data.fd;
    }
    static inline bool checkFlag(const event_t& ev, flag_t flag) {
      return (ev.events & flag) != 0;
    }
    static inline bool checkFilter(const event_t& ev, filt_t filter) {
      return checkFlag(ev, filter);
    }
    static inline std::ostream& printEvent(std::ostream& stream, const event_t& ev) {
      return stream << '[' << getFileDes(ev) << "]\tEvent\tFlags: " << ev.events << '\n';
    }
    static inline void wait(EventQueue& q) {
      q.event_count_ = epoll_wait(q.queue_fd_, q.events_, MAX_EVENTS, 5000);
    }
#else
    typedef struct kevent event_t;
    typedef int16_t filt_t;
    typedef uint16_t flag_t;

    static constexpr filt_t in = EVFILT_READ;
    static constexpr filt_t out = EVFILT_WRITE;
    static constexpr filt_t both = 0; // Not actually supported by kqueue, hacky
    static constexpr flag_t err = EV_ERROR;
    static constexpr flag_t w_hup = EV_EOF;
    static constexpr flag_t r_hup = w_hup;
    static constexpr flag_t flag_add = EV_ADD;
    static constexpr flag_t flag_enable = EV_ENABLE;
    static constexpr flag_t flag_disable = EV_DISABLE;

    static inline int create_queue() {
      return kqueue();
    }
    static inline void add(EventQueue& q, int fd, filt_t direction) {
      if (direction != both) {
        q.changelist_.push_back(create_event(fd, direction, flag_add | flag_enable));
      } else {
        add(q, fd, in);
        add(q, fd, out);
      }
    }
    static inline void mod(EventQueue& q, int fd, filt_t new_direction) {
      if (new_direction == in)
        del(q, fd, out);
      else if (new_direction == out)
        del(q, fd, in);
      add(q, fd, new_direction);
    }
    static inline void del(EventQueue& q, int fd, filt_t direction) {
      q.changelist_.push_back(create_event(fd, direction, flag_disable));
    }
    static inline event_t create_event(int fd, filt_t direction, flag_t flags) {
      event_t ev;
      EV_SET(&ev, fd, direction, flags, 0, 0, nullptr);
      return ev;
    }
    static inline int getFileDes(const event_t& ev) {
      return static_cast<int>(ev.ident);
    }
    static inline bool checkFlag(const event_t& ev, flag_t flag) {
      return (ev.flags & flag) != 0;
    }
    static inline bool checkFilter(const event_t& ev, filt_t filter) {
      return (ev.filter == filter);
    }
    static inline std::ostream& printEvent(std::ostream& stream, const event_t& ev) {
      return stream << '[' << getFileDes(ev) << "]\tEvent"
                    << "\tFilter: " << ev.filter
                    << "\tFlags: " << ev.flags
                    << "\tFFlags: " << ev.fflags
                    << "\tData: " << ev.data << '\n';
    }
    static inline void wait(EventQueue& q) {
      q.event_count_ = kevent(q.queue_fd_, q.changelist_.data(), (int)q.changelist_.size(),
                              q.events_, MAX_EVENTS, nullptr);
      q.changelist_.clear();
      // Todo: check why this happens to begin with?
      // This happens when the only errors had to do with changing the event list
      // because the same event would be readded in the if statement below, this would loop infinitely
      if (Platform::checkFlag(q.events_[q.event_index_], KEVENT_FLAG_ERROR_EVENTS)) {
        // All events were just failed changes... smh my head
        q.event_count_ = 0;
      }
    }
#endif
  };

 public:
  EventQueue();
  ~EventQueue();
  EventQueue(const EventQueue& other) = delete;
  EventQueue& operator=(const EventQueue& rhs) = delete;

  using event_t = Platform::event_t;
  using filt_t = Platform::filt_t;
  using flag_t = Platform::flag_t;

  static constexpr filt_t in = Platform::in;
  static constexpr filt_t out = Platform::out;
  static constexpr filt_t both = Platform::both;
  static constexpr flag_t err = Platform::err;
  static constexpr flag_t w_hup = Platform::w_hup; // Connection is completely closed
  static constexpr flag_t r_hup = Platform::r_hup; // Peer closed their side of connection

  /**
   * Add a new file descriptor we want events for
   * @param fd The file descriptor
   * @param direction The direction we want events for
   */
  void add(int fd, filt_t direction = in);
  /**
   * Change the direction we want events for
   * @param fd The file descriptor we want to modify
   * @param new_direction The new direction we want events for
   */
  void mod(int fd, filt_t new_direction = both);
  /**
   * Remove a certain file descriptor
   * @param fd
   * @param dir
   */
  void del(int fd, filt_t dir);

  event_t& getNext(Server& server);

  static inline int getFileDes(const event_t& ev) {
    return Platform::getFileDes(ev);
  };
  static inline bool isRead(const event_t& ev) {
    return Platform::checkFilter(ev, in);
  };
  static inline bool isWrite(const event_t& ev) {
    return Platform::checkFilter(ev, out);
  };
  static inline bool isError(const event_t& ev) {
    return Platform::checkFlag(ev, err);
  };
  static inline bool isRdHangup(const event_t& ev) {
    return Platform::checkFlag(ev, r_hup);
  }
  static inline bool isWrHangup(const event_t& ev) {
    return Platform::checkFlag(ev, w_hup);
  };
  static inline std::ostream& print_event(std::ostream& stream, const event_t& ev) {
    return Platform::printEvent(stream, ev);
  }

 private:
  int queue_fd_;

  event_t events_[MAX_EVENTS];
  int event_count_;
  int event_index_;

#ifndef __linux__
  std::vector<event_t> changelist_;
#endif
};

inline std::ostream& operator<<(std::ostream& out, const EventQueue::event_t& event) {
  return EventQueue::print_event(out, event);
}
