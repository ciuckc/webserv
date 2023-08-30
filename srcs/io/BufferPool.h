#pragma once

#include <list>
#include <array>
#include <cstdint>

#include "util/Log.h"

template<size_t size_ = 8192> class BufferPool {
 public:
  BufferPool() = default;
  ~BufferPool() = default;

  using buf_t = std::array<char, size_>;
  struct Buf;

  Buf getBuffer();
  static constexpr size_t size() { return size_; };

  void log_info() const;

 private:
  std::list<buf_t> buffers_;
  uint32_t buffer_count_ = 0;

 public:
  struct Buf {
   private:
    std::list<buf_t>& buf_return_;
    buf_t data_;
    bool valid_ = true;

   public:
    explicit Buf(std::list<buf_t>& cart_return)
      : buf_return_(cart_return), data_(std::move(cart_return.front())) {
      cart_return.pop_front();
    };
    Buf(Buf&& other) noexcept
      : buf_return_(other.buf_return_), data_(std::move(other.data_)) {
      other.valid_ = false;
    }
    ~Buf() {
      if (valid_)
        buf_return_.push_front(std::move(data_));
    }
    inline buf_t& getData() { return data_; };
    inline buf_t& operator()() { return data_; };
    inline std::string_view getView(size_t ofs, size_t len = std::string::npos) {
      return {data_.data() + ofs, std::min(len, size_ - ofs)};
    }
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
void BufferPool<size_>::log_info() const {
  if (Log::log_level != Log::TRACE)
    return;
  size_t used_bufs = buffer_count_ - buffers_.size();
  float total_mb = (float) (size_ * buffer_count_) / (1024.f * 1024.f);
  float used_mb = (float) (size_ * used_bufs) / (1024.f * 1024.f);
  Log::trace("Buffer pool: "
             "Alloc: ", buffer_count_, " (", total_mb, "M)"
             "Used: ", used_bufs, " (", used_mb, "M)", '\n');
}
