#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <hdr/hdr_histogram.h>

#include <array>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "feed_handler.hpp"
#include "order_book.hpp"
#include "spsc_queue.hpp"
#include "utils.hpp"

ABSL_FLAG(std::string, input_path, "", "Input path");

namespace {

constexpr std::int64_t kMaxCycles = 1LL << 40;
constexpr auto kOpNames = std::array{"add", "cancel", "delete", "execute"};
static_assert(static_cast<std::size_t>(EventType::KExecute) == kOpNames.size());

void PrintHistogram(const char* name, const hdr_histogram* histogram, double ns_per_cycle) {
  const auto ns = [ns_per_cycle](std::int64_t cycles) {
    return static_cast<double>(cycles) * ns_per_cycle;
  };

  std::printf(
      "%-8s %10" PRId64 " %10.1f %10.1f %10.1f %10.1f %10.1f %10.1f %10.1f\n", name,
      histogram->total_count, hdr_mean(histogram) * ns_per_cycle,
      ns(hdr_value_at_percentile(histogram, 50.0)), ns(hdr_value_at_percentile(histogram, 90.0)),
      ns(hdr_value_at_percentile(histogram, 99.0)), ns(hdr_value_at_percentile(histogram, 99.9)),
      ns(hdr_value_at_percentile(histogram, 99.99)), ns(hdr_max(histogram)));
}

}  // namespace

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("just a little orderbook");
  absl::ParseCommandLine(argc, argv);

  const std::string input_path = absl::GetFlag(FLAGS_input_path);
  if (!std::filesystem::is_regular_file(input_path)) {
    std::cerr << "error: --input_path must be an existing file\n";
    return 1;
  }

  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

  const double tsc_hz = cycles::Hz();
  const double ns_per_cycle = 1e9 / tsc_hz;
  const std::uint64_t overhead = cycles::Overhead();

  std::array<hdr_histogram*, kOpNames.size()> histograms{};
  for (hdr_histogram*& histogram : histograms) {
    if (hdr_init(1, kMaxCycles, 3, &histogram) != 0) {
      std::cerr << "error: hdr_init failed\n";
      for (hdr_histogram* initialized : histograms) {
        if (initialized != nullptr) {
          hdr_close(initialized);
        }
      }
      return 1;
    }
  }

  std::uint64_t dropped = 0;

  SPSCQueue<Event, 1LL << 16> queue;
  FeedHandler feed_handler{&queue, input_path};

  std::thread feed_thread([&] { feed_handler.Run(); });
  threads::PinToCpu(&feed_thread, 1);

  std::thread book_thread([&] {
    auto order_book = std::make_unique<OrderBook>();
    Event event;

    do {
      while (!queue.Pop(&event));

      if (event.IsBookRelevant()) {
        const std::size_t op = static_cast<std::size_t>(event.event_type) - 1;
        const std::uint64_t elapsed = cycles::Measure([&] { order_book->HandleEvent(event); });
        const std::uint64_t adjusted = elapsed > overhead ? elapsed - overhead : 0;
        if (!hdr_record_value(histograms[op], static_cast<std::int64_t>(adjusted))) {
          ++dropped;
        }
      } else {
        order_book->HandleEvent(event);
      }
    } while (event.event_type != EventType::kHalt);
  });
  threads::PinToCpu(&book_thread, 2);

  feed_thread.join();
  book_thread.join();

  std::printf("TSC: %.3f GHz (%.3f ns/cycle); %" PRIu64
              "-cycle (%.1f ns) measurement overhead subtracted\n",
              tsc_hz / 1e9, ns_per_cycle, overhead, static_cast<double>(overhead) * ns_per_cycle);
  std::puts("nanoseconds per OrderBook::HandleEvent (rdtscp; overhead subtracted)");
  std::printf("%-8s %10s %10s %10s %10s %10s %10s %10s %10s\n", "op", "count", "mean", "p50", "p90",
              "p99", "p99.9", "p99.99", "max");
  for (std::size_t i = 0; i < kOpNames.size(); ++i) {
    PrintHistogram(kOpNames[i], histograms[i], ns_per_cycle);
    hdr_close(histograms[i]);
  }

  if (dropped != 0) {
    std::fprintf(stderr, "warning: %" PRIu64 " samples above %" PRId64 " cycles were dropped\n",
                 dropped, kMaxCycles);
  }
}
