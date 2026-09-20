#include "duomec/assembly/advanced_relations.hpp"
#include "duomec/runtime/performance.hpp"

#include <chrono>
#include <iostream>

int main() {
  using namespace duomec::assembly;
  using namespace duomec::runtime;
  std::vector<AssemblyRelation> relations;
  relations.reserve(10000);
  for (std::size_t index = 0; index < 5000; ++index) {
    OccurrenceState a = OccurrenceState::interactive({});
    OccurrenceState b = OccurrenceState::interactive({});
    AssemblyRelation first;
    first.type = RelationType::Distance;
    RelationEndpoint ea, eb;
    ea.occurrenceId = a.id;
    eb.occurrenceId = b.id;
    first.endpoints = {ea, eb};
    first.parameters["distance_si"] = 1.0;
    auto second = first;
    second.id = duomec::cad::AssemblyRelationId::generate();
    if (index % 10 == 0)
      second.parameters["distance_si"] = 2.0;
    relations.push_back(std::move(first));
    relations.push_back(std::move(second));
  }
  RelationDiagnosticService service;
  const auto start = std::chrono::steady_clock::now();
  const auto conflicts = service.localize(relations);
  const auto end = std::chrono::steady_clock::now();
  std::cout << "relations=" << relations.size()
            << ",conflict_sets=" << conflicts.size() << ",diagnostic_us="
            << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                     start)
                   .count()
            << '\n';
  return conflicts.size() == 500 ? 0 : 1;
}
