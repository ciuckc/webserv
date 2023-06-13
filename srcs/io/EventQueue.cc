#include "EventQueue.h"

#include <unistd.h>

#include <cerrno>
#include <exception>

#include "IOException.h"

static int create_queue() {
#ifdef __linux__
  return epoll_create1(EPOLL_CLOEXEC);
#else
  return kqueue();
#endif
}

EventQueue::EventQueue() : events_(), event_count_(), event_index_() {
  // cloexec, so cgi does not inherit the queue
  queue_fd_ = create_queue();
  if (queue_fd_ == -1) throw IOException("Failed to create system event queue", errno);
}

EventQueue::~EventQueue() {
  close(queue_fd_);
  for (std::map<int, Data*>::iterator it = event_args_.begin(); it != event_args_.end(); ++it) {
    delete it->second;
  }
}

static EventQueue::event create_event(int fd, void* context, uint32_t direction) {
#ifdef __linux__
  (void)fd;
  return (epoll_event){direction, {context}};
#else
  EventQueue::event ev;
  EV_SET(&ev, fd, direction, EV_ADD, 0, NULL, context);
  // the first NULL here should be an int pointer, for listen sockets it will contain the backlog
  // and for read/write sockets it will contain the amount of bytes we can read/write
  return ev;
#endif
}

void EventQueue::add(int fd, void* context, uint32_t direction) {
  Data*& data = event_args_[fd];
  if (data == NULL) data = new Data(fd);
  data->handler = context;

  event ev = create_event(fd, data, direction);
  changelist_.push_back(ev);
}

void EventQueue::mod(int fd, void* context, uint32_t direction) { add(fd, context, direction); }

void EventQueue::del(event event) {
  std::map<int, Data*>::iterator it = event_args_.find(getFileDes(event));
  if (it == event_args_.end()) return;
  delete it->second;

#ifdef __linux__
  epoll_ctl(queue_fd_, EPOLL_CTL_DEL, it->first, &event);
#else
  event.flags = EV_DELETE;
  changelist_.push_back(event);
#endif
}

#ifdef __linux__
static void update_events(int queue, std::vector<EventQueue::event>& changes) {
  typedef std::vector<EventQueue::event>::iterator iter;

  for (iter i = changes.begin(); i < changes.end(); ++i) {
    int fd = EventQueue::getFileDes(*i);
    EventQueue::event* ptr = i.operator->();

    if (epoll_ctl(queue, EPOLL_CTL_ADD, fd, ptr) != -1) continue;
    if (errno != EEXIST || epoll_ctl(queue, EPOLL_CTL_MOD, fd, ptr) == -1) throw IOException("epoll", errno);
  }
}
#endif

EventQueue::Data& EventQueue::getNext() {
  while (event_index_ >= event_count_) {
    event_index_ = 0;
#ifdef __linux__
    update_events(queue_fd_, changelist_);
    event_count_ = epoll_wait(queue_fd_, events_, MAX_EVENTS, -1);
#else
    event_count_ = kevent(queue_fd_, changelist_.data(), (int)changelist_.size(), events_, MAX_EVENTS, NULL);
#endif
    if (event_count_ == -1) throw IOException("EventQueue", errno);
    // if (event_count_ == 0)
    //   timeout();
    changelist_.clear();
  }
  return *getUserData(events_[event_index_++]);
}

int EventQueue::getFileDes(const EventQueue::event& ev) { return getUserData(ev)->socket.get_fd(); }

EventQueue::Data* EventQueue::getUserData(const EventQueue::event& ev) {
#ifdef __linux__
  return reinterpret_cast<Data*>(ev.data.ptr);
#else
  return reinterpret_cast<Data*>(ev.udata);
#endif
}

bool EventQueue::isError(const EventQueue::event& ev) {
#ifdef __linux__
  return (ev.events & EPOLLERR) != 0;
#else
  return ev.flags == EV_ERROR;
#endif
}

EventQueue::Data::Data(int fd) : socket(fd), handler() {}

EventQueue::Data::~Data() {}

void EventQueue::Data::operator()() {
  (void)handler;
  // handler.doIO();
}
