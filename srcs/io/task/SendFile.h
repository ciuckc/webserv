#pragma once

#include "IOTask.h"

class SendFile : public OTask {
 public:
  explicit SendFile(int fd) : fd_(fd) {};

  bool operator()(Connection& connection) override {
    while (!connection.getBuffer().needWrite())
      if (connection.getBuffer().readFrom(fd_))
        return true;

    return false;
  };

  void onDone(Connection&) override {
    Log::trace("Sendfile done");
  };

 protected:
  ~SendFile() override { close(fd_); }

 private:
  int fd_;
};
