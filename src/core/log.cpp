#include "duomec/core/log.hpp"
#include <iostream>
#include <mutex>
namespace duomec::core { namespace {
std::mutex mutex;
LogSink sink = [](LogLevel level, std::string_view message) {
  constexpr const char* names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
  std::clog << '[' << names[static_cast<unsigned>(level)] << "] " << message << '\n';
}; }
void set_log_sink(LogSink replacement) { std::scoped_lock lock(mutex); sink = std::move(replacement); }
void log(LogLevel level, std::string_view message) { std::scoped_lock lock(mutex); sink(level, message); }
}
