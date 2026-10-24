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

  Event event;
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
