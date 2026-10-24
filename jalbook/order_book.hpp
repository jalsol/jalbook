#pragma once

#include <cassert>
#include <cstddef>

#include "absl/container/flat_hash_map.h"
#include "event.hpp"
#include "order_pool.hpp"

class OrderBook {
 public:
  // Avoid rehashing in the hot path; live orders peak in the low thousands.
  static constexpr std::size_t kReservedOrders = 1 << 14;

  OrderBook() { id_to_order_.reserve(kReservedOrders); }

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
      case EventType::kHidden:
      case EventType::kHalt:
        return;
    }
    std::unreachable();
  }

 private:
  friend struct OrderBookTest;

  void Add(const Event& event) {
    Order* order = order_pool_.Allocate();
    id_to_order_[event.id] = order;

    Level* level = &levels_[event.price];
    *order = Order{
        .id = event.id,
        .size = event.size,
        .level = level,
    };
    level->AddOrder(order);
  }

  void Cancel(const Event& event) {
    if (Order* order = FindOrder(event.id)) {
      Reduce(order, event.size);
    }
  }

  void Delete(const Event& event) {
    if (Order* order = FindOrder(event.id)) {
      RemoveOrder(order);
    }
  }

  void Execute(const Event& event) {
    if (Order* order = FindOrder(event.id)) {
      Reduce(order, event.size);
    }
  }

  Order* FindOrder(std::uint64_t id) {
    auto it = id_to_order_.find(id);
    return it == id_to_order_.end() ? nullptr : it->second;
  }

  void Reduce(Order* order, std::uint64_t size) {
    order->size -= size;

    if (order->size == 0) {
      RemoveOrder(order);
    }
  }

  void RemoveOrder(Order* order) {
    order->level->DeleteOrder(order);

    id_to_order_.erase(order->id);
    order_pool_.Deallocate(order);
  }

  static constexpr std::uint64_t kMaxPrice = 1 << 16;

  std::array<Level, kMaxPrice> levels_{};
  OrderPool<kMaxPrice * 10> order_pool_{};
  absl::flat_hash_map<std::uint64_t, Order*> id_to_order_;
};
