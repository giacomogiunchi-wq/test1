#pragma once
#include <string>
#include <utility>

namespace duomec::core {
enum class ErrorCode { invalid_argument, geometry_failure, io_failure, dependency_missing,
                       process_failure, timeout, cancelled, unsupported, internal };
struct Error {
  ErrorCode code{ErrorCode::internal};
  std::string message;
  std::string context;
};

template<class T> class Result {
 public:
  static Result success(T value) { return Result(std::move(value)); }
  static Result failure(Error error) { return Result(std::move(error)); }
  [[nodiscard]] bool has_value() const noexcept { return has_value_; }
  [[nodiscard]] explicit operator bool() const noexcept { return has_value_; }
  [[nodiscard]] const T& value() const & { return value_; }
  [[nodiscard]] T&& value() && { return std::move(value_); }
  [[nodiscard]] const Error& error() const noexcept { return error_; }
 private:
  explicit Result(T value) : value_(std::move(value)), has_value_(true) {}
  explicit Result(Error error) : error_(std::move(error)), has_value_(false) {}
  T value_{};
  Error error_{};
  bool has_value_{false};
};
}
