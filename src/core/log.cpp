#include "duomec/core/log.hpp"
#include <iostream>
#include <mutex>
namespace duomec::core {
namespace {
std::mutex mutex;
LogSink sink = [](LogLevel level, std::string_view message) {
  constexpr const char *names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
  std::clog << '[' << names[static_cast<unsigned>(level)] << "] " << message
            << '\n';
};
} // namespace
void set_log_sink(LogSink replacement) {
  std::scoped_lock lock(mutex);
  sink = replacement ? std::move(replacement) : LogSink{};
}
void log(LogLevel level, std::string_view message) {
  LogSink current;
  {
    std::scoped_lock lock(mutex);
    current = sink;
  }
  if (current)
    current(level, message);
}
} // namespace duomec::core
