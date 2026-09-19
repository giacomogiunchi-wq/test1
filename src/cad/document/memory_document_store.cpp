#include "duomec/cad/document/memory_document_store.hpp"

namespace duomec::cad {
namespace {
core::Error state_error(std::string message) {
  return {core::ErrorCode::internal, std::move(message), "MemoryDocumentStore"};
}
} // namespace

MemoryDocumentStore::MemoryDocumentStore(
    std::shared_ptr<MemoryDocumentRepository> repository)
    : repository_(std::move(repository)) {}
core::Result<bool>
MemoryDocumentStore::initialize(const DocumentSnapshot &snapshot) {
  history_ = {snapshot};
  cursor_ = 0;
  transaction_open_ = false;
  return core::Result<bool>::success(true);
}
core::Result<bool> MemoryDocumentStore::begin_transaction() {
  if (transaction_open_ || history_.empty())
    return core::Result<bool>::failure(state_error("cannot begin transaction"));
  pending_ = history_[cursor_];
  transaction_open_ = true;
  return core::Result<bool>::success(true);
}
core::Result<bool>
MemoryDocumentStore::write(const DocumentSnapshot &snapshot) {
  if (!transaction_open_)
    return core::Result<bool>::failure(state_error("no open transaction"));
  pending_ = snapshot;
  return core::Result<bool>::success(true);
}
core::Result<bool> MemoryDocumentStore::commit_transaction() {
  if (!transaction_open_)
    return core::Result<bool>::failure(state_error("no open transaction"));
  history_.erase(history_.begin() + static_cast<std::ptrdiff_t>(cursor_ + 1),
                 history_.end());
  history_.push_back(std::move(pending_));
  cursor_ = history_.size() - 1;
  transaction_open_ = false;
  return core::Result<bool>::success(true);
}
void MemoryDocumentStore::abort_transaction() noexcept {
  transaction_open_ = false;
}
core::Result<bool> MemoryDocumentStore::can_undo() const {
  return core::Result<bool>::success(!history_.empty() && cursor_ > 0);
}
core::Result<bool> MemoryDocumentStore::can_redo() const {
  return core::Result<bool>::success(!history_.empty() &&
                                     cursor_ + 1 < history_.size());
}
core::Result<DocumentSnapshot> MemoryDocumentStore::undo() {
  if (transaction_open_ || history_.empty() || cursor_ == 0)
    return core::Result<DocumentSnapshot>::failure(
        state_error("nothing to undo"));
  return core::Result<DocumentSnapshot>::success(history_[--cursor_]);
}
core::Result<DocumentSnapshot> MemoryDocumentStore::redo() {
  if (transaction_open_ || history_.empty() || cursor_ + 1 >= history_.size())
    return core::Result<DocumentSnapshot>::failure(
        state_error("nothing to redo"));
  return core::Result<DocumentSnapshot>::success(history_[++cursor_]);
}
core::Result<bool>
MemoryDocumentStore::save_atomic(const std::filesystem::path &path) {
  if (history_.empty())
    return core::Result<bool>::failure(state_error("store is not initialized"));
  repository_->files.insert_or_assign(path.lexically_normal().string(),
                                      history_[cursor_]);
  return core::Result<bool>::success(true);
}
core::Result<DocumentSnapshot>
MemoryDocumentStore::load(const std::filesystem::path &path) {
  const auto item = repository_->files.find(path.lexically_normal().string());
  if (item == repository_->files.end())
    return core::Result<DocumentSnapshot>::failure(
        {core::ErrorCode::io_failure, "document not found", path.string()});
  auto initialized = initialize(item->second);
  if (!initialized)
    return core::Result<DocumentSnapshot>::failure(initialized.error());
  return core::Result<DocumentSnapshot>::success(item->second);
}

} // namespace duomec::cad
