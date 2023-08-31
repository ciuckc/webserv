#pragma once
#include <streambuf>

#include "BufferPool.h"
#include "Socket.h"
#include "util/WebServ.h"

class ConnectionBuffer {
 private:
  // read a bit more from a file, less syscalls. 64kb may be a nice amount
  static constexpr size_t max_file_bufs_ = 0xFFFF / BufferPool<>::size();
  // To reuse buffers instead of allocating new ones every time
  BufferPool<>& pool_;
  // The size of every buffer
  size_t size_;

  // Input buffers (that are read into, and we can get lines out of)
  std::list<BufferPool<>::Buf> i_bufs_;
  // The current input position
  size_t i_offset_;
  // The end of the input sequence
  size_t i_end_;
  // Is there more stuff in the buffer we can read?
  bool read_fail_;

  // Output buffers (that are written out of, and we can put stuff in)
  std::list<BufferPool<>::Buf> o_bufs_;
  // The first unwritten output index
  size_t o_start_;
  // The current output position
  size_t o_offset_;
  // Is at least one buffer full?
  bool need_write_;

  // =========== IN ===========
  // Create a string of certain length from the buffer
  // does not check bounds!
  std::string get_str(size_t len);
  // Gets called when an input buffer is used up completely
  void pop_inbuf();

  // =========== OUT ==========
  // Write some data of certain length into the buffer
  void put(const char* data, size_t len);
  // Gets called when an input operation was bigger than the current buffer
  // after this, needWrite returns true, and writeOut should probably be called
  void overflow(const char* data, size_t len);

  inline size_t toBuffer(size_t idx) const {
    return idx & (size_ - 1);
  };
 public:
  explicit ConnectionBuffer(BufferPool<>& pool);
  virtual ~ConnectionBuffer();

  // =========== IN ===========
  // Read from socket into buffer. Should be only called if needRead is true and
  // the socket is ready for reading
  WS::IOStatus readIn(Socket& socket);
  // Did the last operation getting data from the buffer fail? (need more chars!)
  inline bool readFailed() const { return read_fail_; };
  // Get the next line (including \n) from the buffer, sets read_fail if no newline
  // has been found
  ConnectionBuffer& getline(std::string& str);
  // Discard n bytes of data
  size_t discard(size_t n);

  // =========== OUT ==========
  // Write from buffer into socket. Should only be called when the socket is
  // ready for writing
  WS::IOStatus writeOut(Socket& socket);
  // Do we have a full buffer or old data waiting to be written?
  inline bool needWrite() const { return need_write_; }
  inline size_t outAvailable() const { return size_ - o_offset_ + 1; }
  // Read from a file descriptor into the output buffer (sendfile)
  // reads up to max_file_bufs buffers
  // returns true when eof is reached
  bool readFrom(int fd);
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
