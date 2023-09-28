#pragma once

#include "IOTask.h"
#include "io/Connection.h"
#include "util/Log.h"

class RecvFile : public ITask {
 private:
  std::string path_;
  int fd_;
  size_t remaining_;
  bool new_file_;

  void sendResponse(Connection& connection, bool error) {
    if (error) {
      Log::error(connection, "RecvFile ", path_, " failed\n");
      connection.setKeepAlive(false);
      connection.enqueueResponse(Response::builder().message(500).build());
    } else {
      Log::trace(connection, "RecvFile ", path_, " done\n");
      connection.enqueueResponse(Response::builder()
                                      .message(new_file_ ? 201 : 200)
                                      .header("Content-Location", path_)
                                      .build());
    }
  }

 public:
  RecvFile(std::string path, int fd, size_t size, bool new_file)
      : path_(std::move(path)), fd_(fd), remaining_(size), new_file_(new_file) {};
  ~RecvFile() override { close(fd_); }

  WS::IOStatus operator()(Connection& connection) override {
    auto& buffer = connection.getInBuffer();
    if (buffer.capacity() != RingBuffer::file_buf_size_)
      connection.setInSize(RingBuffer::file_buf_size_);
    if (connection.getInBuffer().write(fd_, remaining_) != WS::IO_GOOD) {
      sendResponse(connection, true);
      return WS::IO_FAIL;
    }
    return remaining_ == 0 ? WS::IO_GOOD : WS::IO_AGAIN;
  };

  void onDone(Connection& connection) override {
    connection.setInSize();
    sendResponse(connection, false);
  };
};
