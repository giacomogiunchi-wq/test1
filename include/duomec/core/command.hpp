#pragma once
#include "duomec/core/error.hpp"
#include <memory>
#include <string_view>
#include <vector>
namespace duomec::core {
class ICommand {
 public:
  virtual ~ICommand() = default;
  [[nodiscard]] virtual std::string_view name() const noexcept = 0;
  virtual Result<bool> execute() = 0;
  virtual Result<bool> undo() = 0;
};
class CommandStack {
 public:
  Result<bool> execute(std::unique_ptr<ICommand> command);
  Result<bool> undo();
  Result<bool> redo();
  [[nodiscard]] bool can_undo() const noexcept;
  [[nodiscard]] bool can_redo() const noexcept;
 private:
  std::vector<std::unique_ptr<ICommand>> done_;
  std::vector<std::unique_ptr<ICommand>> undone_;
};
}
