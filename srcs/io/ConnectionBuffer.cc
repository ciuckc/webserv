#include "ConnectionBuffer.h"

#include <algorithm>

ConnectionBuffer::ConnectionBuffer(BufferPool<>& pool)
    : pool_(pool),
      size_(pool.size()),
      i_offset_(),
      i_end_(),
      read_fail_(true),
      o_start_(),
      o_offset_(),
      need_write_() {}

ConnectionBuffer::~ConnectionBuffer() {
  i_bufs_.clear();
  o_bufs_.clear();
}

// =========== IN ===========
WS::IOStatus ConnectionBuffer::readIn(Socket& socket) {
  ssize_t to_read = (ssize_t)toBuffer(size_ - i_end_);
  if (to_read == 0) {  // We've already fully filled the buffers
    i_bufs_.emplace_back(pool_.getBuffer());
    to_read += (ssize_t)size_;
    pool_.log_info();
  }
  auto& buf = i_bufs_.back().getData();
  ssize_t readed = socket.read(&buf[size_ - to_read], to_read);
  // todo: check requirements for this:
  //  according to the eval sheet we need to check <=
  //  but what if our last read call returned IO_GOOD by emptying the
  //  socket buffer while simultaneously filling the input buffer?
  //  Same applies for writeOut
  if (readed <= 0)
    return WS::IO_FAIL;
  read_fail_ = false;
  i_end_ += readed;
  if (readed < to_read)
    return WS::IO_WAIT;
  return WS::IO_GOOD;
}

ConnectionBuffer& ConnectionBuffer::getline(std::string& str) {
  str = std::string();

  size_t old_offs = i_offset_;
  size_t offs = old_offs;

  for (auto buf_iter = i_bufs_.begin(); buf_iter != i_bufs_.end(); ++buf_iter) {
    bool first_iteration = buf_iter == i_bufs_.begin();
    bool last_iteration = buf_iter == std::prev(i_bufs_.end());

    auto& arr = buf_iter->getData();
    char* buf_begin = arr.begin() + (first_iteration ? i_offset_ : 0);
    char* buf_end = last_iteration ? arr.begin() + toBuffer(i_end_) : arr.end();
    char* found_nl = std::find(buf_begin, buf_end, '\n');

    const bool done = found_nl != buf_end;
    offs += std::distance(buf_begin, found_nl) + done;
    if (done) {
      str = get_str(offs - old_offs);
      return *this;
    }
  }
  read_fail_ = true;
  return *this;
}

std::string ConnectionBuffer::get_str(size_t len) {
  std::string str(len, '\0');
  size_t pos = 0;
  while (len > 0) {
    auto& buf = i_bufs_.front().getData();
    size_t to_get = std::min(len, size_ - i_offset_);
    str.replace(pos, to_get, &buf[i_offset_], to_get);
    pos += to_get; len -= to_get;
    i_offset_ += to_get;
    if (i_offset_ >= size_)
      pop_inbuf();
  }
  return str;
}

void ConnectionBuffer::pop_inbuf() {
  i_bufs_.pop_front();
  if (i_offset_ < size_) {
    Log::warn("uhh, popping last input buffer?\n");
    i_offset_ = i_end_ = 0;
  } else {
    i_offset_ -= size_;
    i_end_ -= size_;
  }
}

// =========== OUT ==========
WS::IOStatus ConnectionBuffer::writeOut(Socket& socket) {
  while (!o_bufs_.empty()) {
    auto& buffer = o_bufs_.front().getData();
    ssize_t to_write = (ssize_t)(std::min(size_, o_offset_) - o_start_);
    ssize_t written = socket.write(&buffer[o_start_], to_write, 0);
    if (written <= 0)
      return WS::IO_FAIL;
    if (written < to_write) {
      o_start_ += written;
      return WS::IO_WAIT;
    }
    o_offset_ -= to_write;
    o_start_ = 0;
    o_bufs_.pop_front();
  }
  need_write_ = false;
  return WS::IO_GOOD;
}

void ConnectionBuffer::put(const char* data, size_t len) {
  if (o_bufs_.empty()) {
    o_bufs_.emplace_back(pool_.getBuffer());
    o_offset_ = o_start_ = 0;
  }

  BufferPool<>::buf_t& buf = o_bufs_.back().getData();
  const auto buf_ofs = toBuffer(o_offset_);
  size_t copy_len = std::min(len, size_ - buf_ofs);

  std::memcpy(&buf[buf_ofs], data, copy_len);

  o_offset_ += copy_len;
  if (len > copy_len)
    overflow(data + copy_len, len - copy_len);
}

void ConnectionBuffer::overflow(const char* data, size_t len) {
  if (need_write_)
    Log::warn("You're writing into a buffer that has already overflowed before..\n");
  need_write_ = true;
  o_bufs_.emplace_back(pool_.getBuffer());
  pool_.log_info();
  put(data, len);
}
