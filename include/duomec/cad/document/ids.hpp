#pragma once

#include "duomec/core/error.hpp"

#include <compare>
#include <string>
#include <string_view>

namespace duomec::cad {

template <class Tag> class PersistentId {
public:
  [[nodiscard]] static PersistentId generate();
  [[nodiscard]] static core::Result<PersistentId> parse(std::string_view text);
  [[nodiscard]] const std::string &value() const noexcept { return value_; }
  auto operator<=>(const PersistentId &) const = default;

private:
  explicit PersistentId(std::string value) : value_(std::move(value)) {}
  std::string value_;
};

struct DocumentIdTag;
struct BodyIdTag;
struct FeatureIdTag;
struct SketchIdTag;
struct SketchEntityIdTag;
struct ConstraintIdTag;
struct ParameterIdTag;
struct TopologyReferenceIdTag;

using DocumentId = PersistentId<DocumentIdTag>;
using BodyId = PersistentId<BodyIdTag>;
using FeatureId = PersistentId<FeatureIdTag>;
using SketchId = PersistentId<SketchIdTag>;
using SketchEntityId = PersistentId<SketchEntityIdTag>;
using ConstraintId = PersistentId<ConstraintIdTag>;
using ParameterId = PersistentId<ParameterIdTag>;
using TopologyReferenceId = PersistentId<TopologyReferenceIdTag>;

extern template class PersistentId<DocumentIdTag>;
extern template class PersistentId<BodyIdTag>;
extern template class PersistentId<FeatureIdTag>;
extern template class PersistentId<SketchIdTag>;
extern template class PersistentId<SketchEntityIdTag>;
extern template class PersistentId<ConstraintIdTag>;
extern template class PersistentId<ParameterIdTag>;
extern template class PersistentId<TopologyReferenceIdTag>;

} // namespace duomec::cad
