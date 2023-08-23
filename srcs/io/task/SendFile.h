#pragma once

#include "IOTask.h"

class SendFile : public OTask {
 public:
  explicit SendFile(int fd) : fd_(fd) {};
  bool operator()(Connection& connection) override;
  void onDone(Connection& connection) override;
 protected:
  ~SendFile() override { close(fd_); }
 private:
  static constexpr size_t bufsize = 0x200000; // todo: get all these defined somewhere so they're easily changeable
  int fd_;
  std::array<char, bufsize> filebuf_;
  size_t filebuf_offs_ = 0;
  size_t filebuf_len_ = 0;
  bool eof_ = false;
};