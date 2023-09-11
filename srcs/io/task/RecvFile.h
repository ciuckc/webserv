#pragma once

#include "IOTask.h"

class RecvFile : public ITask {
 public:
  explicit RecvFile(int fd, size_t size) : fd_(fd), remaining_(size) {};

  bool operator()(Connection& connection) override {
    connection.getBuffer().writeTo(fd_, remaining_);
    return remaining_ == 0;
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
