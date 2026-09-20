#include "duomec/assembly/analysis_compilers.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>

namespace {
using Clock = std::chrono::steady_clock;
struct Stats {
  double median{}, p95{}, p99{}, maximum{};
};
template <class Work> Stats measure(Work work, std::size_t samples = 101) {
  std::vector<double> values;
  values.reserve(samples);
  for (std::size_t i = 0; i < samples; ++i) {
    const auto start = Clock::now();
    work();
    values.push_back(
        std::chrono::duration<double, std::micro>(Clock::now() - start)
            .count());
  }
  std::sort(values.begin(), values.end());
  return {values[values.size() / 2], values[values.size() * 95 / 100],
          values[values.size() * 99 / 100], values.back()};
}
void print(std::string_view name, Stats s) {
  std::cout << name << ",median_us=" << s.median << ",p95_us=" << s.p95
            << ",p99_us=" << s.p99 << ",max_us=" << s.maximum << '\n';
}
duomec::assembly::RelationEndpoint
endpoint(duomec::cad::OccurrenceId id, duomec::assembly::GeometryKind kind) {
  duomec::assembly::RelationEndpoint e;
  e.occurrenceId = id;
  e.geometry.kind = kind;
  return e;
}
} // namespace
int main() {
  using namespace duomec::assembly;
  std::vector<OccurrenceState> occurrences;
  for (int i = 0; i < 300; ++i)
    occurrences.push_back(OccurrenceState::interactive({}));
  std::vector<AssemblyRelation> many;
  for (std::size_t i = 1; i < occurrences.size(); ++i)
    many.push_back(
        makeHinge(endpoint(occurrences[i - 1].id, GeometryKind::Axis),
                  endpoint(occurrences[i].id, GeometryKind::Axis)));
  print("many_small_islands", measure(
                                  [&] {
                                    AssemblyRelationGraph g;
                                    for (std::size_t i = 0; i < many.size();
                                         i += 2)
                                      (void)g.add(many[i]);
                                  },
                                  7));
  print("one_large_island", measure(
                                [&] {
                                  AssemblyRelationGraph g;
                                  for (const auto &r : many)
                                    (void)g.add(r);
                                },
                                5));
  std::vector<AssemblyRelation> thousands;
  thousands.reserve(5000);
  for (int i = 0; i < 5000; ++i)
    thousands.push_back(many[static_cast<std::size_t>(i) % many.size()]);
  RelationDiagnosticService diagnostics;
  print("thousands_relations",
        measure([&] { (void)diagnostics.localize(thousands); }, 31));
  MateCandidateEngine candidates;
  GeometryDescriptor plane{.kind = GeometryKind::Plane};
  print("quick_mate",
        measure([&] { (void)candidates.candidates(plane, plane); }));
  NativeAssemblyConstraintSolver solver;
  AssemblyRelationGraph graph;
  (void)graph.add(many.front());
  print("constrained_drag", measure([&] {
          (void)solver.solve(std::span(occurrences).first(2),
                             std::span(many).first(1),
                             {.mode = SolveMode::Interactive});
        }));
  AssemblyLifecycleSnapshot snapshot;
  ComponentDefinition definition;
  snapshot.definitions.push_back(definition);
  InsertComponentCommand insert;
  auto inserted =
      insert.insert(std::array<DefinitionId, 1>{definition.id}, std::nullopt);
  snapshot.occurrences = inserted;
  print("flexible_subassembly", measure([&] {
          (void)setSubassemblySolveMode(snapshot, inserted.front().placement.id,
                                        SubassemblySolveMode::Flexible);
        }));
  ReplacementReferenceMapper mapper;
  ReplaceComponentCommand replace;
  print("replace_component",
        measure(
            [&] {
              auto copy = snapshot;
              (void)replace.execute(copy,
                                    std::array{inserted.front().placement.id},
                                    definition.id, false, mapper);
            },
            31));
  FormSubassemblyCommand form;
  print("create_subassembly",
        measure(
            [&] {
              auto copy = snapshot;
              (void)form.execute(copy,
                                 std::array{inserted.front().placement.id},
                                 DefinitionStorage::Virtual);
            },
            31));
  MakeIndependentCommand independent;
  print("make_independent", measure(
                                [&] {
                                  auto copy = snapshot;
                                  (void)independent.execute(
                                      copy,
                                      std::array{inserted.front().placement.id},
                                      DefinitionStorage::Virtual);
                                },
                                31));
  AssemblyRelation origin;
  origin.type = RelationType::Coincident;
  origin.endpoints = {endpoint(occurrences[0].id, GeometryKind::Point),
                      endpoint(occurrences[1].id, GeometryKind::Point)};
  print("origin_plane_mate", measure([&] {
          AssemblyRelationGraph g;
          (void)g.add(origin);
        }));
  RelationBrowserModel browser;
  print("relation_browser",
        measure(
            [&] {
              (void)browser.group(many, RelationBrowserGrouping::Mechanical);
            },
            31));
}
