#include "EventQueue.h"

#include <unistd.h>

#include <cerrno>

#include "IOException.h"
#include "util/Log.h"

EventQueue::EventQueue() : events_(), event_count_(), event_index_() {
  queue_fd_ = Platform::create_queue();
  if (queue_fd_ == -1)
    throw IOException("Failed to create system event queue", errno);
}

EventQueue::~EventQueue() {
  close(queue_fd_);
}

void EventQueue::add(int fd, filt_t direction) {
  Platform::add(*this, fd, direction);
}

void EventQueue::mod(int fd, filt_t new_direction) {
  Platform::mod(*this, fd, new_direction);
}

void EventQueue::del(int fd, filt_t dir) {
  Platform::del(*this, fd, dir);
}

EventQueue::event_t& EventQueue::getNext() {
  while (event_index_ >= event_count_) {
    event_index_ = 0;
    Platform::wait(*this);
    if (event_count_ == -1)
      throw IOException("EventQueue", errno);
    // if (event_count_ == 0)
    //   timeout();
  }
  if (isError(events_[event_index_]))
    del(getFileDes(events_[event_index_]));
  Log::debug(events_[event_index_]);
  return events_[event_index_++];
}
