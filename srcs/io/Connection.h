#pragma once

#include <list>
#include <stack>
#include <memory>

#include "Socket.h"
#include "EventQueue.h"
#include "http/Request.h"
#include "BufferPool.h"
#include "ConnectionBuffer.h"

class ITask;
class OTask;

class Connection {
 private:
  Socket socket_;
  ConnectionBuffer buffer_;
  EventQueue& event_queue_;
  std::list<std::unique_ptr<ITask>> iqueue_;
  std::list<std::unique_ptr<OTask>> oqueue_;

  bool should_close_;

 public:
  Connection(int fd, EventQueue& event_queue, BufferPool& buf_mgr);
  ~Connection();

  void handle(EventQueue::event& event);
  void handleIn(WS::IOStatus& status);
  void handleOut(WS::IOStatus& status);

  Socket& getSocket();
  ConnectionBuffer& getBuffer();

  void addTask(ITask* task);
  void addTask(OTask* task);

  inline bool shouldClose() const { return should_close_; }
  inline void close() { should_close_ = true; }
};

