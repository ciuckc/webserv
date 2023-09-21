#pragma once

#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "io/EventQueue.h"

class Handler;
class Config;
class Server {
 private:
  EventQueue evqueue_;
  std::vector<std::unique_ptr<Handler>> handlers_;
  std::list<size_t> handler_idxs_;
  std::deque<size_t> handler_timeouts_;
  std::deque<std::function<void()>> task_queue_;

  void handle_event(EventQueue::event_t& event);

 public:
  explicit Server(Config& config);
  ~Server();

  void loop();
  void purge_connections();

  void add_sub(std::unique_ptr<Handler>&& h);
  void del_sub(size_t idx);

  void run_later(std::function<void()>&&);
  void run_tasks();

  EventQueue& getEventQueue();
};
