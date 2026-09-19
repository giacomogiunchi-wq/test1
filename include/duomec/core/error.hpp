#pragma once

#include <string>
#include <utility>
#include <variant>

namespace duomec::core {

enum class ErrorCode {
  invalid_argument,
  geometry_failure,
  io_failure,
  dependency_missing,
  process_failure,
  timeout,
  cancelled,
  unsupported,
  internal
};

struct Error {
  ErrorCode code{ErrorCode::internal};
  std::string message;
  std::string context;
};

template <class T> class Result {
public:
  static Result success(T value) { return Result(std::move(value)); }
  static Result failure(Error error) { return Result(std::move(error)); }

  [[nodiscard]] bool has_value() const noexcept {
    return std::holds_alternative<T>(storage_);
  }
  [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
  [[nodiscard]] const T &value() const & { return std::get<T>(storage_); }
  [[nodiscard]] T &&value() && { return std::get<T>(std::move(storage_)); }
  [[nodiscard]] const Error &error() const { return std::get<Error>(storage_); }

private:
  explicit Result(T value)
      : storage_(std::in_place_type<T>, std::move(value)) {}
  explicit Result(Error error)
      : storage_(std::in_place_type<Error>, std::move(error)) {}

  std::variant<T, Error> storage_;
};

} // namespace duomec::core
