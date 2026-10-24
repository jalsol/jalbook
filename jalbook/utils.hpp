#pragma once

#include <pthread.h>
#include <sched.h>

#include <system_error>
#include <thread>

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
