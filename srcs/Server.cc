#include "Server.h"

#include "config/Config.h"
#include "io/ListenSocket.h"
#include "io/IOException.h"
#include "util/Log.h"
#include "io/Signal.h"
#include "io/Handler.h"

Server::Server(Config& config) {
  std::map<uint16_t, ListenSocket*> ports;

  for (auto& srvcfg : config.getServers()) {
    uint16_t port = srvcfg.getPort();
    auto it = ports.find(port);
    ListenSocket* sock;
    if (it != ports.end()) {
      sock = it->second;
    } else {
      try {
        auto sub = std::make_unique<ListenSocket>(*this, port);
        ports[port] = sock = sub.get();
        add_sub(std::move(sub));
      } catch (const IOException& ex) {
        Log::error("Error creating listen socket: ", ex.what());
        continue;
      }
    }
    sock->addConfig(srvcfg);
  }
  Sig::setup_signals();
}

// This needs to be in here, otherwise everything magically breaks
Server::~Server() = default;

void Server::loop() {
  Log::info("[Server] Entering main loop!\n");
  while (true) {
    try {
      handle_event(event_queue_.getNext(*this));
    } catch (const IOException& err) {
      Log::warn("IOException: ", err.what());
    }
    if (Log::log_level >= Log::DEBUG)
      std::cout << '\n';
  }
}
void Server::handle_event(EventQueue::event_t& event) {
  const uint32_t idx = EventQueue::getIndex(event);

  if (idx >= handlers_.size() || !handlers_[idx])
    return;
  auto& handler = *handlers_[idx];
  handler.updateTimeout(event_queue_.lastWait());
  if (handler.handle(event)) {
    del_sub(idx);
    return;
  }
  handler.updateFilter(event_queue_);
}


void Server::add_sub(std::unique_ptr<Handler>&& h) {
  Log::trace("[Server] Adding IOHandler ", h->getName(), '\n');
  uint32_t idx;
  int fd = h->getFD();
  auto filter = h->getFilter();

  h->updateTimeout(event_queue_.lastWait());
  auto& to = h->getExpiry();
  if (handler_idxs_.empty()) {
    h->setIndex(idx = handlers_.size());
    handlers_.push_back(std::forward<std::unique_ptr<Handler>>(h));
  } else {
    h->setIndex(idx = handler_idxs_.front()), handler_idxs_.pop_front();
    handlers_[idx] = std::forward<std::unique_ptr<Handler>>(h);
  }
  event_queue_.add(fd, idx, filter);
  if (to != std::chrono::system_clock::time_point::max())
    handler_timeouts_.push_back(idx);
}

void Server::del_sub(uint32_t idx) {
  if (!handlers_[idx])
    return;
  handlers_[idx].reset();
  handler_idxs_.push_back(idx);
}

template<typename Compare>
static void sort_timeouts(std::deque<uint32_t>& timeouts, Compare& cmp) {
  // small insertion sort as the list will be mostly sorted anyway
  for (auto i = timeouts.begin(), j = i + 1; j != timeouts.end(); ++j, ++i) {
    if (!cmp(*j, *i))
      continue;
    uint32_t val = *j;
    auto pos = std::upper_bound(timeouts.begin(), i, *j, cmp);
    std::move(pos, j, pos + 1);
    *pos = val;
  }
}

void Server::purge_connections() {
  using namespace std::chrono;
  static system_clock::time_point prev;
  system_clock::time_point now = system_clock::now();
  if (handler_timeouts_.empty() || now - prev < 5000ms)
    return;
  Log::trace("[Server] Purging inactive IOHandlers\n");
  prev = now;
  sort_timeouts(handler_timeouts_, to_cmp_);
  auto end = std::lower_bound(handler_timeouts_.begin(), handler_timeouts_.end(), now, to_cmp_);
  std::for_each(handler_timeouts_.begin(), end, [this](auto idx){
    auto& hp = handlers_[idx];
    if (hp) {
      if (!hp->handleTimeout(*this, true))
        return
      del_sub(idx);
    }
    handler_timeouts_.pop_front();
  });
}

void Server::run_later(std::function<void()>&& runnable) {
  task_queue_.emplace_back(std::move(runnable));
}

void Server::run_tasks() {
  if (task_queue_.empty())
    return;
  Log::trace("[Server] Running queued tasks\n");
  while (!task_queue_.empty()) {
    task_queue_.front()();
    task_queue_.pop_front();
  }
}

EventQueue& Server::getEventQueue() {
  return event_queue_;
}

Server::timeout_cmp::timeout_cmp(Server& s) : server_(s) {}
bool Server::timeout_cmp::operator()(uint32_t a, uint32_t b) const {
  auto& handlers = server_.handlers_;
  if (handlers[a] && handlers[b])
    return handlers[a]->getExpiry() < handlers[b]->getExpiry();
  return (bool)handlers[b];
}
bool Server::timeout_cmp::operator()(uint32_t i, const EventQueue::timep_t& t) const {
  auto& handlers = server_.handlers_;
  return !handlers[i] || handlers[i]->getExpiry() < t;;
}
