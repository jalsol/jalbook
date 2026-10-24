#pragma once

#include <cstdint>

#include "csv.h"

enum class EventType {
  kAdd = 1,
  kCancel = 2,
  kDelete = 3,
  KExecute = 4,
  kHidden = 5,
  kTrade = 6,
  kHalt = 7,
};

enum class Direction {
  kSell = -1,
  kBuy = 1,
};

struct Event {
  EventType event_type;
  std::uint64_t id;
  std::uint64_t size;
  std::uint64_t price;
  Direction direction;

  bool IsBookRelevant() const {
    return (event_type != EventType::kHidden) && (event_type != EventType::kTrade) &&
           (event_type != EventType::kHalt);
  }
};

// exploit fast-csv-cpp-reader internal to read enum class
namespace io::detail {

template <>
inline void parse<io::throw_on_overflow, EventType>(char* col, EventType& value) {
  int raw;
  parse<io::throw_on_overflow>(col, raw);
  value = static_cast<EventType>(raw);
}

template <>
inline void parse<io::throw_on_overflow, Direction>(char* col, Direction& value) {
  int raw;
  parse<io::throw_on_overflow>(col, raw);
  value = static_cast<Direction>(raw);
}

}  // namespace io::detail
