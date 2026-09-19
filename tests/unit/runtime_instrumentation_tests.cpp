#include "duomec/runtime/benchmark_corpus.hpp"
#include "duomec/runtime/performance.hpp"
#include <cassert>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
int main() {
  using namespace duomec::runtime;
  const auto stats = summarize({1ns, 2ns, 3ns, 4ns, 100ns});
  assert(stats.count == 5 && stats.median == 3ns && stats.p95 == 100ns && stats.p99 == 100ns && stats.maximum == 100ns);

  PerformanceRecorder recorder;
  std::vector<std::jthread> threads;
  for (int t = 0; t < 4; ++t) threads.emplace_back([&] { for (int i = 0; i < 1000; ++i) recorder.record("sample", "test", 1ns, 2ns); });
  threads.clear();
  assert(recorder.snapshot().size() == 4000);
  { PerfSpan span(recorder, "span", "test"); }
  assert(recorder.summary("span").count == 1);
  recorder.record("quote\"\n", "escape", 1us, 2us);
  const auto trace = exportChromeTrace(recorder.snapshot());
  assert(trace.starts_with("{\"traceEvents\":["));
  assert(trace.find("quote\\\"\\n") != std::string::npos);

  FrameProfiler frames; frames.record({10ns, 8ns, 2, 3, 4, 5, 6, 7}); frames.record({20ns, {}, 3, 4, 5, 6, 7, 8});
  assert(frames.cpuSummary().maximum == 20ns);
  AssetLoadProfiler assets; assets.record({AssetSource::Memory, 1ns, 1, true}); assets.record({AssetSource::SourceFile, 2ns, 2, false});
  assert(assets.hitRate() == 0.5);
  AssemblyInteractionProfiler interactions; interactions.record(InteractionOperation::Insert, 5ns);
  assert(interactions.summary(InteractionOperation::Insert).median == 5ns);

  const auto corpus = makeLargeAssemblyCorpus();
  assert(corpus.size() == 8 && corpus[1].occurrences == 50000 && corpus.back().networkLatencyMilliseconds == 50);
  const auto decoded = decodeCorpus(encodeCorpus(corpus));
  assert(decoded && decoded.value() == corpus);
  assert(!decodeCorpus("wrong"));
}
