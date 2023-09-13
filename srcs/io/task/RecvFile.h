#pragma once

#include "IOTask.h"
#include "io/Connection.h"
#include "util/Log.h"

class RecvFile : public ITask {
 public:
  explicit RecvFile(int fd, size_t size) : fd_(fd), remaining_(size) {};

  WS::IOStatus operator()(Connection& connection) override {
    if (connection.getInBuffer().write(fd_, remaining_) != WS::IO_GOOD) {
      Log::error(connection, "RecvFile failed\n");
      return WS::IO_FAIL;
    }
    return remaining_ == 0 ? WS::IO_GOOD : WS::IO_AGAIN;
  };

  void onDone(Connection& connection) override {
    Log::trace(connection, "RecvFile done\n");
  };

 protected:
  ~RecvFile() override { close(fd_); }

 private:
  int fd_;
  size_t remaining_;
};
