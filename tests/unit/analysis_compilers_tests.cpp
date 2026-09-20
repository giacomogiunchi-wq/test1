#include "duomec/assembly/analysis_compilers.hpp"

#include <cassert>

namespace {
using namespace duomec::assembly;
RelationEndpoint endpoint(duomec::cad::OccurrenceId occurrence,
                          GeometryKind kind) {
  RelationEndpoint value;
  value.occurrenceId = occurrence;
  value.geometry.kind = kind;
  return value;
}
class CountingResolver final : public IAnalysisGeometryResolver {
public:
  mutable std::size_t calls{};
  duomec::core::Result<ResolvedAnalysisGeometry>
  resolve(duomec::cad::OccurrenceId,
          duomec::cad::TopologyReferenceId reference) const override {
    ++calls;
    return duomec::core::Result<ResolvedAnalysisGeometry>::success(
        {reference, "analysis-only-brep-handle"});
  }
};
} // namespace

int main() {
  using namespace duomec::assembly;
  ComponentOccurrence a, b, outside;
  a.placement = OccurrenceState::interactive({});
  b.placement = OccurrenceState::interactive({});
  outside.placement = OccurrenceState::interactive({});
  const std::vector occurrences{a, b, outside};
  const AnalysisScope scope{{a.placement.id, b.placement.id}, "assembly-r42"};

  auto hinge = makeHinge(endpoint(a.placement.id, GeometryKind::Axis),
                         endpoint(b.placement.id, GeometryKind::Axis));
  AssemblyRelation concentric;
  concentric.type = RelationType::Concentric;
  concentric.endpoints = {endpoint(a.placement.id, GeometryKind::Cylinder),
                          endpoint(b.placement.id, GeometryKind::Cylinder)};
  AssemblyRelation coincident;
  coincident.type = RelationType::Coincident;
  coincident.endpoints = {endpoint(a.placement.id, GeometryKind::Plane),
                          endpoint(b.placement.id, GeometryKind::Plane)};
  AssemblyRelation unrelated;
  unrelated.type = RelationType::Lock;
  unrelated.endpoints = {endpoint(outside.placement.id, GeometryKind::Plane)};
  std::vector relations{concentric, coincident, hinge, unrelated};

  CountingResolver resolver;
  AnalysisModelCache<CompiledKinematicModel> motionCache;
  AnalysisModelCache<CompiledFemHints> femCache;
  assert(motionCache.size() == 0 && femCache.size() == 0 &&
         resolver.calls == 0);

  SemanticMultibodyRelationExporter exporter;
  OnDemandKinematicModelCompiler motionCompiler;
  const auto motion =
      motionCompiler.compile(scope, occurrences, relations, exporter);
  assert(motion && resolver.calls == 0);
  assert(motion.value().bodies.size() == 2);
  // The high-level hinge replaces the redundant coincident/concentric pair.
  assert(motion.value().entities.size() == 1);
  assert(motion.value().entities.front().kind == KinematicEntityKind::Revolute);
  assert(motion.value().entities.front().reactionFrames.size() == 2);
  motionCache.store("motion", scope, motion.value());
  assert(motionCache.size() == 1);

  FemRelationHintExtractor extractor;
  const auto coincidentHint = extractor.extract(coincident);
  assert(coincidentHint &&
         coincidentHint->kind == FemCandidateKind::ContactOrBonded);
  assert(coincidentHint->requiresReview && !coincidentHint->confirmed);
  OnDemandAssemblyToFemCompiler femCompiler;
  const auto fem = femCompiler.compile(scope, relations, extractor, resolver);
  assert(fem && fem.value().candidates.size() == 3);
  assert(resolver.calls == 6); // only three in-scope two-endpoint relations
  for (const auto &candidate : fem.value().candidates)
    assert(candidate.requiresReview && !candidate.confirmed);
  femCache.store("fem", scope, fem.value());

  // Analysis caches are independent and unrelated occurrence edits survive.
  motionCache.invalidate(outside.placement.id);
  femCache.invalidate(outside.placement.id);
  assert(motionCache.size() == 1 && femCache.size() == 1);
  motionCache.invalidate(a.placement.id);
  assert(motionCache.size() == 0 && femCache.size() == 1);

  // Normal graph/solver/browser operations never request exact geometry.
  const auto beforeNormalRuntime = resolver.calls;
  AssemblyRelationGraph graph;
  assert(graph.add(hinge));
  assert(graph.add(unrelated));
  assert(graph.islands().size() == 2);
  NativeAssemblyConstraintSolver solver;
  const std::array pair{a.placement, b.placement};
  const std::array oneRelation{hinge};
  (void)solver.solve(pair, oneRelation, {});
  RelationBrowserModel browser;
  (void)browser.group(relations, RelationBrowserGrouping::Status, &graph);
  assert(resolver.calls == beforeNormalRuntime);

  // Invalid local frames cannot become reaction frames.
  auto invalidHinge = hinge;
  invalidHinge.endpoints.front().localFrame.xAxis = {0, 0, 0};
  assert(!exporter.exportRelation(invalidHinge));

  // All requested future motion mappings are stable and backend-neutral.
  const std::array mappingRelations{
      groundOccurrence(a.placement.id),
      AssemblyRelation{
          .type = RelationType::Lock,
          .endpoints = {endpoint(a.placement.id, GeometryKind::Plane),
                        endpoint(b.placement.id, GeometryKind::Plane)}},
      hinge,
      AssemblyRelation{.type = RelationType::Generic,
                       .motion = {.semantic = MotionSemantic::Prismatic}},
      makeUniversalJoint(
          endpoint(a.placement.id, GeometryKind::CoordinateFrame),
          endpoint(b.placement.id, GeometryKind::CoordinateFrame), {}),
      makeScrew(endpoint(a.placement.id, GeometryKind::Axis),
                endpoint(b.placement.id, GeometryKind::Axis), {0.01})
          .value(),
      makeGear(endpoint(a.placement.id, GeometryKind::Axis),
               endpoint(b.placement.id, GeometryKind::Axis), {2})
          .value(),
      makeRackPinion(endpoint(a.placement.id, GeometryKind::Line),
                     endpoint(b.placement.id, GeometryKind::Axis), {0.1})
          .value(),
      makeBeltChain({endpoint(a.placement.id, GeometryKind::Axis),
                     endpoint(b.placement.id, GeometryKind::Axis)},
                    {{0.1, 0.2}})
          .value(),
      makePathRelation(endpoint(a.placement.id, GeometryKind::Point),
                       endpoint(b.placement.id, GeometryKind::Path), {})
          .value()};
  const std::array expected{
      KinematicEntityKind::FixedBody,  KinematicEntityKind::FixedJoint,
      KinematicEntityKind::Revolute,   KinematicEntityKind::Prismatic,
      KinematicEntityKind::Universal,  KinematicEntityKind::Helical,
      KinematicEntityKind::Gear,       KinematicEntityKind::RackPinion,
      KinematicEntityKind::BeltPulley, KinematicEntityKind::Trajectory};
  for (std::size_t index = 0; index < mappingRelations.size(); ++index)
    assert(exporter.exportRelation(mappingRelations[index]).value().kind ==
           expected[index]);
}
