#include "duomec/assembly/lifecycle.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>

namespace {
duomec::assembly::RelationEndpoint
endpoint(duomec::cad::OccurrenceId occurrence,
         duomec::cad::TopologyReferenceId reference,
         duomec::assembly::GeometryKind kind) {
  duomec::assembly::RelationEndpoint value;
  value.occurrenceId = std::move(occurrence);
  value.topologyReferenceId = std::move(reference);
  value.geometry.kind = kind;
  return value;
}
} // namespace

int main() {
  using namespace duomec::assembly;
  VirtualComponentCommands virtuals;
  auto oldPart = virtuals.newPart("Old Part");
  oldPart.sharedAssets = {"brep", "mesh", "edges", "selection"};
  oldPart.revisionHash = "revision-a";
  oldPart.authoring.featureHistory = {"Sketch", "Pad"};
  oldPart.authoring.bodies = {"Body"};
  oldPart.authoring.materials = {"Steel"};
  MateReferenceDefinition oldMate;
  oldMate.name = "Mount";
  oldMate.geometry.kind = GeometryKind::Plane;
  oldPart.references.mateReferences.push_back(oldMate);

  auto newPart = virtuals.newPart("New Part");
  newPart.sharedAssets = oldPart.sharedAssets;
  MateReferenceDefinition newMate = oldMate;
  newMate.reference = duomec::cad::TopologyReferenceId::generate();
  newPart.references.mateReferences.push_back(newMate);
  auto virtualAssembly = virtuals.newSubassembly("Virtual Assembly");
  assert(oldPart.storage == DefinitionStorage::Virtual);
  assert(virtualAssembly.kind == DefinitionKind::Subassembly);

  InsertComponentCommand insert;
  std::vector<DefinitionId> definitions{oldPart.id, oldPart.id};
  auto defaultInserted = insert.insert(definitions, std::nullopt);
  assert(defaultInserted.size() == 2);
  for (const auto &occurrence : defaultInserted) {
    assert(occurrence.placement.localTransform == Transform{});
    assert(occurrence.placement.mobility == PlacementMobility::Fixed);
  }
  Transform pose;
  pose.matrix[12] = 4.0;
  auto interactive = insert.insert(
      std::span<const DefinitionId>(definitions.data(), 1), std::nullopt, pose);
  assert(interactive.front().placement.localTransform == pose);
  assert(interactive.front().placement.mobility == PlacementMobility::Floating);

  AssemblyLifecycleSnapshot snapshot;
  snapshot.definitions = {oldPart, newPart, virtualAssembly};
  snapshot.occurrences = defaultInserted;
  snapshot.occurrences.front().visible = false;
  snapshot.occurrences.front().suppressed = true;
  snapshot.occurrences.front().appearanceOverrides["color"] = "blue";
  snapshot.occurrences.front().metadata["item"] = "A";
  assert(setPlacementMobility(snapshot,
                              snapshot.occurrences.front().placement.id,
                              PlacementMobility::Floating));
  assert(setSubassemblySolveMode(snapshot,
                                 snapshot.occurrences.front().placement.id,
                                 SubassemblySolveMode::Flexible));
  assert(snapshot.occurrences.front().placement.subassemblyMode ==
         SubassemblySolveMode::Flexible);

  AssemblyRelation mappedRelation;
  mappedRelation.type = RelationType::Coincident;
  mappedRelation.endpoints.push_back(
      endpoint(snapshot.occurrences.front().placement.id, oldMate.reference,
               GeometryKind::Plane));
  snapshot.relations.relations.push_back(mappedRelation);
  ReplaceComponentCommand replace;
  const auto originalId = snapshot.occurrences.front().placement.id;
  const auto originalPose =
      snapshot.occurrences.front().placement.localTransform;
  std::array selected{originalId};
  const auto report = replace.execute(snapshot, selected, newPart.id, false);
  assert(report && report.value().replacedOccurrences.size() == 1);
  assert(report.value().relationStates.at(mappedRelation.id) ==
         ReferenceMigrationState::Resolved);
  assert(snapshot.occurrences.front().placement.id == originalId);
  assert(snapshot.occurrences.front().placement.localTransform == originalPose);
  assert(snapshot.occurrences.front().appearanceOverrides.at("color") ==
         "blue");
  assert(snapshot.relations.relations.front()
             .endpoints.front()
             .topologyReferenceId == newMate.reference);
  assert(snapshot.relations.relations.front().state == RelationState::Active);

  AssemblyLifecycleSnapshot replaceAll;
  replaceAll.definitions = {oldPart, virtualAssembly};
  replaceAll.occurrences = defaultInserted;
  const std::array replaceOne{replaceAll.occurrences.front().placement.id};
  const auto allReport =
      replace.execute(replaceAll, replaceOne, virtualAssembly.id, true);
  assert(allReport && allReport.value().replacedOccurrences.size() == 2);
  for (const auto &occurrence : replaceAll.occurrences)
    assert(occurrence.definition == virtualAssembly.id);

  AssemblyRelation unresolved;
  unresolved.endpoints.push_back(endpoint(
      snapshot.occurrences.back().placement.id,
      duomec::cad::TopologyReferenceId::generate(), GeometryKind::Sphere));
  snapshot.relations.relations.push_back(unresolved);
  std::array secondSelected{snapshot.occurrences.back().placement.id};
  const auto unresolvedReport =
      replace.execute(snapshot, secondSelected, newPart.id, false);
  assert(unresolvedReport.value().relationStates.at(unresolved.id) ==
         ReferenceMigrationState::DanglingReference);
  assert(snapshot.relations.relations.back().state ==
         RelationState::DanglingReference);

  // Make Independent preserves occurrence state and reuses immutable assets.
  MakeIndependentCommand independent;
  const auto independentResult =
      independent.execute(snapshot, selected, DefinitionStorage::Virtual);
  assert(independentResult && independentResult.value().size() == 1);
  assert(snapshot.occurrences.front().placement.id == originalId);
  const auto &independentDefinition = snapshot.definitions.back();
  assert(independentDefinition.sharedAssets == newPart.sharedAssets);
  assert(!std::holds_alternative<duomec::cad::PartDefinitionId>(
             snapshot.occurrences.front().definition) ||
         std::get<duomec::cad::PartDefinitionId>(
             snapshot.occurrences.front().definition) !=
             std::get<duomec::cad::PartDefinitionId>(newPart.id));

  // Form subassembly moves internal ownership, keeps cross-boundary ownership.
  AssemblyLifecycleSnapshot hierarchy;
  hierarchy.definitions = {oldPart};
  auto children = insert.insert(definitions, std::nullopt);
  auto outsider = insert.insert(
      std::span<const DefinitionId>(definitions.data(), 1), std::nullopt);
  hierarchy.occurrences = children;
  hierarchy.occurrences.push_back(outsider.front());
  AssemblyRelation internal;
  internal.endpoints = {endpoint(children[0].placement.id,
                                 oldPart.references.absoluteReferences
                                     .at(AbsoluteReferenceKind::Origin)
                                     .referenceId,
                                 GeometryKind::Point),
                        endpoint(children[1].placement.id,
                                 oldPart.references.absoluteReferences
                                     .at(AbsoluteReferenceKind::Origin)
                                     .referenceId,
                                 GeometryKind::Point)};
  AssemblyRelation cross = internal;
  cross.id = duomec::cad::AssemblyRelationId::generate();
  cross.endpoints.back().occurrenceId = outsider.front().placement.id;
  hierarchy.relations.relations = {internal, cross};
  FormSubassemblyCommand form;
  std::array childIds{children[0].placement.id, children[1].placement.id};
  const auto formed =
      form.execute(hierarchy, childIds, DefinitionStorage::Virtual);
  assert(formed);
  assert(hierarchy.occurrences[0].parent == formed.value());
  assert(hierarchy.occurrences[1].parent == formed.value());
  assert(hierarchy.occurrences[0].placement.localTransform ==
         children[0].placement.localTransform);
  assert(hierarchy.relationOwners.contains(internal.id));
  assert(!hierarchy.relationOwners.contains(cross.id));

  // Mate references drive a descriptor-only smart insertion proposal.
  SmartInsertionEngine smart;
  ComponentOccurrence smartOccurrence = interactive.front();
  const auto target = endpoint(hierarchy.occurrences.back().placement.id,
                               newMate.reference, GeometryKind::Plane);
  const auto preview = smart.preview(smartOccurrence, newPart, target, newMate);
  assert(preview.compatible && preview.proposedRelations.size() == 1);
  snapshot.occurrences.push_back(smartOccurrence);
  assert(smart.commit(snapshot, smartOccurrence.placement.id, preview));
  assert(snapshot.occurrences.back().placement.mobility ==
         PlacementMobility::Floating);

  // Virtual-to-external export is atomic and preserves occurrence identity.
  const auto path = std::filesystem::temp_directory_path() /
                    "duomec-m5-2-3-virtual.duomecpart";
  std::filesystem::remove(path);
  const auto beforeExternalOccurrence = snapshot.occurrences.front();
  assert(virtuals.saveExternally(
      snapshot, snapshot.occurrences.front().definition, path));
  assert(std::filesystem::exists(path));
  assert(snapshot.occurrences.front() == beforeExternalOccurrence);
  std::filesystem::remove(path);

  AssemblyLifecycleSnapshot exportTree;
  exportTree.definitions = {virtualAssembly, oldPart};
  const auto treePath = std::filesystem::temp_directory_path() /
                        "duomec-m5-2-3-subassembly.duomecasm";
  std::filesystem::remove(treePath);
  const std::array embeddedChildren{oldPart.id};
  assert(virtuals.saveExternally(exportTree, virtualAssembly.id, treePath,
                                 embeddedChildren));
  std::ifstream treeFile(treePath, std::ios::binary);
  const std::string treeBytes((std::istreambuf_iterator<char>(treeFile)), {});
  const auto exportedTree = deserializeLifecycle(treeBytes);
  assert(exportedTree && exportedTree.value().definitions.size() == 2);
  assert(exportedTree.value().definitions.front().storage ==
         DefinitionStorage::External);
  std::filesystem::remove(treePath);

  // Full lifecycle persistence and one-command undo/redo.
  const auto encoded = serializeLifecycle(snapshot);
  const auto loaded = deserializeLifecycle(encoded);
  assert(loaded);
  assert(loaded.value().definitions == snapshot.definitions);
  assert(loaded.value().occurrences == snapshot.occurrences);
  assert(loaded.value().relations == snapshot.relations);
  assert(loaded.value().relationOwners == snapshot.relationOwners);
  assert(loaded.value() == snapshot);
  AssemblyLifecycleHistory history(snapshot);
  auto changed = snapshot;
  changed.occurrences.front().visible = true;
  history.commit(changed);
  assert(history.undo() && history.current() == snapshot);
  assert(history.redo() && history.current() == changed);
}
