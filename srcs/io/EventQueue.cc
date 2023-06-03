#include <iostream>
#include <unistd.h>
#include <exception>
#include <cstring>
#include <cerrno>
#include "EventQueue.h"
#include "http/ErrorResponse.h"

EventQueue::EventQueue() : events_(), event_count_(), event_index_() {
  // cloexec, so cgi does not inherit the queue
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ == -1)
    throw std::exception(); // todo
}

EventQueue::~EventQueue() {
  close(epoll_fd_);
}

void EventQueue::add(int fd, void* context, uint32_t flags) {
  Data* data = new Data();
  data->fd = fd;
  data->context = context;

  epoll_event ev = {flags, static_cast<void*>(data)};
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
    std::cerr << "Error adding epoll fd: " << std::strerror(errno) << '\n';
    throw ErrorResponse();
  }
}

void EventQueue::mod(int fd, void* context, uint32_t flags) {

}

void EventQueue::del(int fd) {

}

void EventQueue::doPoll() {

}

epoll_event& EventQueue::getNext() {
  if (event_index_ >= event_count_) {
    event_index_ = 0;
    while ((event_count_ = epoll_wait(epoll_fd_, events_, MAX_EVENTS, -1)) <= 0);
  }
  return events_[event_index_++];
}
