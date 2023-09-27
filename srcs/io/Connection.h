#pragma once

#include <list>
#include <map>
#include <memory>

#include "Socket.h"
#include "io/task/IOTask.h"
#include "Handler.h"
#include "Server.h"
#include "RingBuffer.h"

class ConfigServer;
class Response;

class Connection : public Handler {
 private:
  Server& server_;
  Socket socket_;
  RingBuffer in_buffer_;
  RingBuffer out_buffer_;
  size_t in_buffer_size_ = RingBuffer::buf_size_;
  size_t out_buffer_size_ = RingBuffer::buf_size_;

  const std::map<std::string, ConfigServer&>& host_map_;

  std::list<std::unique_ptr<ITask>> in_queue_;
  std::list<std::unique_ptr<OTask>> out_queue_;

  bool keep_alive_ = true;
  bool read_closed_ = false;
  uint32_t request_count_ = 0;

  bool awaitRequest();

 protected:
  bool handleRead() override;
  bool handleWrite() override;
  bool handleWHup() override;
  bool handleRHup() override;
  bool handleError() override;

 public:
  Connection(Server&, Socket&&, const std::map<std::string, ConfigServer&>&);
  ~Connection() override;

  void addTask(std::unique_ptr<ITask>&& task);
  void addTask(std::unique_ptr<OTask>&& task);

  void enqueueResponse(Response&& response);

  // Send the FIN packet, signifying that we're done. After this the peer should
  // also send one, we can then close the socket!
  void shutdown();

  [[nodiscard]] const std::map<std::string, ConfigServer&>& getHostMap() const;
  [[nodiscard]] const std::string& getName() const override;

  [[nodiscard]] RingBuffer& getInBuffer();
  [[nodiscard]] RingBuffer& getOutBuffer();

  [[nodiscard]] Server& getServer();

  void setInSize(size_t size = RingBuffer::buf_size_);
  void setOutSize(size_t size = RingBuffer::buf_size_);

  void setKeepAlive(bool keepAlive);
  void closeRead();

  // Run the first I/OTask, does not actually write/read more
  WS::IOStatus runITask();
  WS::IOStatus runOTask();

  bool handleTimeout(Server& server, bool src) override;

  void notifyInDone(bool error);
};
