#include "EventQueue.h"

#include <cerrno>

#include "IOException.h"
#include "Server.h"

EventQueue::EventQueue() : events_(), event_count_(), event_index_() {
  queue_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (queue_fd_ == -1)
    throw IOException("Failed to create system event queue", errno);
}

EventQueue::~EventQueue() {
  close(queue_fd_);
}

void EventQueue::add(int fd, filt_t direction) const {
  event_t event = create_event(fd, direction);
  if (epoll_ctl(queue_fd_, EPOLL_CTL_ADD, fd, &event) == -1)
    throw IOException("epoll add", errno);
}

void EventQueue::mod(int fd, filt_t new_direction) const {
  struct epoll_event event = create_event(fd, new_direction);
  if (epoll_ctl(queue_fd_, EPOLL_CTL_MOD, fd, &event) == -1)
    throw IOException("epoll mod", errno);
}

void EventQueue::del(int fd) const {
  epoll_ctl(queue_fd_, EPOLL_CTL_DEL, fd, nullptr);
}

EventQueue::event_t& EventQueue::getNext(Server& server) {
  while (event_index_ >= event_count_) {
    server.purge_connections(); // Purge every time we collect new events from the queue
    event_index_ = 0;
    event_count_ = epoll_wait(queue_fd_, events_, max_events, 5000);
    if (event_count_ == -1 && errno != EINTR) // Ignore interrupts, don't care if we got signaled
      throw IOException("EventQueue", errno);
  }
  return events_[event_index_++];
}

EventQueue::event_t EventQueue::create_event(int fd, EventQueue::filt_t direction) {
  return {.events = direction | r_hup, .data = { .fd = fd } };
}
