#include "duomec/assembly/constraint_runtime.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <sstream>

namespace duomec::assembly {
namespace {
std::vector<cad::OccurrenceId> endpointOccurrences(const AssemblyRelation &r) {
  std::vector<cad::OccurrenceId> ids;
  for (const auto &e : r.endpoints)
    if (std::find(ids.begin(), ids.end(), e.occurrenceId) == ids.end())
      ids.push_back(e.occurrenceId);
  return ids;
}
std::size_t constrainedCount(const AssemblyRelation &relation) {
  switch (relation.type) {
  case RelationType::Fixed:
  case RelationType::Lock:
  case RelationType::Frame:
    return 6;
  case RelationType::Coincident:
    return 3;
  case RelationType::Concentric:
    if (const auto found = relation.parameters.find("lock_rotation");
        found != relation.parameters.end())
      return std::get<bool>(found->second) ? 5 : 4;
    return 4;
  case RelationType::Insert:
    if (const auto found = relation.parameters.find("lock_rotation");
        found != relation.parameters.end())
      return std::get<bool>(found->second) ? 6 : 5;
    return 5;
  case RelationType::LimitDistance:
  case RelationType::LimitAngle:
  case RelationType::LinearCoupler:
  case RelationType::Symmetric:
  case RelationType::Width:
  case RelationType::Cam:
  case RelationType::Gear:
  case RelationType::RackPinion:
  case RelationType::BeltChain:
    return 1;
  case RelationType::Path:
  case RelationType::ProfileCenter:
  case RelationType::Hinge:
  case RelationType::Screw:
  case RelationType::Slot:
    return 5;
  case RelationType::UniversalJoint:
    return 4;
  case RelationType::Parallel:
    return 2;
  case RelationType::Distance:
  case RelationType::Angle:
  case RelationType::Perpendicular:
  case RelationType::Tangent:
    return 1;
  case RelationType::Generic:
    return 0;
  }
  return 0;
}
std::string relationKey(const AssemblyRelation &relation,
                        bool includeParameters) {
  std::vector<std::string> ids;
  ids.reserve(relation.endpoints.size());
  for (const auto &endpoint : relation.endpoints)
    ids.push_back(endpoint.occurrenceId.value() +
                  endpoint.topologyReferenceId.value());
  std::sort(ids.begin(), ids.end());
  std::ostringstream out;
  out << static_cast<int>(relation.type);
  for (const auto &id : ids)
    out << '|' << id;
  if (includeParameters)
    for (const auto &[name, value] : relation.parameters) {
      out << '|' << name << ':' << value.index() << ':';
      std::visit([&](const auto &v) { out << v; }, value);
    }
  return out.str();
}
void constrain(DofState &state, std::size_t amount) {
  for (std::size_t i = 0; i < state.free.size() && amount > 0; ++i)
    if (state.free[i]) {
      state.free[i] = false;
      --amount;
    }
  state.state = state.independentDofCount() == 0
                    ? ConstraintState::FullyConstrained
                    : ConstraintState::UnderConstrained;
}
AssemblyRelation relationForCandidate(QuickMateCandidate candidate,
                                      RelationEndpoint first,
                                      RelationEndpoint second,
                                      Alignment alignment,
                                      std::optional<double> value) {
  if (candidate == QuickMateCandidate::Concentric ||
      candidate == QuickMateCandidate::ConcentricLockRotation)
    return makeConcentric(
        std::move(first), std::move(second),
        {alignment, candidate == QuickMateCandidate::ConcentricLockRotation});
  if (candidate == QuickMateCandidate::Angle) {
    const auto reference = first.localFrame.xAxis;
    return makeAngle(std::move(first), std::move(second),
                     {reference, alignment, value.value_or(0)});
  }
  AssemblyRelation relation;
  relation.endpoints = {std::move(first), std::move(second)};
  switch (candidate) {
  case QuickMateCandidate::Coincident:
    relation.type = RelationType::Coincident;
    break;
  case QuickMateCandidate::Distance:
    relation.type = RelationType::Distance;
    relation.parameters["distance_si"] = value.value_or(0);
    break;
  case QuickMateCandidate::Parallel:
    relation.type = RelationType::Parallel;
    break;
  case QuickMateCandidate::Angle:
    break;
  case QuickMateCandidate::Frame:
    relation.type = RelationType::Frame;
    break;
  case QuickMateCandidate::Tangent:
    relation.type = RelationType::Tangent;
    break;
  default:
    break;
  }
  return relation;
}
} // namespace

SolveResult NativeAssemblyConstraintSolver::solve(
    std::span<const OccurrenceState> occurrences,
    std::span<const AssemblyRelation> relations, const SolveOptions &options) {
  SolveResult result;
  result.inputRevision = options.inputRevision;
  std::set<std::string> completeKeys;
  std::map<std::string, std::string> structuralKeys;
  bool redundant = false;
  bool conflicting = false;
  for (const auto &occurrence : occurrences) {
    result.poses.emplace(occurrence.id, occurrence.localTransform);
    result.dofStates.emplace(occurrence.id,
                             occurrence.mobility == PlacementMobility::Fixed
                                 ? DofState::fixed()
                                 : DofState::floating());
  }
  for (const auto &relation : relations) {
    if (relation.state != RelationState::Active)
      continue;
    const auto structural = relationKey(relation, false);
    const auto complete = relationKey(relation, true);
    if (!completeKeys.insert(complete).second)
      redundant = true;
    const auto [entry, inserted] = structuralKeys.emplace(structural, complete);
    if (!inserted && entry->second != complete)
      conflicting = true;
    for (const auto &endpoint : relation.endpoints) {
      auto found = result.dofStates.find(endpoint.occurrenceId);
      if (found != result.dofStates.end() &&
          found->second.state != ConstraintState::Fixed)
        constrain(found->second, constrainedCount(relation));
    }
  }
  if (conflicting) {
    result.status = SolveStatus::Conflicting;
    for (auto &[id, state] : result.dofStates) {
      (void)id;
      state.state = ConstraintState::Conflicting;
    }
    result.diagnostics.push_back(
        "relations impose different parameters on the same semantic endpoints");
  } else if (redundant) {
    result.status = SolveStatus::Redundant;
    for (auto &[id, state] : result.dofStates) {
      (void)id;
      if (state.state != ConstraintState::Fixed)
        state.state = ConstraintState::Redundant;
    }
  } else {
    const bool anyFree = std::any_of(
        result.dofStates.begin(), result.dofStates.end(),
        [](const auto &v) { return v.second.independentDofCount() != 0; });
    result.status =
        anyFree ? SolveStatus::UnderConstrained : SolveStatus::FullyConstrained;
  }
  result.iterations = relations.empty() ? 0 : 1;
  return result;
}

core::Result<cad::SolveIslandId>
AssemblyRelationGraph::add(const AssemblyRelation &relation) {
  if (relation.endpoints.empty())
    return core::Result<cad::SolveIslandId>::failure(
        {core::ErrorCode::invalid_argument, "relation has no endpoints",
         "AssemblyRelationGraph::add"});
  if (!relations_.emplace(relation.id, relation).second)
    return core::Result<cad::SolveIslandId>::failure(
        {core::ErrorCode::invalid_argument, "relation already exists",
         relation.id.value()});
  const auto occurrences = endpointOccurrences(relation);
  std::vector<std::size_t> touched;
  for (const auto &occurrence : occurrences)
    if (const auto island = occurrenceIsland_.find(occurrence);
        island != occurrenceIsland_.end()) {
      const auto index = islandIndex_.at(island->second);
      if (std::find(touched.begin(), touched.end(), index) == touched.end())
        touched.push_back(index);
    }
  std::sort(touched.begin(), touched.end());
  if (touched.size() == 1) {
    auto &island = islands_[touched.front()];
    island.relations.insert(relation.id);
    for (const auto &id : occurrences) {
      island.occurrences.insert(id);
      occurrenceIsland_.insert_or_assign(id, island.id);
    }
    return core::Result<cad::SolveIslandId>::success(island.id);
  }
  SolveIsland merged;
  for (const auto &id : occurrences)
    merged.occurrences.insert(id);
  merged.relations.insert(relation.id);
  for (auto it = touched.rbegin(); it != touched.rend(); ++it) {
    merged.occurrences.insert(islands_[*it].occurrences.begin(),
                              islands_[*it].occurrences.end());
    merged.relations.insert(islands_[*it].relations.begin(),
                            islands_[*it].relations.end());
    islands_.erase(islands_.begin() + static_cast<std::ptrdiff_t>(*it));
  }
  islands_.push_back(std::move(merged));
  if (touched.empty()) {
    const auto index = islands_.size() - 1;
    islandIndex_[islands_.back().id] = index;
    for (const auto &occurrence : islands_.back().occurrences)
      occurrenceIsland_.insert_or_assign(occurrence, islands_.back().id);
  } else {
    rebuildIndexes();
  }
  return core::Result<cad::SolveIslandId>::success(islands_.back().id);
}
core::Result<bool>
AssemblyRelationGraph::remove(const cad::AssemblyRelationId &relation) {
  const auto found = relations_.find(relation);
  if (found == relations_.end())
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "relation not found",
                                        relation.value()});
  const auto endpoints = endpointOccurrences(found->second);
  const auto affectedId = occurrenceIsland_.at(endpoints.front());
  const auto affectedIndex = islandIndex_.at(affectedId);
  const auto affectedRelations = islands_[affectedIndex].relations;
  islands_.erase(islands_.begin() + static_cast<std::ptrdiff_t>(affectedIndex));
  relations_.erase(found);
  std::vector<AssemblyRelation> toRebuild;
  for (const auto &id : affectedRelations)
    if (id != relation) {
      toRebuild.push_back(relations_.at(id));
      relations_.erase(id);
    }
  rebuildIndexes();
  for (const auto &remaining : toRebuild)
    (void)add(remaining);
  return core::Result<bool>::success(true);
}
void AssemblyRelationGraph::rebuildIndexes() {
  occurrenceIsland_.clear();
  islandIndex_.clear();
  for (std::size_t index = 0; index < islands_.size(); ++index) {
    islandIndex_.emplace(islands_[index].id, index);
    for (const auto &occurrence : islands_[index].occurrences)
      occurrenceIsland_.insert_or_assign(occurrence, islands_[index].id);
  }
}
std::optional<SolveIsland>
AssemblyRelationGraph::islandFor(const cad::OccurrenceId &occurrence) const {
  const auto found = occurrenceIsland_.find(occurrence);
  if (found == occurrenceIsland_.end())
    return std::nullopt;
  return islands_[islandIndex_.at(found->second)];
}
const std::vector<SolveIsland> &
AssemblyRelationGraph::islands() const noexcept {
  return islands_;
}
std::vector<AssemblyRelation>
AssemblyRelationGraph::relationsFor(const SolveIsland &island) const {
  std::vector<AssemblyRelation> result;
  result.reserve(island.relations.size());
  for (const auto &id : island.relations)
    result.push_back(relations_.at(id));
  return result;
}

AssemblyRelation makeConcentric(RelationEndpoint first, RelationEndpoint second,
                                ConcentricOptions options) {
  AssemblyRelation relation;
  relation.type = RelationType::Concentric;
  relation.endpoints = {std::move(first), std::move(second)};
  relation.parameters["alignment"] =
      static_cast<std::int64_t>(options.alignment);
  relation.parameters["lock_rotation"] = options.lockRotation;
  return relation;
}
AssemblyRelation makeAngle(RelationEndpoint first, RelationEndpoint second,
                           AngleBranch branch) {
  AssemblyRelation relation;
  relation.type = RelationType::Angle;
  relation.endpoints = {std::move(first), std::move(second)};
  relation.parameters["angle_rad"] = branch.signedAngleRadians;
  relation.parameters["alignment"] =
      static_cast<std::int64_t>(branch.alignment);
  relation.parameters["reference_x"] = branch.referenceDirection.x;
  relation.parameters["reference_y"] = branch.referenceDirection.y;
  relation.parameters["reference_z"] = branch.referenceDirection.z;
  return relation;
}
AssemblyRelation makeInsert(RelationEndpoint axis, RelationEndpoint seat,
                            InsertOptions options) {
  AssemblyRelation relation;
  relation.type = RelationType::Insert;
  relation.endpoints = {std::move(axis), std::move(seat)};
  relation.parameters["alignment"] =
      static_cast<std::int64_t>(options.alignment);
  relation.parameters["lock_rotation"] = options.lockRotation;
  relation.motion.semantic =
      options.lockRotation ? MotionSemantic::Fixed : MotionSemantic::Revolute;
  return relation;
}
AssemblyRelation groundOccurrence(cad::OccurrenceId occurrence) {
  AssemblyRelation relation;
  relation.type = RelationType::Fixed;
  RelationEndpoint endpoint;
  endpoint.occurrenceId = std::move(occurrence);
  endpoint.geometry.kind = GeometryKind::CoordinateFrame;
  relation.endpoints.push_back(std::move(endpoint));
  return relation;
}

std::vector<QuickMateCandidate>
MateCandidateEngine::candidates(const GeometryDescriptor &first,
                                const GeometryDescriptor &second) const {
  const auto planar = [](GeometryKind k) { return k == GeometryKind::Plane; };
  const auto axial = [](GeometryKind k) {
    return k == GeometryKind::Axis || k == GeometryKind::Cylinder ||
           k == GeometryKind::Circle;
  };
  if (planar(first.kind) && planar(second.kind))
    return {QuickMateCandidate::Coincident, QuickMateCandidate::Distance,
            QuickMateCandidate::Parallel, QuickMateCandidate::Angle};
  if (axial(first.kind) && axial(second.kind))
    return {QuickMateCandidate::Concentric,
            QuickMateCandidate::ConcentricLockRotation};
  if ((planar(first.kind) && second.kind == GeometryKind::Point) ||
      (planar(second.kind) && first.kind == GeometryKind::Point))
    return {QuickMateCandidate::Coincident, QuickMateCandidate::Distance};
  if (first.kind == GeometryKind::CoordinateFrame &&
      second.kind == GeometryKind::CoordinateFrame)
    return {QuickMateCandidate::Frame};
  if ((planar(first.kind) && second.kind == GeometryKind::Cylinder) ||
      (planar(second.kind) && first.kind == GeometryKind::Cylinder))
    return {QuickMateCandidate::Tangent};
  return {};
}
QuickMateController::QuickMateController(MateCandidateEngine engine)
    : engine_(engine) {}
const QuickMateOverlay &
QuickMateController::begin(const RelationEndpoint &first,
                           const RelationEndpoint &second) {
  first_ = first;
  second_ = second;
  overlay_ = {};
  overlay_.candidates = engine_.candidates(first.geometry, second.geometry);
  overlay_.visible = !overlay_.candidates.empty();
  return overlay_;
}
void QuickMateController::flip() {
  overlay_.alignment = overlay_.alignment == Alignment::Aligned
                           ? Alignment::AntiAligned
                           : Alignment::Aligned;
}
void QuickMateController::setInlineValue(double value) {
  overlay_.inlineValue = value;
}
core::Result<bool>
QuickMateController::highlight(QuickMateCandidate candidate) {
  const auto found = std::find(overlay_.candidates.begin(),
                               overlay_.candidates.end(), candidate);
  if (found == overlay_.candidates.end())
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "candidate is not compatible",
                                        "QuickMateController"});
  overlay_.highlighted = static_cast<std::size_t>(
      std::distance(overlay_.candidates.begin(), found));
  return core::Result<bool>::success(true);
}
core::Result<AssemblyRelation> QuickMateController::accept() {
  if (!overlay_.visible || !first_ || !second_ ||
      overlay_.highlighted >= overlay_.candidates.size())
    return core::Result<AssemblyRelation>::failure(
        {core::ErrorCode::invalid_argument, "no Quick Mate candidate",
         "QuickMateController"});
  auto result =
      relationForCandidate(overlay_.candidates[overlay_.highlighted], *first_,
                           *second_, overlay_.alignment, overlay_.inlineValue);
  cancel();
  return core::Result<AssemblyRelation>::success(std::move(result));
}
void QuickMateController::cancel() {
  overlay_ = {};
  first_.reset();
  second_.reset();
}
const QuickMateOverlay &QuickMateController::overlay() const noexcept {
  return overlay_;
}

// Parameter order mirrors the public API: explicit pivot takes precedence.
void ComponentManipulator::begin(
    std::vector<OccurrenceState>
        selection, // NOLINT(bugprone-easily-swappable-parameters)
    std::optional<Vector3> explicitPivot,
    std::optional<Vector3> groupBoundsCenter) {
  original_ = std::move(selection);
  preview_ = original_;
  dragged_ = false;
  if (explicitPivot)
    pivot_ = *explicitPivot;
  else if (original_.size() == 1)
    pivot_ = {original_[0].localTransform.matrix[12],
              original_[0].localTransform.matrix[13],
              original_[0].localTransform.matrix[14]};
  else
    pivot_ = groupBoundsCenter.value_or(Vector3{});
}
ManipulationOutcome
ComponentManipulator::update(PointerButton button,
                             const ManipulationInput &input) {
  if (preview_.empty())
    return ManipulationOutcome::Reverted;
  if (!input.meaningfulDrag && button == PointerButton::Right)
    return ManipulationOutcome::ContextMenu;
  dragged_ = input.meaningfulDrag;
  bool moved = false;
  for (auto &occurrence : preview_) {
    if (occurrence.mobility == PlacementMobility::Fixed)
      continue;
    if (button == PointerButton::Left) {
      const double delta[3]{input.pointerDelta.x, input.pointerDelta.y,
                            input.pointerDelta.z};
      for (std::size_t i = 0; i < 3; ++i)
        if (occurrence.dofState.free[i]) {
          occurrence.localTransform.matrix[12 + i] += delta[i];
          moved = moved || delta[i] != 0;
        }
    } else if (occurrence.dofState.isFree(Dof::Rz)) {
      const double c = std::cos(input.rotationRadians),
                   s = std::sin(input.rotationRadians);
      const double x = occurrence.localTransform.matrix[12] - pivot_.x,
                   y = occurrence.localTransform.matrix[13] - pivot_.y;
      occurrence.localTransform.matrix[12] = pivot_.x + c * x - s * y;
      occurrence.localTransform.matrix[13] = pivot_.y + s * x + c * y;
      const double m0 = occurrence.localTransform.matrix[0];
      const double m1 = occurrence.localTransform.matrix[1];
      const double m4 = occurrence.localTransform.matrix[4];
      const double m5 = occurrence.localTransform.matrix[5];
      occurrence.localTransform.matrix[0] = c * m0 - s * m1;
      occurrence.localTransform.matrix[1] = s * m0 + c * m1;
      occurrence.localTransform.matrix[4] = c * m4 - s * m5;
      occurrence.localTransform.matrix[5] = s * m4 + c * m5;
      moved = moved || input.rotationRadians != 0;
    }
  }
  return moved ? ManipulationOutcome::Preview
               : ManipulationOutcome::FullyConstrained;
}
ManipulationOutcome ComponentManipulator::finish(bool solveConverged) {
  if (!dragged_)
    return ManipulationOutcome::ContextMenu;
  if (!solveConverged) {
    preview_ = original_;
    return ManipulationOutcome::Reverted;
  }
  original_ = preview_;
  return ManipulationOutcome::Committed;
}
const std::vector<OccurrenceState> &
ComponentManipulator::preview() const noexcept {
  return preview_;
}
Vector3 ComponentManipulator::pivot() const noexcept { return pivot_; }

AssemblyStateHistory::AssemblyStateHistory(AssemblyRelationsSnapshot initial)
    : current_(std::move(initial)) {}
void AssemblyStateHistory::commit(AssemblyRelationsSnapshot state) {
  undo_.push_back(current_);
  current_ = std::move(state);
  redo_.clear();
}
bool AssemblyStateHistory::canUndo() const noexcept { return !undo_.empty(); }
bool AssemblyStateHistory::canRedo() const noexcept { return !redo_.empty(); }
core::Result<bool> AssemblyStateHistory::undo() {
  if (!canUndo())
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "nothing to undo",
                                        "AssemblyStateHistory"});
  redo_.push_back(current_);
  current_ = std::move(undo_.back());
  undo_.pop_back();
  return core::Result<bool>::success(true);
}
core::Result<bool> AssemblyStateHistory::redo() {
  if (!canRedo())
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "nothing to redo",
                                        "AssemblyStateHistory"});
  undo_.push_back(current_);
  current_ = std::move(redo_.back());
  redo_.pop_back();
  return core::Result<bool>::success(true);
}
const AssemblyRelationsSnapshot &
AssemblyStateHistory::current() const noexcept {
  return current_;
}

ConstrainedDragController::ConstrainedDragController(
    IAssemblyConstraintSolver &solver, const AssemblyRelationGraph &graph)
    : solver_(&solver), graph_(&graph) {}
void ConstrainedDragController::begin(
    std::vector<OccurrenceState> selection,
    std::span<const OccurrenceState> assemblyOccurrences,
    std::uint64_t revision) {
  assemblyOccurrences_.assign(assemblyOccurrences.begin(),
                              assemblyOccurrences.end());
  inputRevision_ = currentRevision_ = revision;
  islandRelations_.clear();
  islandOccurrences_.clear();
  for (const auto &occurrence : selection)
    if (const auto island = graph_->islandFor(occurrence.id)) {
      islandOccurrences_.insert(island->occurrences.begin(),
                                island->occurrences.end());
      const auto relations = graph_->relationsFor(*island);
      islandRelations_.insert(islandRelations_.end(), relations.begin(),
                              relations.end());
    }
  for (const auto &occurrence : selection)
    islandOccurrences_.insert(occurrence.id);
  manipulator_.begin(std::move(selection));
}
SolveResult ConstrainedDragController::solve(SolveMode mode) {
  std::vector<OccurrenceState> local;
  for (auto occurrence : assemblyOccurrences_)
    if (islandOccurrences_.contains(occurrence.id)) {
      const auto preview = std::find_if(
          manipulator_.preview().begin(), manipulator_.preview().end(),
          [&](const auto &candidate) { return candidate.id == occurrence.id; });
      if (preview != manipulator_.preview().end())
        occurrence.localTransform = preview->localTransform;
      local.push_back(std::move(occurrence));
    }
  SolveOptions options;
  options.mode = mode;
  options.maximumIterations = mode == SolveMode::Interactive ? 12 : 100;
  options.timeBudget = mode == SolveMode::Interactive
                           ? std::chrono::milliseconds(8)
                           : std::chrono::milliseconds(100);
  options.inputRevision = inputRevision_;
  auto result = solver_->solve(local, islandRelations_, options);
  if (currentRevision_ != inputRevision_)
    result.status = SolveStatus::Stale;
  return result;
}
ManipulationOutcome
ConstrainedDragController::update(PointerButton button,
                                  const ManipulationInput &input) {
  const auto outcome = manipulator_.update(button, input);
  if (outcome != ManipulationOutcome::Preview)
    return outcome;
  lastSolve_ = solve(SolveMode::Interactive);
  if (lastSolve_->status == SolveStatus::Conflicting ||
      lastSolve_->status == SolveStatus::IterationLimit ||
      lastSolve_->status == SolveStatus::Cancelled ||
      lastSolve_->status == SolveStatus::Stale)
    return ManipulationOutcome::Reverted;
  return ManipulationOutcome::Preview;
}
ManipulationOutcome ConstrainedDragController::finish() {
  lastSolve_ = solve(SolveMode::Final);
  const bool converged = lastSolve_->status == SolveStatus::Converged ||
                         lastSolve_->status == SolveStatus::UnderConstrained ||
                         lastSolve_->status == SolveStatus::FullyConstrained ||
                         lastSolve_->status == SolveStatus::Redundant;
  return manipulator_.finish(converged);
}
void ConstrainedDragController::setCurrentRevision(
    std::uint64_t revision) noexcept {
  currentRevision_ = revision;
}
const std::optional<SolveResult> &
ConstrainedDragController::lastSolve() const noexcept {
  return lastSolve_;
}
} // namespace duomec::assembly
