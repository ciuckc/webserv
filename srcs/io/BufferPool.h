#pragma once

#include <list>
#include <bits/types/struct_iovec.h>
#include <cstdint>
#include <iostream>
#include <streambuf>
#include <sstream>

class BufferPool {
 private:
  size_t buffer_size_;
  std::list<char*> buffers_;
  uint32_t buffer_count_;

 public:
  struct BorrowedBuffer {
   private:
    std::list<char*>& owner_;
    char* data_;
   public:
    explicit BorrowedBuffer(std::list<char*>& owner)
    : owner_(owner), data_(owner.front()) {
      owner.pop_front();
    };
    BorrowedBuffer(BorrowedBuffer&& other) : owner_(other.owner_), data_(other.data_) {
      other.data_ = nullptr;
    };
    ~BorrowedBuffer() {
      if (data_ != nullptr)
        owner_.push_back(data_);
    }
    inline char* getData() { return data_; }
  };
  typedef struct BorrowedBuffer buf_t;

  explicit BufferPool(size_t size = 8192) : buffer_size_(size), buffers_(), buffer_count_() {}
  ~BufferPool() {
    if (buffers_.size() != buffer_count_)
      std::cerr << "Buffer pool: There are still buffers in use?!! LEAK!\n";
    for (; !buffers_.empty(); buffers_.pop_front())
      delete[] buffers_.front();
  }

  buf_t getBuffer() {
    if (buffers_.empty()){
      buffers_.push_front(new char[buffer_size_]);
      ++buffer_count_;
    }
    return buf_t(buffers_);
  }

  size_t size() const {
    return buffer_size_;
  }

  void log_info() const {
    size_t used_bufs = buffer_count_ - buffers_.size();
    float total_mb = (float)(buffer_size_ * buffer_count_) / (1024.f * 1024.f);
    float used_mb = (float)(buffer_size_ * used_bufs) / (1024.f * 1024.f);
    std::cout << "Buffer pool: "
                 "Alloc: " << buffer_count_ << " (" << total_mb << "M)"
                 "Used: " << used_bufs << " (" << used_mb << "M)" << '\n';
  }
};