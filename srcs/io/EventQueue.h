#pragma once
#ifdef __linux__
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif
#include <vector>

#define MAX_EVENTS 32

class EventQueue {
 private:
  EventQueue(const EventQueue& other);           // = delete;
  EventQueue& operator=(const EventQueue& rhs);  // = delete;

 public:
#ifdef __linux__
#define IN EPOLLIN
#define OUT EPOLLOUT
  typedef struct epoll_event event;
#else
#define IN EVFILT_READ
#define OUT EVFILT_WRITE
  typedef struct kevent event;
#endif

  struct Data {
    int fd;  // dst/src
    // todo: insert handler type
    void* context;  // state
  };

  EventQueue();
  ~EventQueue();

  void add(int fd, void* context, uint32_t direction);
  void mod(int fd, void* context, uint32_t direction);
  void del(event ev);

  event& getNext();

 private:
  int queue_fd_;

  event events_[MAX_EVENTS];
  int event_count_;
  int event_index_;

  std::vector<event> changelist_;

  static int getFileDes(const event& ev);
  static Data* getUserData(const event& ev);
};
