#pragma once

#include "IOTask.h"

class SendFile : public OTask {
 public:
  explicit SendFile(int fd, size_t size) : fd_(fd), size_(size) {};
  ~SendFile() override {
    close(fd_);
  }

  bool operator()(Connection& connection) override {
    if (connection.getBuffer().readFrom(fd_, size_))
      return size_ == 0;
    return false;
  };

  void onDone(Connection& connection) override {
    Log::trace(connection, "Sendfile done\n");
  };

 private:
  int fd_;
  size_t size_;
};
