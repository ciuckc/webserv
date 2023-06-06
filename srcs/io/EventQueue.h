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
    void* handler; //todo: replace this with a handler reference. Maybe the
                   // IOTask type would be nice for this? We do still need this
                   // struct though as the task will change for the fd, so if
                   // we dynamically allocate the handler we will have to track
                   // more memory

    void operator()();
  };

  EventQueue();
  ~EventQueue();

  void add(int fd, void* context, uint32_t direction);
  void mod(int fd, void* context, uint32_t direction);
  void del(event ev);

  event& getNext();

  static int getFileDes(const event& ev);
  static Data* getUserData(const event& ev);
  static bool isError(const event& ev);

 private:
  int queue_fd_;

  event events_[MAX_EVENTS];
  int event_count_;
  int event_index_;

  std::vector<event> changelist_;
};
