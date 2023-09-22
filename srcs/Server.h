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
  struct timeout_cmp {
   private:
    Server& server_;
   public:
    explicit timeout_cmp(Server& s);
    bool operator()(uint32_t a, uint32_t b) const;
    bool operator()(uint32_t i, const EventQueue::timep_t &t) const;
  };
  const timeout_cmp to_cmp_ {*this};

  EventQueue event_queue_;
  std::vector<std::unique_ptr<Handler>> handlers_;
  std::list<uint32_t> handler_idxs_;
  std::deque<uint32_t> handler_timeouts_;
  std::deque<std::function<void()>> task_queue_;

  void handle_event(EventQueue::event_t& event);

 public:
  explicit Server(Config& config);
  ~Server();

  void loop();
  void purge_connections();

  void add_sub(std::unique_ptr<Handler>&& h);
  void del_sub(uint32_t idx);

  void run_later(std::function<void()>&&);
  void run_tasks();

  EventQueue& getEventQueue();
};
