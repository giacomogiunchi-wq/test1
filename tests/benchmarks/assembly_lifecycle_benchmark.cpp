#include "duomec/assembly/lifecycle.hpp"
#include "duomec/runtime/performance.hpp"

#include <chrono>
#include <iostream>

int main() {
  using namespace duomec::assembly;
  using namespace duomec::runtime;
  VirtualComponentCommands factory;
  auto definition = factory.newPart("Repeated Part");
  AssemblyLifecycleSnapshot snapshot;
  snapshot.definitions.push_back(definition);
  std::vector<DefinitionId> ids(10000, definition.id);
  InsertComponentCommand insert;
  const auto insertStart = std::chrono::steady_clock::now();
  snapshot.occurrences = insert.insert(ids, std::nullopt);
  const auto insertEnd = std::chrono::steady_clock::now();
  MakeIndependentCommand independent;
  std::vector<Nanoseconds> samples;
  for (std::size_t index = 0; index < 100; ++index) {
    auto working = snapshot;
    const std::array selected{working.occurrences[index].placement.id};
    const auto start = std::chrono::steady_clock::now();
    const auto result =
        independent.execute(working, selected, DefinitionStorage::Virtual);
    const auto end = std::chrono::steady_clock::now();
    if (!result || working.definitions.size() != 2)
      return 1;
    samples.push_back(std::chrono::duration_cast<Nanoseconds>(end - start));
  }
  const auto stats = summarize(std::move(samples));
  std::cout << "occurrences=" << snapshot.occurrences.size()
            << ",unique_assets=1,insert_us="
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   insertEnd - insertStart)
                   .count()
            << ",independent_median_ns=" << stats.median.count()
            << ",independent_p95_ns=" << stats.p95.count()
            << ",independent_p99_ns=" << stats.p99.count() << '\n';
}
