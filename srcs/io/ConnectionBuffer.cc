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

// =========== IN ===========
WS::IOStatus ConnectionBuffer::readIn(Socket& socket) {
  ssize_t to_read = static_cast<ssize_t>(i_end_ % size_);
  if (to_read == 0) {  // We've already fully filled the buffers
    i_bufs_.push_back(pool_.getBuffer());
    to_read += (int)size_;
  }
  auto& buf = i_bufs_.back().getData();
  ssize_t readed = socket.read(&buf[size_ - to_read], to_read);
  if (readed < 0)
    return WS::ERR;
  read_fail_ = false;
  i_end_ += readed;
  if (readed < to_read)
    return WS::FULL;
  return WS::OK;
}
ConnectionBuffer& ConnectionBuffer::getline(std::string& str) {
  str = std::string();

  size_t old_offs = i_offset_;
  size_t offs = old_offs;

  for (auto buf_iter = i_bufs_.begin(); buf_iter != i_bufs_.end(); ++buf_iter) {
    bool first_iteration = buf_iter == i_bufs_.begin();
    bool last_iteration = buf_iter == std::prev(i_bufs_.end());

    auto& arr = buf_iter->getData();
    auto char_iter = arr.begin() + (first_iteration ? i_offset_ : 0);
    auto char_end = last_iteration ? arr.begin() + (i_end_ % size_) : arr.end();
    auto found_nl = std::find(char_iter, char_end, '\n');

    const bool done = found_nl != char_end;
    offs += std::distance(char_iter, found_nl) + done;
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
    auto& buffer = o_bufs_.front().getData();
    ssize_t to_write = static_cast<ssize_t>(std::max(size_, o_offset_) - o_start_);
    ssize_t written = socket.write(&buffer[o_start_], to_write, 0);
    if (written < 0)
      return WS::ERR;
    if (written < to_write) {
      o_start_ += written;
      return WS::FULL;
    }
    o_offset_ -= to_write + o_start_;
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

  BufferPool<>::buf_t& buf = o_bufs_.back().getData();
  size_t copy_len = std::min(len, size_ - (o_offset_ % size_));

  std::memcpy(&buf[o_offset_ % size_], data, copy_len);

  o_offset_ += copy_len;
  if (len > copy_len)
    overflow(data + copy_len, len - copy_len);
}
void ConnectionBuffer::overflow(const char* data, size_t len) {
  if (need_write_)
    std::cerr << "You're writing into a buffer that has already overflowed before..\n";
  need_write_ = true;
  o_bufs_.push_back(pool_.getBuffer());
  put(data, len);
}
