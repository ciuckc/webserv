#pragma once

#include <memory>
#include <sys/uio.h>
#include "util/Log.h"
#include "util/WebServ.h"
#include "Socket.h"

class RingBuffer {
 private:
  std::unique_ptr<char[]> data_;
  size_t size_;
  size_t start_ = 0;
  size_t end_ = 0;
  bool empty_ = true;
  std::string overflow_;

 public:
  static constexpr size_t buf_size_ = 0x4000; // 16 kb
  // read a bit more from a file, less syscalls.
  static constexpr size_t file_buf_size_ = 0x10000; // 64 kb

  explicit RingBuffer(size_t size = buf_size_);
  ~RingBuffer();

  [[nodiscard]] bool empty() const;
  [[nodiscard]] bool full() const;
  /**
   * The total size of this buffer
   */
  [[nodiscard]] size_t capacity() const;
  /**
   * The amount of bytes stored in this buffer
   */
  [[nodiscard]] size_t dataLen() const;
  /**
   * The amount of bytes that can still be stored in this buffer
   */
  [[nodiscard]] size_t freeLen() const;

  /**
   * Change the size of the buffer (moves data to front for good measure)
   */
  void resize(size_t new_size = buf_size_);

  // --------- functions in this section add data to the buffer ---------
  /**
   * Read as many bytes as possible from socket
   */
  WS::IOStatus read_sock(Socket& sock);
  /**
   * Read as many bytes as possible from fd
   */
  WS::IOStatus read(int fd);
  /**
   * Read up to max bytes from fd
   */
  WS::IOStatus read(int fd, size_t& max);
  /**
   * Put a string (convertible to string_view) into the buffer
   * if the string is too big, stores the leftover part in overflow
   */
  template<class str> void put(const str& s);
  void put(std::string&& s);
  void put(const char* str);

  // --------- functions in this section remove data from the buffer ---------
  /**
   * Write as many bytes as possible to socket
   */
  WS::IOStatus write_sock(Socket& sock);
  /**
   * Write as much data as possible to a file
   */
  WS::IOStatus write(int fd);
  /**
   * Write up to max bytes to fd
   */
  WS::IOStatus write(int fd, size_t& max);
  /**
   * Get a line (including '\n') from the buffer
   * returns false if a line wasn't detected
   * if this returns false while the buffer is full, fail please
   */
  bool getline(std::string& dst);
  /**
   * Discard data
   */
  void discard(size_t& remaining);

 private:
  /**
 * Mark `by' bytes of the buffer as empty
 */
  void shrink(size_t by);
  /**
   * Mark `by' bytes of the buffer as full
   */
  void grow(size_t by);

  /*
   * For the next part:
   *   0  dataTail end  freeHead   start  dataHead   size
   *   v     v      v      v         v       v        v
   *   [DTDTDTDTDTDT|FHFHFHFHFHFHFHFH|DHDHDHDHDHDHDHDH]
   *
   *   dataSplit() == true; freeSplit() == false;
   *
   * or
   *
   *   0  freeTail  start  dataHead   end  freeHead  size
   *   v     v        v        v       v       v      v
   *   [FTFTFTFTFTFTFT|DHDHDHDHDHDHDHDH|FHFHFHFHFHFHFH]
   *
   *   dataSplit() == false; freeSplit == true;
   */
  [[nodiscard]] bool dataSplit() const;
  [[nodiscard]] bool freeSplit() const;
  /**
   * Get a {ptr, len} representation of the first part of data available
   */
  template<class vec> vec dataHead();
  /**
   * Get a {ptr, len} representation of the last part of data available (not counting overflow)
   */
  template<class vec> vec dataTail();
  /**
 * Get a {ptr, len} representation of the first part of free space available
 */
  template<class vec> vec freeHead();
  /**
 * Get a {ptr, len} representation of the last part of free space available
 */
  template<class vec> vec freeTail();
  template<class vec> int getInVecs(vec vecs[2]);
  template<class vec> int getOutVecs(vec vecs[3]);
  /**
   * Limit the size of all vecs (combined) to max
   */
  static void limitVecs(iovec vecs[], int& vec_n, size_t max);
};
