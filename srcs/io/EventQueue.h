#pragma once
#ifdef __linux__
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif
#include <map>
#include <vector>

#include "Socket.h"

#define MAX_EVENTS 32

class EventQueue {
 private:
  EventQueue(const EventQueue& other);           // = delete;
  EventQueue& operator=(const EventQueue& rhs);  // = delete;

 public:
#ifdef __linux__
#define EV_IN EPOLLIN
#define EV_OUT EPOLLOUT
  typedef struct epoll_event event;
#else
#define EV_IN EVFILT_READ
#define EV_OUT EVFILT_WRITE
  typedef struct kevent event;
#endif

  EventQueue();
  ~EventQueue();

  void add(int fd, uint32_t direction);
  void mod(int fd, uint32_t direction);
  void del(int fd);

  event& getNext();

  static int getFileDes(const event& ev);
  static inline bool isFlag(const event& ev, uint32_t flag) {
    return (ev.events & flag) != 0;
  }
  static inline bool isRead(const event& ev) {
    return isFlag(ev, EPOLLIN);
  };
  static inline bool isWrite(const event& ev) {
    return isFlag(ev, EPOLLOUT);
  };
  static inline bool isError(const event& ev) {
    return isFlag(ev, EPOLLERR);
  };
  static inline bool isHangup(const event& ev) {
    return isFlag(ev, EPOLLHUP);
  };

 private:
  int queue_fd_;

  event events_[MAX_EVENTS];
  int event_count_;
  int event_index_;

  std::vector<event> changelist_;
};
