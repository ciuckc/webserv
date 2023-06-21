#include "ConnectionBuffer.h"

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

size_t ConnectionBuffer::available(WS::Dir direction) const {
  if (direction == WS::OUT) {
    if (o_bufs_.empty())
      return 0;
    return size_ - (o_offset_ % size_);
  } else {
    if (i_bufs_.empty())
      return 0;
    return i_end_ - i_offset_;
  }
}

// =========== IN ===========
WS::IOStatus ConnectionBuffer::readIn(Socket& socket) {
  ssize_t to_read = static_cast<ssize_t>(i_end_ % size_);
  if (to_read == 0) {  // We've already fully filled the buffers
    i_bufs_.push_back(pool_.getBuffer());
    to_read += (int)size_;
  }
  BufferPool<>::buf_t& buf = i_bufs_.back().getData();
  char* dst = &buf[size_ - to_read];
  ssize_t readed = socket.read(dst, to_read);
  if (readed < 0)
    return WS::ERR;
  read_fail_ = false;
  i_end_ += readed;
  if (readed < to_read)
    return WS::FULL;
  return WS::OK;
}
ConnectionBuffer& ConnectionBuffer::getline(std::string& str) {
  size_t old_offs = i_offset_;
  size_t offs = old_offs;

  for (auto buf_iter = i_bufs_.begin(); buf_iter != i_bufs_.end(); ++buf_iter) {
    auto char_iter = buf_iter->getData().begin() + (buf_iter == i_bufs_.begin() ? i_offset_ : 0);
    while (char_iter != buf_iter->getData().end()) {
      if (*char_iter == '\n') {
        str = std::string();
        auto app_iter = i_bufs_.begin();
        do {
          of

        } while (&*(app_iter++) != &buf_iter);
        auto app_iter = i_bufs_.begin();
        while (app_iter <= buf_iter) {

        }

      }
      ++offs;
    }
  }
  size_t i = 0;
  size_t block_i = i_offset_;
  size_t aval = available(WS::IN);
  auto buf_iter = i_bufs_.begin();
  while (i < aval) {
    if (block_i == size_) {
      block_i = 0;
      ++buf_iter;
    }
    char c = buf_iter->getData()[block_i];
    if (c == '\n')
      break;
    ++i;
    ++block_i;
  }
  if (i < aval)
    str = get_str(i + 1);
  else
    read_fail_ = true;
  return *this;
}
std::string ConnectionBuffer::get_str(size_t len) {
  std::string str(len, '\0');
  size_t str_idx = 0;
  while (len > 0) {
    char* data = i_bufs_.front().getData() + i_offset_;
    size_t to_get = std::min(len, size_ - (i_offset_ % size_));
    if (to_get == 0)
      to_get = size_;
    str.replace(str_idx, to_get, data, to_get);
    str_idx += to_get;
    len -= to_get;
    i_offset_ += to_get;
    if (i_offset_ >= size_)
      pop_inbuf();
  }
  return str;
}
void ConnectionBuffer::pop_inbuf() {
  i_bufs_.pop_front();
  if (i_offset_ < size_) {
    std::cerr << "uhh, popping last input buffer?\n";
    i_offset_ = i_end_ = 0;
  } else {
    i_offset_ -= size_;
    i_end_ -= size_;
  }
}

// =========== OUT ==========
WS::IOStatus ConnectionBuffer::writeOut(Socket& socket) {
  while (!o_bufs_.empty()) {
    auto& buffer = o_bufs_.front();
    char* data = buffer.getData() + o_start_;
    ssize_t to_write = static_cast<ssize_t>(std::max(size_, o_offset_) - o_start_);
    ssize_t written = socket.write(data, to_write, 0);
    if (written < 0)
      return WS::ERR;
    if (written < to_write) {
      o_start_ += written;
      return WS::FULL;
    }
    o_offset_ -= to_write - o_start_;
    o_start_ = 0;
    o_bufs_.pop_front();
  }
  need_write_ = false;
  return WS::OK;
}
void ConnectionBuffer::put(const char* data, size_t len) {
  if (o_bufs_.empty()) {
    o_bufs_.push_back(pool_.getBuffer());
    o_offset_ = o_start_ = 0;
  }
  size_t to_wr = std::min(len, available(WS::OUT));
  char* dst = o_bufs_.back().getData() + (o_offset_ % size_);
  std::memcpy(dst, data, to_wr);
  o_offset_ += to_wr;
  if (len > to_wr)
    overflow(data + to_wr, len - to_wr);
}
void ConnectionBuffer::overflow(const char* data, size_t len) {
  if (need_write_)
    std::cerr << "You're writing into a buffer that has already overflowed before..\n";
  need_write_ = true;
  o_bufs_.push_back(pool_.getBuffer());
  put(data, len);
}
