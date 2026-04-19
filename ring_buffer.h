#include <atomic>
#include <cstdint>
#include <stddef.h>

template <typename T>
class RingBuffer {
public:
  RingBuffer(size_t capacity)
      : capacity_(capacity), buffer_(new T[capacity]), head_(0), tail_(0) {}
  
  ~RingBuffer() { delete[] buffer_; }

  // Producer (Network Thread) calls this
  bool push(const T &item) {
    size_t head = head_.load(std::memory_order_acquire);
    size_t tail = tail_.load(std::memory_order_relaxed);

    if ((tail + 1) % capacity_ == head) {
      return false; // Buffer is full!
    }

    buffer_[tail] = item;
    tail_.store((tail + 1) % capacity_, std::memory_order_release);
    return true;
  }

  bool pop(T &item) {
    size_t tail = tail_.load(std::memory_order_acquire);
    size_t head = head_.load(std::memory_order_relaxed);

    if (head == tail) {
      return false; // Buffer is empty!
    }

    item = buffer_[head];
    head_.store((head + 1) % capacity_, std::memory_order_release);
    return true;
  }

private:
  const size_t capacity_;
  T *const buffer_;
  alignas(64) std::atomic<size_t> head_;
  alignas(64) std::atomic<size_t> tail_;
};