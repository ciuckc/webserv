#pragma once

#include <bits/types/struct_iovec.h>

#include <cstdint>
#include <iostream>
#include <list>
#include <sstream>
#include <streambuf>
#include <array>
#include <memory>

template<size_t size_ = 8192> class BufferPool {
 public:
  BufferPool() = default;
  ~BufferPool() = default;

  typedef std::array<char, size_> buf_t;
  struct Buf;

  Buf getBuffer();
  size_t size() const;

  void log_info() const;

 private:
  std::list<buf_t> buffers_;
  uint32_t buffer_count_ = 0;

 public:
  struct Buf {
   private:
    std::list<buf_t>& buf_return_;
    buf_t data_;

   public:
    explicit Buf(std::list<buf_t>& cart_return)
      : buf_return_(cart_return), data_(std::move(cart_return.front())) {
      cart_return.pop_front();
    };
    ~Buf() { buf_return_.push_front(std::move(data_)); }
    inline buf_t& getData() { return data_; };
    inline buf_t& operator()() { return data_; };
  };
};


template<size_t size_>
typename BufferPool<size_>::Buf BufferPool<size_>::getBuffer() {
  if (buffers_.empty()) {
    buffers_.push_front(buf_t());
    ++buffer_count_;
  }
  return Buf(buffers_);
}

template<size_t size_>
inline size_t BufferPool<size_>::size() const { return size_; }

template<size_t size_>
void BufferPool<size_>::log_info() const {
  size_t used_bufs = buffer_count_ - buffers_.size();
  float total_mb = (float) (size_ * buffer_count_) / (1024.f * 1024.f);
  float used_mb = (float) (size_ * used_bufs) / (1024.f * 1024.f);
  std::cout << "Buffer pool: "
               "Alloc: " << buffer_count_ << " (" << total_mb << "M)"
               "Used: " << used_bufs << " (" << used_mb << "M)" << '\n';
}
