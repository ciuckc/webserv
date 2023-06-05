#include <iostream>
#include <unistd.h>
#include <exception>
#include <cerrno>
#include "EventQueue.h"
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
  if (queue_fd_ == -1)
    throw IOException("Failed to create system event queue", errno);
}

EventQueue::~EventQueue() {
  close(queue_fd_);
}

static EventQueue::event create_event(int fd, void* context, bool listen_sock) {
#ifdef __linux__
  return {
    listen_sock ? EPOLLIN : EPOLLIN | EPOLLOUT,
    static_cast<void*>(data);
  };
#else
  EventQueue::event ev;
  EV_SET(&ev, fd,
         listen_sock ? EVFILT_READ : EVFILT_WRITE,
         EV_ADD, 0, NULL, context);
  // the first NULL here should be an int pointer, for listen sockets it will contain the backlog
  // and for read/write sockets it will contain the amount of bytes we can read/write
  return ev;
#endif
}

void EventQueue::add(int fd, void* context, bool listen_sock) {
  Data* data = new Data();
  data->fd = fd;
  data->context = context;

  event ev = create_event(fd, data, listen_sock);
  changelist_.push_back(ev);
}

void EventQueue::mod(int fd, void* context, uint32_t flags) {

}

void EventQueue::del(int fd) {

}

void EventQueue::doPoll() {

}

EventQueue::event& EventQueue::getNext() {
  if (event_index_ >= event_count_) {
    event_index_ = 0;
#ifdef __linux__
    for (std::vector<event>::iterator it = changelist_.begin(); it < changelist_.end(); it++) {
      if (epoll_ctl(queue_fd_, EPOLL_CTL_ADD, fd, *it) == -1) {
        if (errno == EEXIST) {
          if (epoll_ctl(queue_fd, EPOLL_CTL_MOD, fd, *it) != -1) {
            continue;
          }
        }
        throw IOException("epoll", errno);
      }
    }
    event_count_ = epoll_wait(queue_fd_, events_, MAX_EVENTS, -1);
#else
    event_count_ = kevent(queue_fd_, changelist_.data(), changelist_.size(), events_, MAX_EVENTS, NULL);
#endif
    if (event_count_ == -1)
      throw IOException("EventQueue", errno);
    // if (event_count_ == 0)
    //   timeout();
    changelist_.clear();
  }
  return events_[event_index_++];
}
