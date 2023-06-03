#pragma once
#include <sys/epoll.h>
#define MAX_EVENTS 32

class EventQueue {
 private:
  EventQueue(const EventQueue& other); // = delete;
  EventQueue& operator=(const EventQueue& rhs); // = delete;

  int epoll_fd_;

  epoll_event events_[MAX_EVENTS];
  int event_count_;
  int event_index_;

 public:
  struct Data {
    int fd; // dst/src
    //todo: insert handler type
    void* context; // state
  };

  EventQueue();
  ~EventQueue();

  void add(int fd, void* context, uint32_t flags);
  void mod(int fd, void* context, uint32_t flags);
  void del(int fd);

  void doPoll();
  epoll_event& getNext();
};
