#include "duomec/assembly/lifecycle.hpp"

#include <cassert>
#include <memory>

namespace {
duomec::assembly::Transform translated(double x, double y = 0.0) {
  duomec::assembly::Transform result;
  result.matrix[12] = x;
  result.matrix[13] = y;
  return result;
}
} // namespace

int main() {
  using namespace duomec::assembly;
  auto registry = std::make_shared<DefinitionRegistry>();

  PartDefinition fastener;
  fastener.name = "M8 fastener";
  PartRevision revisionA;
  revisionA.definitionId = fastener.id;
  revisionA.authoringHash = AuthoringHash("authoring-a");
  revisionA.geometryHash = GeometryHash("geometry-shared");
  revisionA.displayRelevantHash = DisplayRelevantHash("display-shared");
  BodyRevision body;
  body.geometryHash = GeometryHash("body-a");
  revisionA.bodies.push_back(body);
  fastener.defaultRevision = revisionA.id;
  assert(registry->addPartDefinition(fastener));
  const auto committedA = registry->commitPartRevision(revisionA);
  assert(committedA);

  PartRevision revisionB = revisionA;
  revisionB.id = duomec::cad::PartRevisionId::generate();
  revisionB.authoringHash = AuthoringHash("authoring-b");
  revisionB.geometryHash = GeometryHash("geometry-b");
  assert(registry->commitPartRevision(revisionB));
  assert(registry->partRevision(revisionA.id) == committedA.value());
  assert(registry->partRevision(revisionB.id)->geometryHash ==
         GeometryHash("geometry-b"));

  PartDefinition wrongDefinition;
  assert(registry->addPartDefinition(wrongDefinition));
  assert(!registry->reference(wrongDefinition.id, revisionA.id));
  const auto partRefResult = registry->reference(fastener.id, revisionA.id);
  assert(partRefResult);
  const auto partRef = partRefResult.value();

  AssemblyRuntimeGraph graph(registry);
  std::vector<duomec::cad::OccurrenceId> repeated;
  repeated.reserve(5'000);
  for (std::size_t index = 0; index < 5'000; ++index) {
    AssemblyOccurrence occurrence;
    occurrence.reference = partRef;
    occurrence.localTransform = translated(static_cast<double>(index));
    const auto inserted = graph.addOccurrence(std::move(occurrence));
    assert(inserted);
    repeated.push_back(inserted.value());
  }
  assert(graph.size() == 5'000);
  assert(std::get<PartDefinitionRef>(graph.find(repeated[0])->reference) ==
         std::get<PartDefinitionRef>(graph.find(repeated[4'999])->reference));
  assert(graph.find(repeated[0])->localTransform !=
         graph.find(repeated[4'999])->localTransform);

  const auto originalHash = partRef.geometryHash;
  const auto transformInvalidation =
      graph.updateOccurrenceTransform(repeated[0], translated(42));
  assert(transformInvalidation);
  assert(transformInvalidation.value().contains(InvalidationDomain::Transform));
  assert(transformInvalidation.value().contains(
      InvalidationDomain::AssemblyBounds));
  assert(!transformInvalidation.value().contains(
      InvalidationDomain::ExactGeometry));
  assert(!transformInvalidation.value().contains(
      InvalidationDomain::Tessellation));
  assert(std::get<PartDefinitionRef>(graph.find(repeated[0])->reference)
             .geometryHash == originalHash);

  const auto visibility = graph.setVisibility(repeated[1], false);
  assert(visibility &&
         visibility.value().contains(InvalidationDomain::Visibility));
  assert(!visibility.value().contains(InvalidationDomain::ExactGeometry));
  const auto appearance =
      graph.setAppearanceOverride(repeated[2], "color", "blue");
  assert(appearance &&
         !appearance.value().contains(InvalidationDomain::ExactGeometry));
  assert(graph.find(repeated[1])->visible == false);
  assert(graph.find(repeated[2])->visible == true);

  // Copy-on-write compatibility: a new logical definition can initially use
  // the same immutable geometry/display identities.
  PartDefinition independent;
  independent.name = "Independent fastener";
  PartRevision independentRevision;
  independentRevision.definitionId = independent.id;
  independentRevision.authoringHash = AuthoringHash("independent-authoring");
  independentRevision.geometryHash = revisionA.geometryHash;
  independentRevision.displayRelevantHash = revisionA.displayRelevantHash;
  independent.defaultRevision = independentRevision.id;
  assert(registry->addPartDefinition(independent));
  assert(registry->commitPartRevision(independentRevision));
  const auto independentRef =
      registry->reference(independent.id, independentRevision.id);
  assert(independentRef);
  const auto replaced =
      graph.replaceReference(repeated[0], independentRef.value());
  assert(replaced);
  assert(std::get<PartDefinitionRef>(graph.find(repeated[0])->reference)
             .definitionId != fastener.id);
  assert(std::get<PartDefinitionRef>(graph.find(repeated[0])->reference)
             .geometryHash == partRef.geometryHash);
  assert(std::get<PartDefinitionRef>(graph.find(repeated[1])->reference)
             .definitionId == fastener.id);

  AssemblyDefinition subassembly;
  subassembly.name = "Nested assembly";
  AssemblyRevision assemblyRevision;
  assemblyRevision.definitionId = subassembly.id;
  assemblyRevision.authoringHash = AuthoringHash("assembly-a");
  subassembly.defaultRevision = assemblyRevision.id;
  assert(registry->addAssemblyDefinition(subassembly));
  assert(registry->commitAssemblyRevision(assemblyRevision));
  const auto assemblyRef =
      registry->reference(subassembly.id, assemblyRevision.id);
  assert(assemblyRef);

  AssemblyOccurrence parent;
  parent.reference = assemblyRef.value();
  parent.localTransform = translated(10, 2);
  const auto parentId = graph.addOccurrence(parent);
  assert(parentId);
  AssemblyOccurrence child;
  child.reference = partRef;
  child.parent = parentId.value();
  child.localTransform = translated(3, 4);
  const auto childId = graph.addOccurrence(child);
  assert(childId);
  assert(graph.parentOf(childId.value()) == parentId.value());
  assert(graph.childrenOf(parentId.value()).front() == childId.value());
  const auto path = graph.pathTo(childId.value());
  assert(path && path.value().size() == 2);
  const auto before = graph.worldTransform(childId.value());
  assert(before && before.value().matrix[12] == 13.0 &&
         before.value().matrix[13] == 6.0);
  assert(graph.reparent(childId.value(), std::nullopt, true));
  assert(graph.find(childId.value())->id == childId.value());
  assert(graph.worldTransform(childId.value()).value() == before.value());
  assert(!graph.reparent(parentId.value(), parentId.value()));

  // Changing one occurrence to committed Revision B leaves every other path
  // on Revision A and reports geometry-scoped invalidation.
  const auto revisionBRef = registry->reference(fastener.id, revisionB.id);
  assert(revisionBRef);
  const auto switched =
      graph.replaceReference(repeated[3], revisionBRef.value());
  assert(switched &&
         switched.value().contains(InvalidationDomain::ExactGeometry));
  assert(std::get<PartDefinitionRef>(graph.find(repeated[3])->reference)
             .revisionId == revisionB.id);
  assert(std::get<PartDefinitionRef>(graph.find(repeated[4])->reference)
             .revisionId == revisionA.id);

  // Existing M5.2 commands operate on the authoritative runtime graph without
  // changing occurrence identity or cloning content identities.
  ReplaceComponentCommand replaceCommand;
  const std::array replaceSelection{repeated[6]};
  const auto runtimeReplace = replaceCommand.execute(
      graph, replaceSelection, independentRef.value(), false);
  assert(runtimeReplace && graph.find(repeated[6])->id == repeated[6]);
  assert(std::get<PartDefinitionRef>(graph.find(repeated[6])->reference)
             .definitionId == independent.id);
  MakeIndependentCommand independentCommand;
  const std::array independentSelection{repeated[7]};
  const auto runtimeIndependent =
      independentCommand.execute(*registry, graph, independentSelection);
  assert(runtimeIndependent && runtimeIndependent.value().size() == 1);
  const auto &independentCopy =
      std::get<PartDefinitionRef>(runtimeIndependent.value().front());
  assert(independentCopy.definitionId != fastener.id);
  assert(independentCopy.geometryHash == partRef.geometryHash);
  assert(graph.find(repeated[7])->id == repeated[7]);
  assert(std::get<PartDefinitionRef>(graph.find(repeated[8])->reference)
             .definitionId == fastener.id);
}
