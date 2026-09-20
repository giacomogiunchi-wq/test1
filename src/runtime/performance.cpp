#include "duomec/runtime/performance.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <thread>

namespace duomec::runtime {
namespace {
Nanoseconds percentile(const std::vector<Nanoseconds> &v, double p) {
  if (v.empty())
    return {};
  const auto rank =
      static_cast<std::size_t>(std::ceil(p * static_cast<double>(v.size())));
  const auto index = std::max<std::size_t>(rank, 1) - 1;
  return v[index];
}
std::string escaped(std::string_view value) {
  std::string out;
  for (char c : value) {
    if (c == '"' || c == '\\') {
      out.push_back('\\');
      out.push_back(c);
    } else if (c == '\n')
      out += "\\n";
    else
      out.push_back(c);
  }
  return out;
}
} // namespace

PercentileSummary summarize(std::vector<Nanoseconds> samples) {
  PercentileSummary result;
  if (samples.empty())
    return result;
  std::sort(samples.begin(), samples.end());
  result.count = samples.size();
  result.minimum = samples.front();
  result.median = percentile(samples, .50);
  result.p95 = percentile(samples, .95);
  result.p99 = percentile(samples, .99);
  result.maximum = samples.back();
  const auto total = std::accumulate(
      samples.begin(), samples.end(), std::int64_t{},
      [](std::int64_t sum, Nanoseconds value) { return sum + value.count(); });
  result.mean = Nanoseconds(total / static_cast<std::int64_t>(samples.size()));
  return result;
}

void PerformanceRecorder::record(std::string name, std::string category,
                                 Nanoseconds start, Nanoseconds duration) {
  std::scoped_lock lock(mutex_);
  events_.push_back({nextSequence_++, std::move(name), std::move(category),
                     start, duration,
                     std::hash<std::thread::id>{}(std::this_thread::get_id())});
}
std::vector<PerfEvent> PerformanceRecorder::snapshot() const {
  std::scoped_lock lock(mutex_);
  return events_;
}
PercentileSummary PerformanceRecorder::summary(std::string_view name) const {
  std::vector<Nanoseconds> values;
  for (const auto &event : snapshot())
    if (event.name == name)
      values.push_back(event.duration);
  return summarize(std::move(values));
}
void PerformanceRecorder::clear() {
  std::scoped_lock lock(mutex_);
  events_.clear();
  nextSequence_ = 0;
}

PerfSpan::PerfSpan(PerformanceRecorder &recorder, std::string name,
                   std::string category)
    : recorder_(&recorder), name_(std::move(name)),
      category_(std::move(category)), start_(std::chrono::steady_clock::now()) {
}
PerfSpan::~PerfSpan() {
  const auto end = std::chrono::steady_clock::now();
  recorder_->record(
      name_, category_,
      std::chrono::duration_cast<Nanoseconds>(start_.time_since_epoch()),
      std::chrono::duration_cast<Nanoseconds>(end - start_));
}

std::string exportChromeTrace(const std::vector<PerfEvent> &events) {
  std::vector<PerfEvent> ordered = events;
  std::sort(ordered.begin(), ordered.end(), [](const auto &a, const auto &b) {
    return a.sequence < b.sequence;
  });
  std::ostringstream out;
  out << "{\"traceEvents\":[";
  for (std::size_t i = 0; i < ordered.size(); ++i) {
    const auto &e = ordered[i];
    if (i)
      out << ',';
    out << "{\"name\":\"" << escaped(e.name) << "\",\"cat\":\""
        << escaped(e.category)
        << "\",\"ph\":\"X\",\"ts\":" << e.start.count() / 1000
        << ",\"dur\":" << e.duration.count() / 1000
        << ",\"pid\":1,\"tid\":" << e.threadId << '}';
  }
  return out.str() + "]}";
}

void FrameProfiler::record(FrameSample sample) {
  std::scoped_lock lock(mutex_);
  samples_.push_back(sample);
}
std::vector<FrameSample> FrameProfiler::snapshot() const {
  std::scoped_lock lock(mutex_);
  return samples_;
}
PercentileSummary FrameProfiler::cpuSummary() const {
  std::vector<Nanoseconds> v;
  for (const auto &s : snapshot())
    v.push_back(s.cpuTime);
  return summarize(std::move(v));
}
void AssetLoadProfiler::record(AssetLoadSample sample) {
  std::scoped_lock lock(mutex_);
  samples_.push_back(sample);
}
PercentileSummary AssetLoadProfiler::durationSummary() const {
  std::scoped_lock lock(mutex_);
  std::vector<Nanoseconds> values;
  values.reserve(samples_.size());
  for (const auto &sample : samples_)
    values.push_back(sample.duration);
  return summarize(std::move(values));
}
double AssetLoadProfiler::hitRate() const {
  std::scoped_lock lock(mutex_);
  if (samples_.empty())
    return 0.;
  const auto n = std::count_if(samples_.begin(), samples_.end(),
                               [](const auto &s) { return s.hit; });
  return static_cast<double>(n) / static_cast<double>(samples_.size());
}
void AssemblyInteractionProfiler::record(InteractionOperation operation,
                                         Nanoseconds duration) {
  std::scoped_lock lock(mutex_);
  samples_.emplace_back(operation, duration);
}
PercentileSummary
AssemblyInteractionProfiler::summary(InteractionOperation operation) const {
  std::scoped_lock lock(mutex_);
  std::vector<Nanoseconds> v;
  for (const auto &[op, d] : samples_)
    if (op == operation)
      v.push_back(d);
  return summarize(std::move(v));
}
} // namespace duomec::runtime
