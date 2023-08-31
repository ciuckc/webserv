#include "ConnectionBuffer.h"

#include <algorithm>

ConnectionBuffer::ConnectionBuffer(BufferPool<>& pool)
    : pool_(pool),
      size_(BufferPool<>::size()),
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
  auto to_read = (ssize_t)toBuffer(size_ - i_end_);
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
    bool first = buf_iter == i_bufs_.begin();
    bool last  = buf_iter == std::prev(i_bufs_.end());

    auto buf_view = buf_iter->getView(
        first ? i_offset_ : 0,
        last ? toBuffer(i_end_) : std::string::npos);
    size_t newline = buf_view.find('\n');

    offs += std::min(newline, buf_view.size());
    if (newline != std::string::npos) {
      str = get_str(offs - old_offs + 1);
      return *this;
    }
  }
  read_fail_ = true;
  return *this;
}

size_t ConnectionBuffer::discard(size_t n) {
  size_t in_avail = i_end_ - i_offset_ + 1;

  if (in_avail >= n) {
    i_offset_ += n;
    while (i_offset_ >= size_)
      pop_inbuf();
    return 0;
  }
  i_offset_ = i_end_ = 0;
  i_bufs_.clear();
  read_fail_ = true;
  return n - in_avail;
}

std::string ConnectionBuffer::get_str(size_t len) {
  std::string str(len, '\0');
  size_t pos = 0;
  while (len > 0) {
    auto view = i_bufs_.front().getView(i_offset_, len);
    auto to_get = std::min(len, view.size());
    str.replace(pos, len, view);
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
    i_offset_ = i_end_ = 0;
    read_fail_ = true;
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
  if (o_offset_ >= size_)
    need_write_ = true;
}

void ConnectionBuffer::overflow(const char* data, size_t len) {
  if (need_write_)
    Log::warn("You're writing into a buffer that has already overflowed before..\n");
  need_write_ = true;
  o_bufs_.emplace_back(pool_.getBuffer());
  pool_.log_info();
  put(data, len);
}

bool ConnectionBuffer::readFrom(int fd) {
  size_t buf_count = o_bufs_.size();
  if (buf_count == 0) {
    o_bufs_.emplace_back(pool_.getBuffer());
    o_offset_ = o_start_ = 0;
    buf_count = 1;
  }

  size_t read_len = size_ - toBuffer(o_offset_);
  char* buf_ptr = &o_bufs_.back().getData().at(size_ - read_len);
  while (true) {
    ssize_t read_bytes = read(fd, buf_ptr, read_len);
    if (read_bytes < 0)
      throw IOException("Borked file bro");
    o_offset_ += read_bytes;
    if ((size_t)read_bytes < read_len) {
      need_write_ = true;
      // end of file
      return true;
    }
    if (++buf_count > max_file_bufs_) {
      need_write_ = true;
      // send some first
      return false;
    }
    o_bufs_.emplace_back(pool_.getBuffer());
    buf_ptr = o_bufs_.back().getData().begin();
    read_len = size_;
  }
}

void ConnectionBuffer::writeTo(int fd, size_t& remaining) {
  while (remaining && i_offset_ < i_end_) {
    auto& buf = i_bufs_.front().getData();
    size_t to_write = std::min(std::min(size_, i_end_) - i_offset_, remaining);
    ssize_t written = write(fd, &buf.at(i_offset_), to_write);
    if (written == -1)
      throw IOException("That's messed up");
    i_offset_ += written; remaining -= written;
    if (i_offset_ == size_ || i_offset_ == i_end_)
      pop_inbuf();
  }
  read_fail_ = true;
}
