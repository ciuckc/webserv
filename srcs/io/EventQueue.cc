#include "EventQueue.h"

#include <unistd.h>

#include <cerrno>
#include <iostream>

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
}

static EventQueue::event create_event(int fd, uint32_t direction) {
#ifdef __linux__
  return (epoll_event){.events = direction, .data = {.fd = fd}};
#else
  EventQueue::event ev;
  EV_SET(&ev, fd, direction, EV_ADD, 0, nullptr, nullptr);
  // the first NULL here should be an int pointer, for listen sockets it will contain the backlog
  // and for read/write sockets it will contain the amount of bytes we can read/write
  return ev;
#endif
}

void EventQueue::add(int fd, uint32_t direction) {
  event ev = create_event(fd, direction);
  changelist_.push_back(ev);
}

void EventQueue::mod(int fd, uint32_t direction) { add(fd, direction); }

void EventQueue::del(int fd) {
#ifdef __linux__
  epoll_ctl(queue_fd_, EPOLL_CTL_DEL, fd, nullptr);
#else
  event.flags = EV_DELETE;
  changelist_.push_back(event);
#endif
}

#ifdef __linux__
static void update_events(int queue, std::vector<EventQueue::event>& changes) {
  if (changes.empty())
    return;

  typedef std::vector<EventQueue::event>::iterator iter;
  for (iter i = changes.begin(); i < changes.end(); ++i) {
    int fd = EventQueue::getFileDes(*i);
    EventQueue::event* ptr = i.operator->();

    if (epoll_ctl(queue, EPOLL_CTL_ADD, fd, ptr) == -1
    && (errno != EEXIST || epoll_ctl(queue, EPOLL_CTL_MOD, fd, ptr) == -1))
      throw IOException("epoll", errno);
  }
}
#endif

EventQueue::event& EventQueue::getNext() {
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
  // We need to remove all the sockets that don't work anymore
  // I don't want to make this function recursive but also poll more
  // so that's why the goto is there
  if (isHangup(events_[event_index_]) || isError(events_[event_index_]))
    del(getFileDes(events_[event_index_]));
  std::cout << "Event " << getFileDes(events_[event_index_]) << " flags: " << events_[event_index_].events << '\n';
  return events_[event_index_++];
}

int EventQueue::getFileDes(const EventQueue::event& ev) {
#ifdef __linux__
  return ev.data.fd;
#else
  return ev.ident;
#endif
}
