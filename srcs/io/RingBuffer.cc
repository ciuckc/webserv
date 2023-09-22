#include "RingBuffer.h"

RingBuffer::RingBuffer(size_t size) : data_(std::make_unique<char[]>(size)), size_(size) {}
RingBuffer::~RingBuffer() = default;

bool RingBuffer::empty() const {
  return empty_;
}
bool RingBuffer::full() const {
  return !empty_ && start_ == end_;
}
size_t RingBuffer::capacity() const {
  return size_;
}
size_t RingBuffer::dataLen() const {
  if (empty_)
    return 0;
  return end_ - start_ + (start_ >= end_ ? size_ : 0);
}
size_t RingBuffer::freeLen() const {
  return size_ - dataLen();
}

void RingBuffer::resize(size_t new_size) {
  if (new_size < dataLen()) {
    Log::warn("Possibly losing data from resize\n");
    return; // todo
  }
  auto new_data = std::make_unique<char[]>(new_size);
  if (!empty_) {
    auto copy = dataHead<std::string_view>();
    char* i = std::copy(copy.begin(), copy.end(), new_data.get());
    if (end_ < start_) {
      copy = dataTail<std::string_view>();
      std::copy(copy.begin(), copy.end(), i);
      end_ += size_;
    }
    end_ -= start_;
    start_ = 0;
  }
  data_ = std::move(new_data);
  size_ = new_size;
}

WS::IOStatus RingBuffer::read_sock(Socket& sock) {
  return read(sock.get_fd());
}

WS::IOStatus RingBuffer::read(int fd) {
  iovec vecs[2];
  auto vec_n = getInVecs(vecs);
  ssize_t read = readv(fd, vecs, vec_n);
  if (read < 0)
    return WS::IO_FAIL;
  if (read == 0)
    return WS::IO_EOF;
  grow(read);
  return WS::IO_GOOD;
}

WS::IOStatus RingBuffer::read(int fd, size_t& max) {
  iovec vecs[2];
  auto vec_n = getInVecs(vecs);
  limitVecs(vecs, vec_n, max);
  ssize_t read = readv(fd, vecs, (int)vec_n);
  if (read < 0)
    return WS::IO_FAIL;
  if (read == 0)
    return WS::IO_EOF;
  max -= read;
  grow(read);
  return WS::IO_GOOD;
}

void RingBuffer::put(std::string&& s) {
  put(s);
}

void RingBuffer::put(const char* str) {
  put(std::string_view(str, strlen(str)));
}

WS::IOStatus RingBuffer::write_sock(Socket& sock) {
  return write(sock.get_fd());
}

WS::IOStatus RingBuffer::write(int fd) {
  iovec vecs[3];
  auto vec_n = getOutVecs(vecs);
  ssize_t written = writev(fd, vecs, vec_n);
  if (written < 0)
    return WS::IO_FAIL;
  if (written == 0)
    return WS::IO_EOF;
  size_t data_len = dataLen();
  shrink(std::min((size_t) written, data_len));
  if ((size_t) written > data_len)
    overflow_ = overflow_.substr(written - data_len);
  if (!overflow_.empty())
    put(overflow_);
  return WS::IO_GOOD;
}

WS::IOStatus RingBuffer::write(int fd, size_t& max) {
  iovec vecs[3];
  auto vec_n = getOutVecs(vecs);
  limitVecs(vecs, vec_n, max);
  ssize_t written = writev(fd, vecs, (int)vec_n);
  if (written < 0)
    return WS::IO_FAIL;
  if (written == 0)
    return WS::IO_EOF;
  max -= written;
  size_t data_len = dataLen();
  shrink(std::min((size_t) written, data_len));
  if ((size_t) written > data_len)
    overflow_ = overflow_.substr(written - data_len);
  if (!overflow_.empty())
    put(std::move(overflow_));
  return WS::IO_GOOD;
}

bool RingBuffer::getline(std::string& dst) {
  if (empty_)
    abort();

  std::string_view strs[3];
  auto chunk_n = getOutVecs(strs);
  for (auto i = 0; i < chunk_n; i++) {
    size_t idx = strs[i].find_first_of('\n');
    if (idx != std::string::npos) {
      size_t len = idx + 1;
      for (auto j = 0; j < i; j++)
        len += strs[j].size();
      dst.reserve(len);
      dst.clear();
      for (auto j = 0; j < i; j++)
        dst.append(strs[j]);
      dst.append(strs[i].substr(0, idx + 1));
      shrink(dst.size());
      return true;
    }
  }
  return false;
}

void RingBuffer::discard(size_t& remaining) {
  size_t len = dataLen();
  size_t n = std::min(remaining, len + overflow_.size());
  remaining -= n;
  shrink(n);
  if (len < n)
    overflow_ = overflow_.substr(n - len);
  if (!overflow_.empty())
    put(std::move(overflow_));
}

void RingBuffer::shrink(size_t by) {
  start_ += by;
  if ((start_ %= size_) == end_) {
    start_ = end_ = 0;
    empty_ = true;
  }
}

void RingBuffer::grow(size_t by) {
  empty_ = false;
  end_ += by;
  end_ %= size_;
}


bool RingBuffer::dataSplit() const {
  return !empty() && end_ < start_ && end_ != 0;
}
bool RingBuffer::freeSplit() const {
  return !full() && end_ > start_ && start_ != 0;
}

void RingBuffer::limitVecs(iovec* vecs, int& vec_n, size_t max) {
  for (auto i = 0; i < vec_n; i++) {
    if (vecs[i].iov_len >= max) {
      vecs[i].iov_len = max;
      vec_n = i + 1;
      break;
    }
    max -= vecs[i].iov_len;
  }
}
