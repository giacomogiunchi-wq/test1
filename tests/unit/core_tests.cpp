#include "duomec/core/command.hpp"
#include "duomec/core/error.hpp"
#include "duomec/core/log.hpp"
#include "duomec/core/units.hpp"

#include <cmath>
#include <iostream>

using namespace duomec::core;
namespace {
class Add final : public ICommand {
public:
  Add(int &value, int by) : value_(value), by_(by) {}
  std::string_view name() const noexcept override { return "add"; }
  Result<bool> execute() override {
    value_ += by_;
    return Result<bool>::success(true);
  }
  Result<bool> undo() override {
    value_ -= by_;
    return Result<bool>::success(true);
  }

private:
  int &value_;
  int by_;
};
bool check(bool condition, const char *message) {
  if (!condition)
    std::cerr << "FAIL: " << message << '\n';
  return condition;
}
} // namespace
int main() {
  bool ok = true;
  auto failed =
      Result<int>::failure({ErrorCode::geometry_failure, "bad shape", "box"});
  ok &= check(!failed && failed.error().context == "box", "structured error");
  ok &= check(std::abs(to_si(25.4, LengthUnit::millimetre) - .0254) < 1e-12,
              "SI conversion");
  int value = 0;
  CommandStack stack;
  ok &= check(stack.execute(std::make_unique<Add>(value, 3)).has_value() &&
                  value == 3,
              "execute");
  ok &=
      check(stack.undo().has_value() && value == 0 && stack.can_redo(), "undo");
  ok &= check(stack.redo().has_value() && value == 3, "redo");

  // A sink may safely replace itself; invocation must happen outside the lock.
  int logCalls = 0;
  set_log_sink([&](LogLevel, std::string_view) {
    ++logCalls;
    set_log_sink({});
  });
  log(LogLevel::info, "reentrant sink");
  log(LogLevel::info, "disabled sink");
  ok &= check(logCalls == 1, "logging does not deadlock on reentrant sink");
  return ok ? 0 : 1;
}
