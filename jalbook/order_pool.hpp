#pragma once

#include <array>

#include "order.hpp"

template <std::size_t Capacity>
class OrderPool {
  static_assert(Capacity > 0);

 public:
  OrderPool() {
    for (auto i = 0uz; i + 1 < Capacity; ++i) {
      orders_[i].next = &orders_[i + 1];
    }

    free_head_ = &orders_[0];
  }

  [[nodiscard]] Order* Allocate() noexcept {
    if (free_head_ == nullptr) return nullptr;

    Order* ret = free_head_;
    free_head_ = free_head_->next;
    return ret;
  }

  void Deallocate(Order* order) noexcept {
    order->next = free_head_;
    free_head_ = order;
  }

  OrderPool(const OrderPool&) = delete;
  OrderPool(OrderPool&&) = delete;
  OrderPool& operator=(const OrderPool&) = delete;
  OrderPool& operator=(OrderPool&&) = delete;

 private:
  std::array<Order, Capacity> orders_{};
  Order* free_head_{};
};
