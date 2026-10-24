#include <CLI/CLI.hpp>
#include <thread>

#include "feed_handler.hpp"
#include "utils.hpp"

int main(int argc, char** argv) {
  CLI::App app{"just a little orderbook"};
  argv = app.ensure_utf8(argv);

  std::string input_path;
  app.add_option("-i", input_path, "Input path")->required()->check(CLI::ExistingFile);

  CLI11_PARSE(app, argc, argv);

  // TODO:
  // - init a spsc queue
  // - bind feed to input_path and queue
  // - bind book processing to queue

  FeedHandler feed_handler{input_path};

  std::thread feed_thread([&] { feed_handler.Run(); });
  threads::PinToCpu(&feed_thread, 1);

  std::thread book_thread([&] {
    // TODO: continuously get data from queue
  });
  threads::PinToCpu(&book_thread, 2);

  feed_thread.join();
  book_thread.join();
}
