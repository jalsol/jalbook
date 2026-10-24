#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/log/globals.h>
#include <absl/log/initialize.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "feed_handler.hpp"
#include "order_book.hpp"
#include "spsc_queue.hpp"
#include "utils.hpp"

// Abseil's --help only lists flags defined in <binary-name>.cpp (jalbook.cpp).
ABSL_FLAG(std::string, input_path, "", "Input path");

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("just a little orderbook");
  absl::ParseCommandLine(argc, argv);

  const std::string input_path = absl::GetFlag(FLAGS_input_path);
  if (!std::filesystem::is_regular_file(input_path)) {
    std::cerr << "error: --input_path must be an existing file\n";
    return 1;
  }

  absl::InitializeLog();
  // The stderr sink defaults to ERROR; keep logging every event as before.
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

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
