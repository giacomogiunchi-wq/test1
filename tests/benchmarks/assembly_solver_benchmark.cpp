#include "duomec/assembly/constraint_runtime.hpp"
#include "duomec/runtime/performance.hpp"

#include <chrono>
#include <iostream>

int main() {
  using namespace duomec::assembly;
  using namespace duomec::runtime;
  constexpr std::size_t islandCount = 5000;
  AssemblyRelationGraph graph;
  std::vector<OccurrenceState> occurrences;
  occurrences.reserve(islandCount * 2);
  const auto graphStart = std::chrono::steady_clock::now();
  for (std::size_t i = 0; i < islandCount; ++i) {
    occurrences.push_back(OccurrenceState::interactive({}));
    occurrences.push_back(OccurrenceState::interactive({}));
    AssemblyRelation relation;
    relation.type = RelationType::Distance;
    RelationEndpoint a, b;
    a.occurrenceId = occurrences[i * 2].id;
    b.occurrenceId = occurrences[i * 2 + 1].id;
    relation.endpoints = {a, b};
    relation.parameters["distance_si"] = 0.1;
    if (!graph.add(relation))
      return 1;
  }
  const auto graphEnd = std::chrono::steady_clock::now();
  const auto island = graph.islandFor(occurrences.front().id);
  if (!island || island->occurrences.size() != 2)
    return 2;
  const auto relations = graph.relationsFor(*island);
  NativeAssemblyConstraintSolver solver;
  std::vector<OccurrenceState> local{occurrences[0], occurrences[1]};
  std::vector<Nanoseconds> samples;
  samples.reserve(10000);
  for (std::size_t i = 0; i < 10000; ++i) {
    const auto start = std::chrono::steady_clock::now();
    const auto result = solver.solve(local, relations, {});
    const auto end = std::chrono::steady_clock::now();
    if (result.poses.size() != 2)
      return 3;
    samples.push_back(std::chrono::duration_cast<Nanoseconds>(end - start));
  }
  const auto summary = summarize(std::move(samples));
  std::cout << "islands=" << graph.islands().size() << ",graph_build_us="
            << std::chrono::duration_cast<std::chrono::microseconds>(graphEnd -
                                                                     graphStart)
                   .count()
            << ",local_occurrences=" << local.size()
            << ",median_ns=" << summary.median.count()
            << ",p95_ns=" << summary.p95.count()
            << ",p99_ns=" << summary.p99.count()
            << ",max_ns=" << summary.maximum.count() << '\n';
}
