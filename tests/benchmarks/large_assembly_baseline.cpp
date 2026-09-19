#include "duomec/runtime/benchmark_corpus.hpp"
#include "duomec/runtime/performance.hpp"
#include <chrono>
#include <iostream>

using Clock = std::chrono::steady_clock;
int main() {
  using namespace duomec::runtime;
  const auto corpus = makeLargeAssemblyCorpus();
  std::vector<Nanoseconds> encode, decode, record;
  PerformanceRecorder recorder;
  for (int i = 0; i < 1000; ++i) {
    auto start = Clock::now(); const auto text = encodeCorpus(corpus); auto middle = Clock::now();
    const auto decoded = decodeCorpus(text); auto end = Clock::now();
    if (!decoded) return 1;
    encode.push_back(std::chrono::duration_cast<Nanoseconds>(middle - start));
    decode.push_back(std::chrono::duration_cast<Nanoseconds>(end - middle));
  }
  for (int i = 0; i < 10000; ++i) {
    auto start = Clock::now(); recorder.record("baseline", "instrumentation", Nanoseconds{}, Nanoseconds{1});
    record.push_back(std::chrono::duration_cast<Nanoseconds>(Clock::now() - start));
  }
  const auto print = [](const char *name, const PercentileSummary &s) {
    std::cout << name << ",count=" << s.count << ",median_ns=" << s.median.count()
      << ",p95_ns=" << s.p95.count() << ",p99_ns=" << s.p99.count() << ",max_ns=" << s.maximum.count() << '\n';
  };
  print("corpus_encode", summarize(std::move(encode)));
  print("corpus_decode", summarize(std::move(decode)));
  print("event_record", summarize(std::move(record)));
  const auto trace = exportChromeTrace(recorder.snapshot());
  std::cout << "scenarios=" << corpus.size() << ",trace_bytes=" << trace.size() << '\n';
}
