#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <memory>

#include "jalbook/event.hpp"
#include "jalbook/feed_handler.hpp"
#include "jalbook/order_book.hpp"
#include "jalbook/spsc_queue.hpp"

TEST_CASE("FeedHandler converts LOBSTER price and size units") {
  {
    std::ofstream csv{"feed_test.csv"};
    csv << "34200.01399412,1,18617948,500,310400,-1\n";
  }

  SPSCQueue<Event, 8> queue;
  FeedHandler<Event, 8> feed{&queue, "feed_test.csv"};
  feed.Run();

  Event event{};
  REQUIRE(queue.Pop(&event));
  CHECK(event.event_type == EventType::kAdd);
  CHECK(event.price == 3104);
  CHECK(event.size == 500);

  REQUIRE(queue.Pop(&event));
  CHECK(event.event_type == EventType::kHalt);
}

TEST_CASE("OrderBook ignores the halt sentinel") {
  auto book = std::make_unique<OrderBook>();  // too big for the stack
  Event halt{};
  halt.event_type = EventType::kHalt;
  book->HandleEvent(halt);
}

struct OrderBookTest {
  static bool Empty(const OrderBook& book) { return book.id_to_order_.empty(); }

  static bool Contains(const OrderBook& book, std::uint64_t id) {
    return book.id_to_order_.contains(id);
  }

  static std::uint64_t Size(const OrderBook& book, std::uint64_t id) {
    return book.id_to_order_.at(id)->size;
  }

  static Order* At(OrderBook& book, std::uint64_t id) { return book.id_to_order_.at(id); }

  static Order* Head(OrderBook& book, std::uint64_t price) { return book.levels_[price].Head(); }

  static Order* Tail(OrderBook& book, std::uint64_t price) { return book.levels_[price].Tail(); }
};

TEST_CASE("OrderBook reduces sizes on cancel and execute") {
  auto book = std::make_unique<OrderBook>();  // too big for the stack

  auto submit = [&](std::uint64_t id, std::uint64_t size) {
    book->HandleEvent(Event{
        .event_type = EventType::kAdd,
        .direction = Direction::kBuy,
        .id = id,
        .size = size,
        .price = 100,
    });
  };
  submit(1, 100);
  submit(2, 10);  // Same level, so removal has to relink more than one order.
  CHECK(OrderBookTest::Head(*book, 100) == OrderBookTest::At(*book, 1));
  CHECK(OrderBookTest::Tail(*book, 100) == OrderBookTest::At(*book, 2));

  book->HandleEvent(Event{
      .event_type = EventType::KExecute,
      .direction = Direction::kBuy,
      .id = 1,
      .size = 40,
      .price = 100,
  });
  CHECK(OrderBookTest::Size(*book, 1) == 60);

  book->HandleEvent(Event{
      .event_type = EventType::kCancel,
      .direction = Direction::kBuy,
      .id = 1,
      .size = 20,
      .price = 100,
  });
  CHECK(OrderBookTest::Size(*book, 1) == 40);

  book->HandleEvent(Event{
      .event_type = EventType::kTrade,
      .direction = Direction::kBuy,
      .id = 1,
      .size = 40,
      .price = 100,
  });
  CHECK(OrderBookTest::Size(*book, 1) == 40);

  book->HandleEvent(Event{
      .event_type = EventType::KExecute,
      .direction = Direction::kBuy,
      .id = 1,
      .size = 40,
      .price = 100,
  });
  CHECK_FALSE(OrderBookTest::Contains(*book, 1));
  CHECK(OrderBookTest::Head(*book, 100) == OrderBookTest::At(*book, 2));
  CHECK(OrderBookTest::Tail(*book, 100) == OrderBookTest::At(*book, 2));

  book->HandleEvent(Event{
      .event_type = EventType::kDelete,
      .direction = Direction::kBuy,
      .id = 2,
      .size = 10,
      .price = 100,
  });
  CHECK_FALSE(OrderBookTest::Contains(*book, 2));
  CHECK(OrderBookTest::Head(*book, 100) == OrderBookTest::Tail(*book, 100));
}

TEST_CASE("OrderBook ignores events for unknown order ids") {
  auto book = std::make_unique<OrderBook>();  // too big for the stack

  for (auto type : {EventType::kCancel, EventType::KExecute, EventType::kDelete}) {
    book->HandleEvent(Event{
        .event_type = type,
        .direction = Direction::kBuy,
        .id = 1,
        .size = 50,
        .price = 100,
    });
  }
  CHECK(OrderBookTest::Empty(*book));

  book->HandleEvent(Event{
      .event_type = EventType::kAdd,
      .direction = Direction::kBuy,
      .id = 1,
      .size = 100,
      .price = 100,
  });
  CHECK(OrderBookTest::Contains(*book, 1));
}
