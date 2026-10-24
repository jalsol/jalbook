#pragma once

#include <cstdint>

class Level;  // forward-declare

struct Order {
  std::uint64_t id;
  std::uint64_t size;
  Level* level;

  Order* next = nullptr;
  Order* prev = nullptr;
};

class Level {
 public:
  Level() { sentinel_.next = sentinel_.prev = &sentinel_; }

  Order*& Tail() { return sentinel_.prev; }
  Order*& Head() { return sentinel_.next; }

  void AddOrder(Order* order) {
    order->prev = Tail();
    order->next = &sentinel_;

    Tail()->next = order;
    Tail() = order;
  }

 private:
  Order sentinel_{};
};
