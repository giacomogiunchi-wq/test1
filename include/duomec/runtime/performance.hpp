#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace duomec::runtime {

using Nanoseconds = std::chrono::nanoseconds;

struct PercentileSummary {
  std::size_t count{};
  Nanoseconds minimum{};
  Nanoseconds median{};
  Nanoseconds p95{};
  Nanoseconds p99{};
  Nanoseconds maximum{};
  Nanoseconds mean{};
};

[[nodiscard]] PercentileSummary summarize(std::vector<Nanoseconds> samples);

struct PerfEvent {
  std::uint64_t sequence{};
  std::string name;
  std::string category;
  Nanoseconds start{};
  Nanoseconds duration{};
  std::uint64_t threadId{};
};

class PerformanceRecorder {
public:
  void record(std::string name, std::string category, Nanoseconds start,
              Nanoseconds duration);
  [[nodiscard]] std::vector<PerfEvent> snapshot() const;
  [[nodiscard]] PercentileSummary summary(std::string_view name) const;
  void clear();

private:
  mutable std::mutex mutex_;
  std::vector<PerfEvent> events_;
  std::uint64_t nextSequence_{};
};

class PerfSpan {
public:
  PerfSpan(PerformanceRecorder &recorder, std::string name,
           std::string category);
  ~PerfSpan();
  PerfSpan(const PerfSpan &) = delete;
  PerfSpan &operator=(const PerfSpan &) = delete;

private:
  PerformanceRecorder *recorder_;
  std::string name_;
  std::string category_;
  std::chrono::steady_clock::time_point start_;
};

[[nodiscard]] std::string exportChromeTrace(const std::vector<PerfEvent> &events);

struct FrameSample {
  Nanoseconds cpuTime{};
  std::optional<Nanoseconds> gpuTime;
  std::uint64_t drawCalls{};
  std::uint64_t triangles{};
  std::uint64_t lineSegments{};
  std::uint64_t visibleOccurrences{};
  std::uint64_t culledOccurrences{};
  std::uint64_t gpuUploadBytes{};
};

class FrameProfiler {
public:
  void record(FrameSample sample);
  [[nodiscard]] PercentileSummary cpuSummary() const;
  [[nodiscard]] std::vector<FrameSample> snapshot() const;
private:
  mutable std::mutex mutex_;
  std::vector<FrameSample> samples_;
};

enum class AssetSource { Memory, LocalCache, SourceFile, Network };
struct AssetLoadSample { AssetSource source{}; Nanoseconds duration{}; std::uint64_t bytes{}; bool hit{}; };
class AssetLoadProfiler {
public:
  void record(AssetLoadSample sample);
  [[nodiscard]] PercentileSummary durationSummary() const;
  [[nodiscard]] double hitRate() const;
private:
  mutable std::mutex mutex_;
  std::vector<AssetLoadSample> samples_;
};

enum class InteractionOperation { Insert, TransformFrame, TransformCommit, SelectOccurrence, SelectFace, Visibility, Isolate, TabSwitch, EnterEdit, FirstEditableFrame, CommitPartEdit };
class AssemblyInteractionProfiler {
public:
  void record(InteractionOperation operation, Nanoseconds duration);
  [[nodiscard]] PercentileSummary summary(InteractionOperation operation) const;
private:
  mutable std::mutex mutex_;
  std::vector<std::pair<InteractionOperation, Nanoseconds>> samples_;
};

} // namespace duomec::runtime
