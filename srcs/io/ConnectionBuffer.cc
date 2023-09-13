#include "ConnectionBuffer.h"

#include <algorithm>
#include <cassert>

// =========== IN ===========
bool ConnectionBuffer::readIn(Socket& socket, WS::IOStatus& status) {
  if (i_bufs_.empty() || i_bufs_.back().full())
    i_bufs_.emplace_back(i_size_);
  Buf& buf = i_bufs_.back();
  ssize_t available = (ssize_t)buf.avail();
  ssize_t bytes_rd = socket.read(buf.end(), available);
  if (bytes_rd < 0)
    return status = WS::IO_FAIL, false;
  if (bytes_rd == 0)
    return status = WS::IO_WAIT, false;
  buf.stretch(bytes_rd);
  status = bytes_rd < available ? WS::IO_WAIT : WS::IO_GOOD;
  need_read_ = false;
  return true;
}

bool ConnectionBuffer::getline(std::string& str) {
  assert(!i_bufs_.empty() && !i_bufs_.front().empty());
  size_t nl = i_bufs_.back().getView().find('\n');
  if (nl == std::string::npos) {
    need_read_ = true;
    return false;
  }

  size_t linelen = nl + 1;
  for (auto it = i_bufs_.begin(); it != std::prev(i_bufs_.end()); ++it)
    linelen += it->len();
  str.clear();
  str.reserve(linelen);
  while (linelen != 0) {
    Buf& buf = i_bufs_.front();
    auto view = buf.getView(linelen);
    str.append(view);
    linelen = buf.shrink(linelen);
    if (buf.empty() && buf != i_bufs_.back())
      i_bufs_.pop_front();
  }
  need_read_ = i_bufs_.front().empty();
  return true;
}

size_t ConnectionBuffer::discard(size_t n) {
  while (n != 0) {
    n = i_bufs_.front().shrink(n);
    if (i_bufs_.front().empty() && i_bufs_.front() == i_bufs_.back())
      return n;
  }
  return 0;
}

// =========== OUT ==========
bool ConnectionBuffer::writeOut(Socket& socket, WS::IOStatus& status) {
  const size_t bufcount = o_bufs_.size();
  assert(bufcount != 0);
  iovec vec[bufcount];
  size_t i = 0;
  for (Buf& buf : o_bufs_)
    vec[i++] = buf.asOutVec();
  ssize_t bytes_wr = writev(socket.get_fd(), vec, (int)bufcount);
  if (bytes_wr < 0)
    return status = WS::IO_FAIL, false;
  while (bytes_wr != 0) {
    auto& buf = o_bufs_.front();
    bytes_wr = buf.shrink(bytes_wr);
    if (buf.empty())
      o_bufs_.pop_front();
  }
  if (o_bufs_.empty()) {
    need_write_ = false;
    return status = WS::IO_GOOD, true;
  } else {
    need_write_ = true;
    return status = WS::IO_WAIT, false;
  }
}

void ConnectionBuffer::put(const char* data, size_t len) {
  assert(len != 0);
  if (o_bufs_.empty() || o_bufs_.back().full())
    o_bufs_.emplace_back(o_size_);
  while (len != 0) {
    Buf& buf = o_bufs_.back();
    size_t available = buf.avail();
    size_t copy_len = std::min(available, len);
    std::copy(data, data + copy_len, buf.end());
    buf.stretch(copy_len);
    if (len > available) {
      need_write_ = true;
      o_bufs_.emplace_back(o_size_);
    }
    len -= copy_len; data += copy_len;
  }
}

bool ConnectionBuffer::readFrom(int fd, size_t& filesize) {
  assert(filesize != 0);
  if (o_bufs_.empty())
    o_bufs_.emplace_back(o_size_);
  const auto last_av = o_bufs_.back().avail();
  if (last_av > filesize)
    return o_bufs_.back().read_file(fd, filesize);

  size_t read_len = std::min(filesize, file_buf_size_ + last_av);
  iovec vec[file_buf_size_ / buf_size_ + 1];

  int i = 0;
  if (last_av == 0)
    o_bufs_.emplace_back(o_size_);
  while (true) {
    vec[i++] = o_bufs_.back().asInVec(read_len);
    if (read_len == 0)
      break;
    o_bufs_.emplace_back(o_size_);
  }
  ssize_t bytes_rd = readv(fd, vec, i);
  if (bytes_rd < 0)
    throw IOException("Can't read from file :(\n");
  if (bytes_rd == 0)
    return false;
  filesize -= (size_t)bytes_rd;
  for (auto it = o_bufs_.begin(); bytes_rd != 0; ++it)
    bytes_rd = it->stretch(bytes_rd);
  need_write_ = true;
  return true;
}

void ConnectionBuffer::writeTo(int fd, size_t& remaining) {
  assert(remaining != 0);
  size_t bufcount = i_bufs_.size();
  iovec vec[bufcount];
  size_t max = remaining;
  int i = 0;
  for (Buf& buf : i_bufs_) {
    vec[i++] = buf.asOutVec(max);
    if (max == 0)
      break;
  }
  ssize_t bytes_wr = writev(fd, vec, (int)i);
  if (bytes_wr < 0)
    throw IOException("Couldn't write to file\n");
  if (bytes_wr == 0)
    return;
  remaining -= bytes_wr;
  while (bytes_wr != 0) {
    auto& buf = i_bufs_.front();
    bytes_wr = buf.shrink(bytes_wr);
    if (buf.empty())
      if (buf != i_bufs_.back())
        i_bufs_.pop_front();
  }
  if (i_bufs_.front().empty())
    need_read_ = true;
}
