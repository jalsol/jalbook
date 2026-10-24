#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <type_traits>

template <typename T, std::size_t Size>
class SPSCQueue {
  static_assert(std::has_single_bit(Size), "Size must be power of 2");
  static_assert(std::is_trivially_copyable_v<T>);

 public:
  SPSCQueue() = default;
  SPSCQueue(const SPSCQueue&) = delete;
  SPSCQueue(SPSCQueue&&) = delete;
  SPSCQueue operator=(const SPSCQueue&) = delete;
  SPSCQueue operator=(SPSCQueue&&) = delete;

  bool Push(const T& item) {
    const auto tail = tail_.load(std::memory_order_relaxed);
    if (tail - cached_head_ >= Size) {
      cached_head_ = head_.load(std::memory_order_acquire);
      if (tail - cached_head_ >= Size) {
        return false;
      }
    }

    buffer_[tail & kMask] = item;
    tail_.store(tail + 1, std::memory_order_release);
    return true;
  }

  bool Pop(T* item) {
    const auto head = head_.load(std::memory_order_relaxed);
    if (head == cached_tail_) {
      cached_tail_ = tail_.load(std::memory_order_acquire);
      if (head == cached_tail_) {
        return false;
      }
    }

    *item = buffer_[head & kMask];
    head_.store(head + 1, std::memory_order_release);
    return true;
  }

 private:
  static constexpr int kMask = Size - 1;
  static constexpr int kCacheLineSize = 64;

  std::array<T, Size> buffer_;

  alignas(kCacheLineSize) std::atomic<std::size_t> head_{};
  alignas(kCacheLineSize) std::size_t cached_head_{};
  alignas(kCacheLineSize) std::atomic<std::size_t> tail_{};
  alignas(kCacheLineSize) std::size_t cached_tail_{};
};
