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
  changelist_.push_back(Platform::create_event(fd, direction));
}

void EventQueue::mod(int fd, filt_t direction) {
  add(fd, direction);
}

void EventQueue::del(int fd) {
#ifdef __linux__
  epoll_ctl(queue_fd_, EPOLL_CTL_DEL, fd, nullptr);
#else
  event_t ev = Platform::create_event(fd, 0);
  ev.flags = EV_DELETE;
  changelist_.push_back(ev);
#endif
}

#ifdef __linux__
static void update_events(int queue, std::vector<EventQueue::event_t>& changes) {
  if (changes.empty())
    return;

  typedef std::vector<EventQueue::event_t>::iterator iter;
  for (iter i = changes.begin(); i < changes.end(); ++i) {
    int fd = EventQueue::getFileDes(*i);
    EventQueue::event_t* ptr = i.operator->();

    if (epoll_ctl(queue, EPOLL_CTL_ADD, fd, ptr) == -1 &&
        (errno != EEXIST || epoll_ctl(queue, EPOLL_CTL_MOD, fd, ptr) == -1))
      throw IOException("epoll", errno);
  }
}
#endif

static const bool debug_evqueue = false;
EventQueue::event_t& EventQueue::getNext() {
  while (event_index_ >= event_count_) {
    event_index_ = 0;
#ifdef __linux__
    update_events(queue_fd_, changelist_);
    event_count_ = epoll_wait(queue_fd_, events_, MAX_EVENTS, -1);
#else
    event_count_ =
        kevent(queue_fd_, changelist_.data(), (int)changelist_.size(), events_, MAX_EVENTS, nullptr);
#endif
    if (event_count_ == -1)
      throw IOException("EventQueue", errno);
    // if (event_count_ == 0)
    //   timeout();
    changelist_.clear();
#ifndef __linux__
    // Todo: check why this happens to begin with?
    // This happens when the only errors had to do with changing the event list
    // because the same event would be readded in the if statement below, this would loop infinitely
    if (Platform::checkFlag(events_[event_index_], KEVENT_FLAG_ERROR_EVENTS)) {
      // All events were just failed changes... smh my head
      event_count_ = 0;
    }
#endif
  }
  if (isHangup(events_[event_index_]) || isError(events_[event_index_]))
    del(getFileDes(events_[event_index_]));
  Log::debug(events_[event_index_]);
  return events_[event_index_++];
}
