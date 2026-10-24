#pragma once

#include <string_view>

class FeedHandler {
 public:
  FeedHandler(std::string_view path);

  FeedHandler(const FeedHandler&) = delete;
  FeedHandler(FeedHandler&&) = delete;
  FeedHandler operator=(const FeedHandler&) = delete;
  FeedHandler operator=(FeedHandler&&) = delete;

  void Run() {}

 private:
  std::string_view path_;
};
