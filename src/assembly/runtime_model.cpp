#include "duomec/assembly/runtime_model.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace duomec::assembly {
namespace {
core::Error invalid(std::string message, std::string context) {
  return {core::ErrorCode::invalid_argument, std::move(message),
          std::move(context)};
}
Transform multiply(const Transform &a, const Transform &b) {
  Transform result;
  result.matrix.fill(0.0);
  for (std::size_t column = 0; column < 4; ++column)
    for (std::size_t row = 0; row < 4; ++row)
      for (std::size_t k = 0; k < 4; ++k)
        result.matrix[column * 4 + row] +=
            a.matrix[k * 4 + row] * b.matrix[column * 4 + k];
  return result;
}
std::optional<Transform> inverse(const Transform &value) {
  std::array<std::array<double, 8>, 4> rows{};
  for (std::size_t row = 0; row < 4; ++row) {
    for (std::size_t column = 0; column < 4; ++column)
      rows[row][column] = value.matrix[column * 4 + row];
    rows[row][row + 4] = 1.0;
  }
  for (std::size_t column = 0; column < 4; ++column) {
    auto pivot = column;
    for (std::size_t row = column + 1; row < 4; ++row)
      if (std::abs(rows[row][column]) > std::abs(rows[pivot][column]))
        pivot = row;
    if (std::abs(rows[pivot][column]) < 1e-14)
      return std::nullopt;
    std::swap(rows[pivot], rows[column]);
    const auto divisor = rows[column][column];
    for (auto &entry : rows[column])
      entry /= divisor;
    for (std::size_t row = 0; row < 4; ++row) {
      if (row == column)
        continue;
      const auto factor = rows[row][column];
      for (std::size_t index = 0; index < 8; ++index)
        rows[row][index] -= factor * rows[column][index];
    }
  }
  Transform result;
  for (std::size_t row = 0; row < 4; ++row)
    for (std::size_t column = 0; column < 4; ++column)
      result.matrix[column * 4 + row] = rows[row][column + 4];
  return result;
}
} // namespace

core::Result<bool>
DefinitionRegistry::addPartDefinition(PartDefinition definition) {
  if (parts_.contains(definition.id))
    return core::Result<bool>::failure(
        invalid("duplicate part definition", definition.id.value()));
  const auto id = definition.id;
  parts_.emplace(id,
                 std::make_shared<const PartDefinition>(std::move(definition)));
  return core::Result<bool>::success(true);
}
core::Result<bool>
DefinitionRegistry::addAssemblyDefinition(AssemblyDefinition definition) {
  if (assemblies_.contains(definition.id))
    return core::Result<bool>::failure(
        invalid("duplicate assembly definition", definition.id.value()));
  const auto id = definition.id;
  assemblies_.emplace(
      id, std::make_shared<const AssemblyDefinition>(std::move(definition)));
  return core::Result<bool>::success(true);
}
core::Result<std::shared_ptr<const PartRevision>>
DefinitionRegistry::commitPartRevision(PartRevision revision) {
  if (!parts_.contains(revision.definitionId))
    return core::Result<std::shared_ptr<const PartRevision>>::failure(
        invalid("part definition missing", revision.definitionId.value()));
  if (partRevisions_.contains(revision.id))
    return core::Result<std::shared_ptr<const PartRevision>>::failure(
        invalid("revision already committed", revision.id.value()));
  auto committed = std::make_shared<const PartRevision>(std::move(revision));
  partRevisions_.emplace(committed->id, committed);
  return core::Result<std::shared_ptr<const PartRevision>>::success(committed);
}
core::Result<std::shared_ptr<const AssemblyRevision>>
DefinitionRegistry::commitAssemblyRevision(AssemblyRevision revision) {
  if (!assemblies_.contains(revision.definitionId))
    return core::Result<std::shared_ptr<const AssemblyRevision>>::failure(
        invalid("assembly definition missing", revision.definitionId.value()));
  if (assemblyRevisions_.contains(revision.id))
    return core::Result<std::shared_ptr<const AssemblyRevision>>::failure(
        invalid("revision already committed", revision.id.value()));
  auto committed =
      std::make_shared<const AssemblyRevision>(std::move(revision));
  assemblyRevisions_.emplace(committed->id, committed);
  return core::Result<std::shared_ptr<const AssemblyRevision>>::success(
      committed);
}
std::shared_ptr<const PartDefinition>
DefinitionRegistry::partDefinition(const cad::PartDefinitionId &id) const {
  const auto found = parts_.find(id);
  return found == parts_.end() ? nullptr : found->second;
}
std::shared_ptr<const AssemblyDefinition>
DefinitionRegistry::assemblyDefinition(
    const cad::AssemblyDefinitionId &id) const {
  const auto found = assemblies_.find(id);
  return found == assemblies_.end() ? nullptr : found->second;
}
std::shared_ptr<const PartRevision>
DefinitionRegistry::partRevision(const cad::PartRevisionId &id) const {
  const auto found = partRevisions_.find(id);
  return found == partRevisions_.end() ? nullptr : found->second;
}
std::shared_ptr<const AssemblyRevision>
DefinitionRegistry::assemblyRevision(const cad::AssemblyRevisionId &id) const {
  const auto found = assemblyRevisions_.find(id);
  return found == assemblyRevisions_.end() ? nullptr : found->second;
}
core::Result<PartDefinitionRef>
DefinitionRegistry::reference(const cad::PartDefinitionId &definition,
                              const cad::PartRevisionId &revision) const {
  const auto value = partRevision(revision);
  if (!partDefinition(definition) || !value ||
      value->definitionId != definition)
    return core::Result<PartDefinitionRef>::failure(
        invalid("invalid definition/revision pair", revision.value()));
  return core::Result<PartDefinitionRef>::success(
      {definition, revision, value->authoringHash, value->geometryHash,
       value->displayRelevantHash});
}
core::Result<AssemblyDefinitionRef>
DefinitionRegistry::reference(const cad::AssemblyDefinitionId &definition,
                              const cad::AssemblyRevisionId &revision) const {
  const auto value = assemblyRevision(revision);
  if (!assemblyDefinition(definition) || !value ||
      value->definitionId != definition)
    return core::Result<AssemblyDefinitionRef>::failure(
        invalid("invalid definition/revision pair", revision.value()));
  return core::Result<AssemblyDefinitionRef>::success(
      {definition, revision, value->authoringHash});
}

bool AssemblyRuntimeGraph::validReference(
    const CommittedDefinitionRef &reference) const {
  return std::visit(
      [this](const auto &value) {
        const auto resolved =
            registry_->reference(value.definitionId, value.revisionId);
        return resolved && resolved.value() == value;
      },
      reference);
}
core::Result<cad::OccurrenceId>
AssemblyRuntimeGraph::addOccurrence(AssemblyOccurrence occurrence) {
  if (occurrences_.contains(occurrence.id))
    return core::Result<cad::OccurrenceId>::failure(
        invalid("duplicate occurrence", occurrence.id.value()));
  if (!validReference(occurrence.reference))
    return core::Result<cad::OccurrenceId>::failure(
        invalid("unregistered committed reference", occurrence.id.value()));
  if (occurrence.parent && !occurrences_.contains(*occurrence.parent))
    return core::Result<cad::OccurrenceId>::failure(
        invalid("parent occurrence missing", occurrence.parent->value()));
  const auto id = occurrence.id;
  children_[occurrence.parent].insert(id);
  occurrences_.emplace(id, std::move(occurrence));
  return core::Result<cad::OccurrenceId>::success(id);
}
core::Result<bool>
AssemblyRuntimeGraph::removeOccurrence(const cad::OccurrenceId &id) {
  if (!occurrences_.contains(id))
    return core::Result<bool>::failure(
        invalid("occurrence missing", id.value()));
  if (!children_[id].empty())
    return core::Result<bool>::failure(
        invalid("cannot remove occurrence with children", id.value()));
  const auto parent = occurrences_.at(id).parent;
  children_[parent].erase(id);
  children_.erase(id);
  occurrences_.erase(id);
  return core::Result<bool>::success(true);
}
core::Result<InvalidationEvent>
AssemblyRuntimeGraph::updateOccurrenceTransform(const cad::OccurrenceId &id,
                                                Transform transform) {
  const auto found = occurrences_.find(id);
  if (found == occurrences_.end())
    return core::Result<InvalidationEvent>::failure(
        invalid("occurrence missing", id.value()));
  found->second.localTransform = std::move(transform);
  return core::Result<InvalidationEvent>::success(
      {{InvalidationDomain::Transform, InvalidationDomain::AssemblyBounds,
        InvalidationDomain::AssemblyRelation},
       id,
       std::nullopt});
}
core::Result<InvalidationEvent>
AssemblyRuntimeGraph::setVisibility(const cad::OccurrenceId &id, bool visible) {
  const auto found = occurrences_.find(id);
  if (found == occurrences_.end())
    return core::Result<InvalidationEvent>::failure(
        invalid("occurrence missing", id.value()));
  found->second.visible = visible;
  return core::Result<InvalidationEvent>::success(
      {{InvalidationDomain::Visibility, InvalidationDomain::Selection},
       id,
       std::nullopt});
}
core::Result<InvalidationEvent> AssemblyRuntimeGraph::setAppearanceOverride(
    const cad::OccurrenceId &id, std::string key, std::string value) {
  const auto found = occurrences_.find(id);
  if (found == occurrences_.end())
    return core::Result<InvalidationEvent>::failure(
        invalid("occurrence missing", id.value()));
  found->second.appearanceOverrides.insert_or_assign(std::move(key),
                                                     std::move(value));
  return core::Result<InvalidationEvent>::success(
      {{InvalidationDomain::Metadata}, id, std::nullopt});
}
core::Result<Transform>
AssemblyRuntimeGraph::worldTransform(const cad::OccurrenceId &id) const {
  const auto *current = find(id);
  if (!current)
    return core::Result<Transform>::failure(
        invalid("occurrence missing", id.value()));
  Transform result = current->localTransform;
  std::set<cad::OccurrenceId> visited{id};
  while (current->parent) {
    if (!visited.insert(*current->parent).second)
      return core::Result<Transform>::failure(
          invalid("cycle in occurrence hierarchy", id.value()));
    current = find(*current->parent);
    if (!current)
      return core::Result<Transform>::failure(
          invalid("broken occurrence hierarchy", id.value()));
    result = multiply(current->localTransform, result);
  }
  return core::Result<Transform>::success(result);
}
core::Result<bool>
AssemblyRuntimeGraph::reparent(const cad::OccurrenceId &id,
                               std::optional<cad::OccurrenceId> parent,
                               bool preserveWorldTransform) {
  auto found = occurrences_.find(id);
  if (found == occurrences_.end())
    return core::Result<bool>::failure(
        invalid("occurrence missing", id.value()));
  if (parent && !occurrences_.contains(*parent))
    return core::Result<bool>::failure(
        invalid("parent missing", parent->value()));
  for (auto cursor = parent; cursor;) {
    if (*cursor == id)
      return core::Result<bool>::failure(
          invalid("hierarchy cycle", id.value()));
    cursor = occurrences_.at(*cursor).parent;
  }
  Transform local = found->second.localTransform;
  if (preserveWorldTransform) {
    const auto oldWorld = worldTransform(id);
    if (!oldWorld)
      return core::Result<bool>::failure(oldWorld.error());
    if (parent) {
      const auto parentWorld = worldTransform(*parent);
      if (!parentWorld)
        return core::Result<bool>::failure(parentWorld.error());
      const auto parentInverse = inverse(parentWorld.value());
      if (!parentInverse)
        return core::Result<bool>::failure(
            invalid("parent transform is singular", parent->value()));
      local = multiply(*parentInverse, oldWorld.value());
    } else {
      local = oldWorld.value();
    }
  }
  children_[found->second.parent].erase(id);
  found->second.parent = parent;
  found->second.localTransform = local;
  children_[parent].insert(id);
  return core::Result<bool>::success(true);
}
core::Result<InvalidationEvent>
AssemblyRuntimeGraph::replaceReference(const cad::OccurrenceId &id,
                                       CommittedDefinitionRef reference) {
  auto found = occurrences_.find(id);
  if (found == occurrences_.end() || !validReference(reference))
    return core::Result<InvalidationEvent>::failure(
        invalid("occurrence or committed reference missing", id.value()));
  found->second.reference = std::move(reference);
  return core::Result<InvalidationEvent>::success(
      {{InvalidationDomain::ExactGeometry, InvalidationDomain::Tessellation,
        InvalidationDomain::Selection, InvalidationDomain::MassProperties,
        InvalidationDomain::AssemblyRelation,
        InvalidationDomain::AssemblyBounds},
       id,
       std::nullopt});
}
const AssemblyOccurrence *
AssemblyRuntimeGraph::find(const cad::OccurrenceId &id) const {
  const auto found = occurrences_.find(id);
  return found == occurrences_.end() ? nullptr : &found->second;
}
std::optional<cad::OccurrenceId>
AssemblyRuntimeGraph::parentOf(const cad::OccurrenceId &id) const {
  const auto *value = find(id);
  return value ? value->parent : std::nullopt;
}
std::vector<cad::OccurrenceId> AssemblyRuntimeGraph::childrenOf(
    const std::optional<cad::OccurrenceId> &parent) const {
  const auto found = children_.find(parent);
  return found == children_.end()
             ? std::vector<cad::OccurrenceId>{}
             : std::vector<cad::OccurrenceId>(found->second.begin(),
                                              found->second.end());
}
core::Result<std::vector<cad::OccurrenceId>>
AssemblyRuntimeGraph::pathTo(const cad::OccurrenceId &id) const {
  std::vector<cad::OccurrenceId> path;
  const auto *current = find(id);
  if (!current)
    return core::Result<std::vector<cad::OccurrenceId>>::failure(
        invalid("occurrence missing", id.value()));
  path.push_back(id);
  while (current->parent) {
    path.push_back(*current->parent);
    current = find(*current->parent);
    if (!current)
      return core::Result<std::vector<cad::OccurrenceId>>::failure(
          invalid("broken occurrence hierarchy", id.value()));
  }
  std::reverse(path.begin(), path.end());
  return core::Result<std::vector<cad::OccurrenceId>>::success(std::move(path));
}

} // namespace duomec::assembly
