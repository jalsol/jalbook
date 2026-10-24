#pragma once

#include <pthread.h>
#include <sched.h>
#include <x86intrin.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <system_error>
#include <thread>
#include <utility>

namespace threads {
inline void PinToCpu(std::thread* t, int cpu) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);

  if (auto rc = pthread_setaffinity_np(t->native_handle(), sizeof(cpuset), &cpuset); rc != 0) {
    throw std::system_error(rc, std::generic_category(), "pthread_setaffinity_np");
  }
}
}  // namespace threads

namespace cycles {
// lfence keeps prior work from crossing the timestamp.
[[gnu::always_inline]] inline std::uint64_t Now() noexcept {
  unsigned aux;
  _mm_lfence();
  return __rdtscp(&aux);
}

template <typename F>
[[gnu::always_inline]] inline std::uint64_t Measure(F&& f) {
  const std::uint64_t start = Now();
  std::forward<F>(f)();
  return Now() - start;
}

// Calibrates the TSC against the steady clock; call once before measuring.
inline double Hz() {
  using Clock = std::chrono::steady_clock;

  const auto wall_start = Clock::now();
  const std::uint64_t tsc_start = Now();
  // Busy-wait so the core is not in a low-frequency idle state when the
  // measurement overhead is calibrated.
  const auto deadline = wall_start + std::chrono::milliseconds(100);
  while (Clock::now() < deadline) {
  }
  const std::uint64_t tsc_end = Now();
  const auto wall_end = Clock::now();

  return static_cast<double>(tsc_end - tsc_start) /
         std::chrono::duration<double>(wall_end - wall_start).count();
}

// Minimum cost of an empty Measure; interrupts only inflate a reading.
inline std::uint64_t Overhead() {
  constexpr std::size_t kSamples = 1000;
  std::uint64_t overhead = std::numeric_limits<std::uint64_t>::max();

  for (std::size_t i = 0; i < kSamples; ++i) {
    overhead = std::min(overhead, Measure([] {}));
  }

  return overhead;
}
}  // namespace cycles
