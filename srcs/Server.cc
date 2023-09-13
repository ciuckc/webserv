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
      handle_event(evqueue_.getNext(*this));
    } catch (const IOException& err) {
      Log::warn("IOException: ", err.what());
    }
    if (Log::log_level >= Log::DEBUG)
      std::cout << '\n';
  }
}
void Server::handle_event(EventQueue::event_t& event) {
  const uint32_t idx = EventQueue::getIndex(event);

  if (idx >= handlers_.size() || !handlers_[idx]) {
    Log::error("Got event without subscriber? ", event, '\n');
    abort();
  }
  auto& handler = *handlers_[idx];
  handler.updateTimeout(evqueue_.lastWait());
  if (handler.handle(event)) {
    del_sub(idx);
    return;
  }
  handler.updateFilter(evqueue_);
}


void Server::add_sub(std::unique_ptr<Handler>&& h) {
  h->updateTimeout(evqueue_.lastWait());
  if (handler_idxs_.empty()) {
    h->setIndex(handlers_.size());
    evqueue_.add(h->getFD(), h->getIndex(), h->getfilter());
    handler_timeouts_.push_back(h->getIndex());
    handlers_.push_back(std::forward<std::unique_ptr<Handler>>(h));
  } else {
    h->setIndex(handler_idxs_.front());
    evqueue_.add(h->getFD(), h->getIndex(), h->getfilter());
    handler_timeouts_.push_back(h->getIndex());
    handler_idxs_.pop_front();
    handlers_[h->getIndex()] = std::forward<std::unique_ptr<Handler>>(h);
  }
}

void Server::del_sub(size_t idx) {
  if (!handlers_[idx])
    return;
  handlers_[idx].reset();
  handler_idxs_.push_back(idx);
}

void Server::purge_connections() {
  using namespace std::chrono;
  static system_clock::time_point prev;
  system_clock::time_point now = system_clock::now();
  if (now - prev < 5000ms)
    return;
  prev = now;
  auto timeout_cmp = [this](uint32_t a, uint32_t b){
    if (handlers_[a] && handlers_[b])
      return handlers_[a]->getExpiry() < handlers_[b]->getExpiry();
    if (handlers_[a])
      return false;
    if (handlers_[b])
      return true;
    return false;
  };
  auto tp_cmp = [this](uint32_t idx, const EventQueue::timep_t& v) {
    return !handlers_[idx] || handlers_[idx]->getExpiry() < v;
  };

  std::sort(handler_timeouts_.begin(), handler_timeouts_.end(), timeout_cmp);
  auto end = std::lower_bound(handler_timeouts_.begin(), handler_timeouts_.end(), now, tp_cmp);
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
  while (!task_queue_.empty()) {
    task_queue_.front()();
    task_queue_.pop_front();
  }
}
EventQueue& Server::getEventQueue() {
  return evqueue_;
}
