#pragma once

#include <string_view>

#include "csv.h"
#include "event.hpp"
#include "spsc_queue.hpp"

template <typename T, std::size_t Size>
class FeedHandler {
 public:
  FeedHandler(SPSCQueue<T, Size>* queue, std::string_view path)
      : queue_{queue}, csv_reader_{path.data()} {}

  FeedHandler(const FeedHandler&) = delete;
  FeedHandler(FeedHandler&&) = delete;
  FeedHandler& operator=(const FeedHandler&) = delete;
  FeedHandler& operator=(FeedHandler&&) = delete;

  void Run() {
    Event event{};
    double timestamp;

    while (csv_reader_.read_row(timestamp, event.event_type, event.id, event.size, event.price,
                                event.direction)) {
      if (event.IsBookRelevant()) {
        event.price /= 100;  // LOBSTER multiplies by 10000 while 100 suffices
        while (!queue_->Push(event));
      }
    }

    event.event_type = EventType::kHalt;
    event.direction = Direction::kSell;
    while (!queue_->Push(event));
  }

 private:
  SPSCQueue<T, Size>* queue_;
  io::CSVReader<6> csv_reader_;
};
