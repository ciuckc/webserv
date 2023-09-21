#pragma once

#include "IOTask.h"
#include "io/Handler.h"
#include "io/RingBuffer.h"

// Read from socket into pipe
class SpliceIn : public ITask {
 private:
  class OHandler : public Handler {
   private:
    Server& server_;
    SpliceIn& parent_;
    Connection& connection_;
    RingBuffer& buffer_;
    size_t remaining_;
    std::string name_;
   public:
    OHandler(Server& server, SpliceIn& parent, Connection& conn, int pipe_fd, size_t len);
    ~OHandler() override;

    [[nodiscard]] const std::string& getName() const override;
    bool handleTimeout(Server& server, bool) override;

   protected:
    bool handleRead() override;
    bool handleWHup() override;
    bool handleError() override;
    bool handleWrite() override;
    bool handleRHup() override;
  };

  Server& server_;
  OHandler* handler_ = nullptr;
  bool done_ = false;
  bool fail_ = false;

 public:
  SpliceIn(Server& server, Connection& conn, int pipe_fd, size_t len);
  ~SpliceIn() override;

  WS::IOStatus operator()(Connection& connection) override;
  void onDone(Connection& connection) override;

  void setDone();
  void setFail();
};
