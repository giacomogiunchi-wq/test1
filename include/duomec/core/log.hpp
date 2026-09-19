#pragma once
#include <functional>
#include <string_view>
namespace duomec::core {
enum class LogLevel { debug, info, warning, error };
using LogSink = std::function<void(LogLevel, std::string_view)>;
void set_log_sink(LogSink sink);
void log(LogLevel level, std::string_view message);
}
