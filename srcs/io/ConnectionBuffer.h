#pragma once
#include <array>
#include <list>
#include <memory>

#include "Socket.h"
#include "util/WebServ.h"

class ConnectionBuffer {
 private:
  static constexpr size_t buf_size_ = 0x4000; // 16 kb
  // read a bit more from a file, less syscalls.
  static constexpr size_t file_buf_size_ = 0x10000; // 64 kb

  class Buf {
   public:
    explicit Buf(size_t size)
      : size(size), data_(std::make_unique<char[]>(size)) {}
    ~Buf() = default;
    Buf(Buf&& other) = default;

    using data_t = std::unique_ptr<char[]>;
    size_t size;

    inline size_t len() const {
      return data_end_ - data_start_;
    }
    inline size_t avail() const {
      return size - data_end_;
    }
    inline bool full() const {
      return data_end_ == size;
    }
    inline bool empty() const {
      return data_end_ == 0;
    }
    inline char* start() {
      return data_.get() + data_start_;
    }
    inline char* end() {
      return data_.get() + data_end_;
    }
    template<typename int_t> inline int_t shrink(int_t n) {
      int_t by = std::min((int_t)len(), n);
      data_start_ += by;
      if (data_start_ == data_end_)
        data_start_ = data_end_ = 0;
      return n - by;
    }
    template<typename int_t> inline int_t stretch(int_t n) {
      int_t by = std::min(n, (int_t)avail());
      data_end_ += by;
      return n - by;
    }
    inline void reset() {
      data_start_ = data_end_ = 0;
    }
    inline std::string_view getView() const {
      return {data_.get() + data_start_, len()};
    }
    inline std::string_view getView(size_t len) {
      return {data_.get() + data_start_, std::min(len, this->len())};
    }
    inline iovec asOutVec() {
      return {start(), len()};
    }
    inline iovec asOutVec(size_t& max) {
      size_t n = std::min(len(), max);
      max -= n;
      return {start(), n};
    }
    inline iovec asInVec(size_t& len) {
      size_t n = std::min(avail(), len);
      len -= n;
      return {end(), n};
    }
    inline bool operator==(const Buf& other) const {
      return this == &other;
    }
    inline bool operator!=(const Buf& other) const {
      return !(*this == other);
    }
    inline bool read_file(int fd, size_t& n) {
      ssize_t bytes_rd = read(fd, end(), n);
      if (bytes_rd < 0)
        throw IOException("Can't read from file :(\n");
      stretch(bytes_rd);
      return (n -= bytes_rd) == 0;
    }
    void resize(size_t new_size) { // does not copy any data!
      size = new_size;
      data_ = std::make_unique<char[]>(size);
    }
   private:
    data_t data_;
    size_t data_start_ = 0;
    size_t data_end_ = 0;
  };

  size_t i_size_ = buf_size_;
  size_t o_size_ = buf_size_;
  // Input buffers (that are read into, and we can get lines out of)
  std::list<Buf> i_bufs_;
  // Output buffers (that are written out of, and we can put stuff in)
  std::list<Buf> o_bufs_;
  // Is there more stuff in the socket buffer we can read?
  bool need_read_ = true;
  // Is at least one output buffer full?
  bool need_write_ = false;
  // =========== OUT ==========
  // Write some data of certain length into the buffer
  void put(const char* data, size_t len);

 public:
  ConnectionBuffer() = default;
  ~ConnectionBuffer() = default;

  // =========== IN ===========
  // Read from socket into buffer. Should be only called if needRead is true and
  // the socket is ready for reading
  bool readIn(Socket& socket, WS::IOStatus& status);
  // Did the last operation getting data from the buffer fail? (need more chars!)
  inline bool needRead() const { return need_read_ || inEmpty(); };
  inline bool inEmpty() const { return i_bufs_.empty() || i_bufs_.front().empty(); }
  inline bool outEmpty() const { return o_bufs_.empty() || o_bufs_.front().empty(); }
  inline bool outFull() const { return !o_bufs_.empty() && o_bufs_.front().full(); }
  inline void setInSize(size_t n) { i_size_ = n; }
  inline void setOutSize(size_t n) { o_size_ = n; }
  // Get the next line (including \n) from the buffer, sets read_fail if no newline
  // has been found
  bool getline(std::string& str);
  // Discard n bytes of data
  size_t discard(size_t n);

  // =========== OUT ==========
  // Write from buffer into socket. Should only be called when the socket is
  // ready for writing
  bool writeOut(Socket& socket, WS::IOStatus& status);
  // Do we have a full buffer or old data waiting to be written?
  inline bool needWrite() const { return need_write_; }
  inline size_t outAvailable() const { return o_bufs_.empty() ? o_size_ : o_bufs_.back().avail(); }
  // Read from a file descriptor into the output buffer (sendfile)
  // reads up to max_file_bufs buffers
  // returns true when eof is reached
  bool readFrom(int fd, size_t& filesize);
  // Read from the input buffer into a fd (recvfile)
  void writeTo(int fd, size_t& remaining);

  inline ConnectionBuffer& operator<<(const std::string& str) {
    put(str.c_str(), str.size());
    return *this;
  }
  inline ConnectionBuffer& operator<<(const std::string_view& str) {
    put(str.data(), str.size());
    return *this;
  }
  inline ConnectionBuffer& operator<<(const char* str) {
    put(str, std::strlen(str));
    return *this;
  }
  inline ConnectionBuffer& operator<<(char c) {
    put(&c, 1);  // lol
    return *this;
  }
  inline ConnectionBuffer& operator<<(int num) { return operator<<(std::to_string(num)); }
};
