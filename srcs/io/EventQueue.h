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
  EventQueue(const EventQueue& other); // = delete;
  EventQueue& operator=(const EventQueue& rhs); // = delete;

 public:
#ifdef __linux__
  typedef struct epoll_event event
#else
  typedef struct kevent event;
#endif

  struct Data {
    int fd; // dst/src
    //todo: insert handler type
    void* context; // state
  };

  EventQueue();
  ~EventQueue();

  void add(int fd, void* context, bool listen_sock);
  void mod(int fd, void* context, uint32_t flags);
  void del(int fd);

  void doPoll();
  event& getNext();

 private:
  int queue_fd_;

  event events_[MAX_EVENTS];
  int event_count_;
  int event_index_;

  std::vector<event> changelist_;
};
