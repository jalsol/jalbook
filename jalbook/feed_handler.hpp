#pragma once

#include <iostream>
#include <string_view>

#include "csv.h"
#include "order.hpp"
#include "spsc_queue.hpp"

template <typename T, std::size_t Size>
class FeedHandler {
 public:
  FeedHandler(SPSCQueue<T, Size>* queue, std::string_view path)
      : queue_{queue}, csv_reader_{path.data()} {}

  FeedHandler(const FeedHandler&) = delete;
  FeedHandler(FeedHandler&&) = delete;
  FeedHandler operator=(const FeedHandler&) = delete;
  FeedHandler operator=(FeedHandler&&) = delete;

  void Run() {
    Order order;
    double timestamp;

    int cnt = 0;

    while (csv_reader_.read_row(timestamp, order.event_type, order.id, order.size, order.price,
                                order.direction)) {
      if (order.event_type != EventType::kHalt) [[likely]] {
        while (!queue_->Push(order));
      }

      std::println(std::cerr, "Event {}: type={}", ++cnt, std::to_underlying(order.event_type));
    }

    order.event_type = EventType::kHalt;
    order.direction = Direction::kSell;
    while (!queue_->Push(order));
  }

 private:
  SPSCQueue<T, Size>* queue_;
  io::CSVReader<6> csv_reader_;
};
