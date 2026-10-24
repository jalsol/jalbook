#include <CLI/CLI.hpp>
#include <thread>

#include "feed_handler.hpp"
#include "order_book.hpp"
#include "spsc_queue.hpp"
#include "utils.hpp"

int main(int argc, char** argv) {
  CLI::App app{"just a little orderbook"};
  argv = app.ensure_utf8(argv);

  std::string input_path;
  app.add_option("-i", input_path, "Input path")->required()->check(CLI::ExistingFile);

  CLI11_PARSE(app, argc, argv);

  // TODO:
  // - bind book processing to queue

  SPSCQueue<Event, 1LL << 16> queue;
  FeedHandler feed_handler{&queue, input_path};

  std::thread feed_thread([&] { feed_handler.Run(); });
  threads::PinToCpu(&feed_thread, 1);

  std::thread book_thread([&] {
    auto order_book = std::make_unique<OrderBook>();
    Event event;

    do {
      while (!queue.Pop(&event));
      order_book->HandleEvent(event);
    } while (event.event_type != EventType::kHalt);
  });
  threads::PinToCpu(&book_thread, 2);

  feed_thread.join();
  book_thread.join();
}
