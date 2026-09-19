#include "duomec/core/command.hpp"
namespace duomec::core {
Result<bool> CommandStack::execute(std::unique_ptr<ICommand> command) {
  if (!command) return Result<bool>::failure({ErrorCode::invalid_argument, "null command", {}});
  auto result = command->execute(); if (!result) return result;
  done_.push_back(std::move(command)); undone_.clear(); return Result<bool>::success(true);
}
Result<bool> CommandStack::undo() {
  if (done_.empty()) return Result<bool>::failure({ErrorCode::invalid_argument, "nothing to undo", {}});
  auto command = std::move(done_.back()); done_.pop_back(); auto result = command->undo();
  if (!result) { done_.push_back(std::move(command)); return result; }
  undone_.push_back(std::move(command)); return Result<bool>::success(true);
}
Result<bool> CommandStack::redo() {
  if (undone_.empty()) return Result<bool>::failure({ErrorCode::invalid_argument, "nothing to redo", {}});
  auto command = std::move(undone_.back()); undone_.pop_back(); auto result = command->execute();
  if (!result) { undone_.push_back(std::move(command)); return result; }
  done_.push_back(std::move(command)); return Result<bool>::success(true);
}
bool CommandStack::can_undo() const noexcept { return !done_.empty(); }
bool CommandStack::can_redo() const noexcept { return !undone_.empty(); }
}
