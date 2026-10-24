#pragma once

#include "event.hpp"
#include "order_pool.hpp"

class OrderBook {
 public:
  void HandleEvent(const Event& event) {
    switch (event.event_type) {
      case EventType::kAdd:
        Add(event);
        return;
      case EventType::kCancel:
        Cancel(event);
        return;
      case EventType::kDelete:
        Delete(event);
        return;
      case EventType::KExecute:
        Execute(event);
        return;
      case EventType::kTrade:
        Trade(event);
        return;
      case EventType::KHidden:
      case EventType::kHalt:
        return;
    }
    std::unreachable();
  }

 private:
  void Add(const Event& event) {
    Order* order = order_pool_.allocate();  // TODO: map order id -> order object
    Level* level = &levels_[event.price];
    *order = Order{
        .id = event.id,
        .size = event.size,
        .level = level,
    };
    level->AddOrder(order);
  }

  void Cancel(const Event&) {}
  void Delete(const Event&) {}
  void Execute(const Event&) {}
  void Trade(const Event&) {}

  static constexpr std::uint64_t kMaxPrice = 1 << 16;

  std::array<Level, kMaxPrice> levels_{};
  OrderPool<kMaxPrice * 10> order_pool_{};
};
