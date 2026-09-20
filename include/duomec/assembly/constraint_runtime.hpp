#pragma once

#include "duomec/assembly/relations.hpp"

#include <chrono>
#include <map>
#include <optional>
#include <set>
#include <span>

namespace duomec::assembly {

enum class SolveMode { Interactive, Final };
enum class SolveStatus {
  Converged,
  UnderConstrained,
  FullyConstrained,
  Redundant,
  Conflicting,
  IterationLimit,
  Cancelled,
  Stale
};

struct SolveOptions {
  SolveMode mode{SolveMode::Final};
  std::size_t maximumIterations{100};
  std::chrono::microseconds timeBudget{std::chrono::milliseconds(100)};
  std::uint64_t inputRevision{};
  bool warmStart{true};
};
struct SolveResult {
  SolveStatus status{SolveStatus::Converged};
  std::map<cad::OccurrenceId, Transform> poses;
  std::map<cad::OccurrenceId, DofState> dofStates;
  std::size_t iterations{};
  std::uint64_t inputRevision{};
  std::vector<std::string> diagnostics;
};

class IAssemblyConstraintSolver {
public:
  virtual ~IAssemblyConstraintSolver() = default;
  [[nodiscard]] virtual SolveResult
  solve(std::span<const OccurrenceState> occurrences,
        std::span<const AssemblyRelation> relations,
        const SolveOptions &options) = 0;
};

// Dependency-free semantic baseline. It validates relation consistency and
// computes DOF state; a numeric backend can replace it through the interface.
class NativeAssemblyConstraintSolver final : public IAssemblyConstraintSolver {
public:
  [[nodiscard]] SolveResult solve(std::span<const OccurrenceState> occurrences,
                                  std::span<const AssemblyRelation> relations,
                                  const SolveOptions &options) override;
};

struct SolveIsland {
  cad::SolveIslandId id{cad::SolveIslandId::generate()};
  std::set<cad::OccurrenceId> occurrences;
  std::set<cad::AssemblyRelationId> relations;
};

class AssemblyRelationGraph {
public:
  core::Result<cad::SolveIslandId> add(const AssemblyRelation &relation);
  core::Result<bool> remove(const cad::AssemblyRelationId &relation);
  [[nodiscard]] std::optional<SolveIsland>
  islandFor(const cad::OccurrenceId &occurrence) const;
  [[nodiscard]] const std::vector<SolveIsland> &islands() const noexcept;
  [[nodiscard]] std::vector<AssemblyRelation>
  relationsFor(const SolveIsland &island) const;

private:
  void rebuildIndexes();
  std::map<cad::AssemblyRelationId, AssemblyRelation> relations_;
  std::vector<SolveIsland> islands_;
  std::map<cad::OccurrenceId, cad::SolveIslandId> occurrenceIsland_;
  std::map<cad::SolveIslandId, std::size_t> islandIndex_;
};

enum class Alignment { Aligned, AntiAligned };
struct ConcentricOptions {
  Alignment alignment{Alignment::Aligned};
  bool lockRotation{};
};
struct AngleBranch {
  Vector3 referenceDirection{1, 0, 0};
  Alignment alignment{Alignment::Aligned};
  double signedAngleRadians{};
};
struct InsertOptions {
  Alignment alignment{Alignment::Aligned};
  bool lockRotation{};
};

[[nodiscard]] AssemblyRelation makeConcentric(RelationEndpoint first,
                                              RelationEndpoint second,
                                              ConcentricOptions options = {});
[[nodiscard]] AssemblyRelation
makeAngle(RelationEndpoint first, RelationEndpoint second, AngleBranch branch);
[[nodiscard]] AssemblyRelation makeInsert(RelationEndpoint axis,
                                          RelationEndpoint seat,
                                          InsertOptions options = {});
[[nodiscard]] AssemblyRelation groundOccurrence(cad::OccurrenceId occurrence);

enum class QuickMateCandidate {
  Coincident,
  Distance,
  Parallel,
  Angle,
  Concentric,
  ConcentricLockRotation,
  Frame,
  Tangent
};
class MateCandidateEngine {
public:
  [[nodiscard]] std::vector<QuickMateCandidate>
  candidates(const GeometryDescriptor &first,
             const GeometryDescriptor &second) const;
};
struct QuickMateOverlay {
  bool visible{};
  std::vector<QuickMateCandidate> candidates;
  std::size_t highlighted{};
  Alignment alignment{Alignment::Aligned};
  std::optional<double> inlineValue;
};
class QuickMateController {
public:
  explicit QuickMateController(MateCandidateEngine engine = {});
  const QuickMateOverlay &begin(const RelationEndpoint &first,
                                const RelationEndpoint &second);
  void flip();
  void setInlineValue(double value);
  core::Result<bool> highlight(QuickMateCandidate candidate);
  [[nodiscard]] core::Result<AssemblyRelation> accept();
  void cancel();
  [[nodiscard]] const QuickMateOverlay &overlay() const noexcept;

private:
  MateCandidateEngine engine_;
  QuickMateOverlay overlay_;
  std::optional<RelationEndpoint> first_;
  std::optional<RelationEndpoint> second_;
};

enum class PointerButton { Left, Right };
enum class ManipulationOutcome {
  Preview,
  FullyConstrained,
  ContextMenu,
  Committed,
  Reverted
};
struct ManipulationInput {
  Vector3 pointerDelta;
  double rotationRadians{};
  bool meaningfulDrag{};
};
class ComponentManipulator {
public:
  void begin(std::vector<OccurrenceState> selection,
             std::optional<Vector3> explicitPivot = std::nullopt,
             std::optional<Vector3> groupBoundsCenter = std::nullopt);
  [[nodiscard]] ManipulationOutcome update(PointerButton button,
                                           const ManipulationInput &input);
  [[nodiscard]] ManipulationOutcome finish(bool solveConverged);
  [[nodiscard]] const std::vector<OccurrenceState> &preview() const noexcept;
  [[nodiscard]] Vector3 pivot() const noexcept;

private:
  std::vector<OccurrenceState> original_;
  std::vector<OccurrenceState> preview_;
  Vector3 pivot_{};
  bool dragged_{};
};

class AssemblyStateHistory {
public:
  explicit AssemblyStateHistory(AssemblyRelationsSnapshot initial = {});
  void commit(AssemblyRelationsSnapshot state);
  [[nodiscard]] bool canUndo() const noexcept;
  [[nodiscard]] bool canRedo() const noexcept;
  core::Result<bool> undo();
  core::Result<bool> redo();
  [[nodiscard]] const AssemblyRelationsSnapshot &current() const noexcept;

private:
  std::vector<AssemblyRelationsSnapshot> undo_;
  std::vector<AssemblyRelationsSnapshot> redo_;
  AssemblyRelationsSnapshot current_;
};

class ConstrainedDragController {
public:
  ConstrainedDragController(IAssemblyConstraintSolver &solver,
                            const AssemblyRelationGraph &graph);
  void begin(std::vector<OccurrenceState> selection,
             std::span<const OccurrenceState> assemblyOccurrences,
             std::uint64_t revision);
  [[nodiscard]] ManipulationOutcome update(PointerButton button,
                                           const ManipulationInput &input);
  [[nodiscard]] ManipulationOutcome finish();
  void setCurrentRevision(std::uint64_t revision) noexcept;
  [[nodiscard]] const std::optional<SolveResult> &lastSolve() const noexcept;

private:
  [[nodiscard]] SolveResult solve(SolveMode mode);
  IAssemblyConstraintSolver *solver_;
  const AssemblyRelationGraph *graph_;
  ComponentManipulator manipulator_;
  std::vector<OccurrenceState> assemblyOccurrences_;
  std::vector<AssemblyRelation> islandRelations_;
  std::set<cad::OccurrenceId> islandOccurrences_;
  std::uint64_t inputRevision_{};
  std::uint64_t currentRevision_{};
  std::optional<SolveResult> lastSolve_;
};

} // namespace duomec::assembly
