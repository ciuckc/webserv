#pragma once

#include "IOTask.h"
#include "io/Connection.h"
#include "io/Handler.h"
#include "cgi/Cgi.h"

// Write from pipe into socket
class SpliceOut : public OTask {
 private:
  class IHandler : public Handler {
   private:
    Server& server_;
    SpliceOut& parent_;
    Connection& connection_;
    const std::string& cgi_path_;
    const ConfigServer& cfg_;
    RingBuffer& buffer_;
    std::string name_;
    bool state_headers_, chunked_;
    std::string headers_;

   public:
    IHandler(Server& server, SpliceOut& parent, Connection& connection,
             const std::string& cgi_path, const ConfigServer& cfg, int pipe_fd);
    ~IHandler() override;

    [[nodiscard]] const std::string& getName() const override;
    bool handleTimeout(Server& server, bool) override;

   protected:
    bool handleRead() override;
    bool handleWHup() override;
    bool handleError() override;
    bool handleWrite() override;
    bool handleRHup() override;
    void readBuffer_();
    void chunkBuffer_();
  };

  Server& server_;
  IHandler* handler_ = nullptr;
  bool done_ = false;
  bool fail_ = false;

 public:
  SpliceOut(Server& server, Connection& conn, const std::string& cgi_path, const ConfigServer& cfg, int pipe_fd);
  ~SpliceOut() override;

  WS::IOStatus operator()(Connection& connection) override;
  void onDone(Connection& connection) override;

  void setDone();
  void setFail();
};
