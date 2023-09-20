#pragma once

#include "IOTask.h"
#include "io/Connection.h"
#include "util/Log.h"

class SendFile : public OTask {
 public:
  explicit SendFile(int fd, size_t size) : fd_(fd), size_(size) {};
  ~SendFile() override {
    close(fd_);
  }

  WS::IOStatus operator()(Connection& connection) override {
    auto& buffer = connection.getOutBuffer();
    if (buffer.capacity() != RingBuffer::file_buf_size_)
      connection.setOutSize(RingBuffer::file_buf_size_);
    if (buffer.read(fd_, size_) != WS::IO_GOOD) {
      Log::error(connection, "SendFile failed\n");
      return WS::IO_FAIL;
    }
    return size_ == 0 ? WS::IO_GOOD : WS::IO_AGAIN;
  };

  void onDone(Connection& connection) override {
    Log::trace(connection, "Sendfile done\n");
    connection.setOutSize();
  };

 private:
  int fd_;
  size_t size_;
};
