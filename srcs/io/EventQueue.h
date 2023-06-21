#pragma once
#ifdef __linux__
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif

#include <map>
#include <vector>
#include <iostream>

#include "Socket.h"

#define MAX_EVENTS 32

class EventQueue {
 private:
  EventQueue(const EventQueue& other);           // = delete;
  EventQueue& operator=(const EventQueue& rhs);  // = delete;

  struct Platform {
#ifdef __linux__
    typedef struct epoll_event event_t;
    typedef uint32_t filt_t;
    typedef uint32_t flag_t;

    static const filt_t in = EPOLLIN;
    static const filt_t out = EPOLLOUT;
    static const flag_t err = EPOLLERR;
    static const flag_t eof = EPOLLHUP;

    static inline int create_queue() {
      return epoll_create1(EPOLL_CLOEXEC);
    }
    static inline event_t create_event(int fd, filt_t direction) {
      return {.events = direction, .data = { .fd = fd } };
    }
    static inline int getFileDes(const event_t& ev) {
      return ev.data.fd;
    }
    static inline bool checkFlag(const event_t& ev, flag_t flag) {
      return (ev.events & flag) != 0;
    }
    static inline bool checkFilter(const event_t& ev, filt_t filter) {
      return checkFlag(ev, filter)
    }
    static inline void printEvent(const event_t& ev) {
      std::cout << "Event fd: " << getFileDes(ev)
                << "\tEvents: " << ev.events
                << '\n';
    }
#else
    typedef struct kevent event_t;
    typedef int16_t filt_t;
    typedef uint16_t flag_t;

    static const filt_t in = EVFILT_READ;
    static const filt_t out = EVFILT_WRITE;
    static const flag_t err = EV_ERROR;
    static const flag_t eof = EV_EOF;

    static inline int create_queue() {
      return kqueue();
    }
    static inline event_t create_event(int fd, filt_t direction) {
      event_t ev;
      EV_SET(&ev, fd, direction, EV_ADD, 0, 0, nullptr);
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
    static inline void printEvent(const event_t& ev) {
      std::cout << "Event fd: " << getFileDes(ev)
                << "\tFilter: " << ev.filter
                << "\tFlags: " << ev.flags
                << "\tFFlags: " << ev.fflags
                << '\n';
    }
#endif
  };

 public:
  EventQueue();
  ~EventQueue();

  using event_t = Platform::event_t;
  using filt_t = Platform::filt_t;
  using flag_t = Platform::flag_t;

  static const filt_t in = Platform::in;
  static const filt_t out = Platform::out;
  static const flag_t err = Platform::err;
  static const flag_t eof = Platform::eof;

  void add(int fd, filt_t direction);
  void mod(int fd, filt_t direction);
  void del(int fd);

  event_t& getNext();

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
  static inline bool isHangup(const event_t& ev) {
    return Platform::checkFlag(ev, eof);
  };

 private:
  int queue_fd_;

  event_t events_[MAX_EVENTS];
  int event_count_;
  int event_index_;

  std::vector<event_t> changelist_;
};
