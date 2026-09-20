#include "duomec/assembly/runtime_model.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
template <class Function> long long measure(Function &&function) {
  const auto start = Clock::now();
  function();
  return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() -
                                                               start)
      .count();
}
} // namespace

int main() {
  using namespace duomec::assembly;
  auto registry = std::make_shared<DefinitionRegistry>();
  PartDefinition definition;
  PartRevision revision;
  revision.definitionId = definition.id;
  revision.authoringHash = AuthoringHash("m5-1b-authoring");
  revision.geometryHash = GeometryHash("m5-1b-geometry");
  revision.displayRelevantHash = DisplayRelevantHash("m5-1b-display");
  definition.defaultRevision = revision.id;
  if (!registry->addPartDefinition(definition) ||
      !registry->commitPartRevision(revision))
    return 1;
  const auto reference = registry->reference(definition.id, revision.id);
  if (!reference)
    return 1;

  AssemblyRuntimeGraph graph(registry);
  std::vector<duomec::cad::OccurrenceId> ids;
  ids.reserve(50'000);
  const auto create10k = measure([&] {
    for (std::size_t index = 0; index < 10'000; ++index) {
      AssemblyOccurrence occurrence;
      occurrence.reference = reference.value();
      occurrence.localTransform.matrix[12] = static_cast<double>(index);
      const auto result = graph.addOccurrence(std::move(occurrence));
      if (!result)
        std::terminate();
      ids.push_back(result.value());
    }
  });
  const auto createRemaining40k = measure([&] {
    for (std::size_t index = 10'000; index < 50'000; ++index) {
      AssemblyOccurrence occurrence;
      occurrence.reference = reference.value();
      occurrence.localTransform.matrix[12] = static_cast<double>(index);
      const auto result = graph.addOccurrence(std::move(occurrence));
      if (!result)
        std::terminate();
      ids.push_back(result.value());
    }
  });
  const auto transforms = measure([&] {
    Transform transform;
    for (std::size_t index = 0; index < ids.size(); ++index) {
      transform.matrix[12] = static_cast<double>(index + 1);
      if (!graph.updateOccurrenceTransform(ids[index], transform))
        std::terminate();
    }
  });
  const auto hierarchyLookup = measure([&] {
    for (const auto &id : ids)
      if (graph.pathTo(id).value().size() != 1)
        std::terminate();
  });
  const auto registryLookup = measure([&] {
    for (std::size_t index = 0; index < ids.size(); ++index)
      if (registry->partRevision(revision.id).get() == nullptr)
        std::terminate();
  });
  if (graph.size() != 50'000 ||
      std::get<PartDefinitionRef>(graph.find(ids.front())->reference) !=
          std::get<PartDefinitionRef>(graph.find(ids.back())->reference) ||
      registry->partRevision(revision.id).get() !=
          registry->partRevision(revision.id).get())
    return 2;

  std::cout << "operation,count,microseconds\n"
            << "create_repeated,10000," << create10k << '\n'
            << "create_repeated_incremental,40000," << createRemaining40k
            << '\n'
            << "transform_update,50000," << transforms << '\n'
            << "hierarchy_lookup,50000," << hierarchyLookup << '\n'
            << "definition_revision_lookup,50000," << registryLookup << '\n';
}
