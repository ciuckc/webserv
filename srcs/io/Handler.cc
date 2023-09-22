#include "Handler.h"

#include "util/Log.h"

Handler::Handler(int fd, Handler::filt_t f, int timeout_ms)
    : fd_(fd), filter_(f), timeout_(std::chrono::milliseconds(timeout_ms)) {}
Handler::~Handler() = default;

bool Handler::handle(const EventQueue::event_t& event) {
  struct h_info {
    filt_t ev;
    bool (Handler::*fun)();
    bool abort;
  };
  static constexpr std::array<h_info, 5> info {{
    {EventQueue::err, &Handler::handleError, true},
    {EventQueue::w_hup, &Handler::handleWHup, false},
    {EventQueue::in, &Handler::handleRead, false},
    {EventQueue::out, &Handler::handleWrite, false},
    {EventQueue::r_hup, &Handler::handleRHup, false}}
  };

  Log::debug(*this, event, '\n');

  for (auto& i : info)
    if (event.events & i.ev)
      if ((this->*i.fun)() || i.abort)
        return true;

  return false;
}

void Handler::updateTimeout(const Handler::timep_t& wait) {
  expires_ = wait + timeout_;
}

void Handler::enableFilter(EventQueue& evq, Handler::filt_t f) {
  addFilter(f);
  updateFilter(evq);
}

void Handler::updateFilter(EventQueue& evq) {
  if (filter_ == prev_filter_)
    return;
  evq.mod(fd_, index_, filter_); // todo
  prev_filter_ = filter_;
}

int Handler::getFD() const {
  return fd_;
}

uint32_t Handler::getIndex() const {
  return index_;
}

Handler::filt_t Handler::getFilter() const {
  return filter_;
}

const Handler::timep_t& Handler::getExpiry() const {
  return expires_;
}

void Handler::setIndex(uint32_t idx) {
  index_ = idx;
}


void Handler::addFilter(Handler::filt_t f) {
  filter_ |= f;
}

void Handler::delFilter(Handler::filt_t f) {
  filter_ &= ~f;
}
