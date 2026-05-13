#include "profkit/profiler.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace profkit {
namespace {
struct ScopeInfo {
  std::string name;
};

struct ScopeStat {
  std::uint64_t calls = 0;

  std::uint64_t total_ns = 0;
  std::uint64_t min_ns = std::numeric_limits<std::uint64_t>::max();
  std::uint64_t max_ns = 0;

  std::uint64_t first_start_ns = std::numeric_limits<std::uint64_t>::max();
  std::uint64_t last_end_ns = 0;
};

void AddSample(ScopeStat& stat, std::uint64_t start_ns, std::uint64_t end_ns) {
  const std::uint64_t elapsed_ns = end_ns - start_ns;

  ++stat.calls;

  stat.total_ns += elapsed_ns;
  stat.min_ns = std::min(stat.min_ns, elapsed_ns);
  stat.max_ns = std::max(stat.max_ns, elapsed_ns);

  stat.first_start_ns = std::min(stat.first_start_ns, start_ns);
  stat.last_end_ns = std::max(stat.last_end_ns, end_ns);
}

void MergeStat(ScopeStat& dst, const ScopeStat& src) {
  if (src.calls == 0) {
    return;
  }

  dst.calls += src.calls;
  dst.total_ns += src.total_ns;

  dst.min_ns = std::min(dst.min_ns, src.min_ns);
  dst.max_ns = std::max(dst.max_ns, src.max_ns);

  dst.first_start_ns = std::min(dst.first_start_ns, src.first_start_ns);
  dst.last_end_ns = std::max(dst.last_end_ns, src.last_end_ns);
}

class ProfilerStorage {
 public:
  ScopeId RegisterScope(std::string_view name) {
    std::lock_guard lock(mutex_);

    const std::string key{name};
    const auto it = ids_by_name_.find(key);
    if (it != ids_by_name_.end()) {
      return it->second;
    }

    const auto id = static_cast<ScopeId>(scopes_.size());

    scopes_.push_back(ScopeInfo{.name = key});
    ids_by_name_.emplace(scopes_.back().name, id);

    return id;
  }

  std::uint32_t RegisterThread() {
    std::lock_guard lock(mutex_);
    return next_thread_id_++;
  }

  void AddFinishedThreadStats(std::vector<ScopeStat> stats) {
    std::lock_guard lock(mutex_);
    finished_thread_stats_.push_back(std::move(stats));
  }

  void PrintReportWithCurrentThreadStats(const std::vector<ScopeStat>& current_thread_stats) {
    std::lock_guard lock(mutex_);

    std::vector<std::vector<ScopeStat>> all_thread_stats = finished_thread_stats_;
    all_thread_stats.push_back(current_thread_stats);

    std::vector<ScopeStat> total_stats(scopes_.size());
    std::vector<std::uint64_t> max_thread_total_ns(scopes_.size(), 0);

    for (const auto& thread_stats : all_thread_stats) {
      for (ScopeId id = 0; id < thread_stats.size() && id < scopes_.size(); ++id) {
        const auto& stat = thread_stats[id];

        if (stat.calls == 0) {
          continue;
        }

        MergeStat(total_stats[id], stat);
        max_thread_total_ns[id] = std::max(max_thread_total_ns[id], stat.total_ns);
      }
    }

    PrintTable(total_stats, max_thread_total_ns);
  }

  void Reset() {
    std::lock_guard lock(mutex_);
    finished_thread_stats_.clear();
  }

 private:
  void PrintTable(const std::vector<ScopeStat>& total_stats,
                  const std::vector<std::uint64_t>& max_thread_total_ns) const {
    std::cout << "\nProfiler report\n";
    std::cout << std::string(145, '=') << "\n\n";

    // clang-format off
    std::cout << std::left << std::setw(45) << "scope"
              << std::right << std::setw(10) << "calls"
              << std::setw(14) << "wall ms"
              << std::setw(16) << "cpu total ms"
              << std::setw(14) << "avg ms"
              << std::setw(14) << "min ms"
              << std::setw(14) << "max ms"
              << std::setw(18) << "max thread ms"
              << '\n';
    // clang-format on

    std::cout << std::string(145, '-') << '\n';

    for (ScopeId id = 0; id < scopes_.size(); ++id) {
      const auto& stat = total_stats[id];

      if (stat.calls == 0) {
        continue;
      }

      const double wall_ms =
          static_cast<double>(stat.last_end_ns - stat.first_start_ns) / 1'000'000.0;
      const double cpu_total_ms = static_cast<double>(stat.total_ns) / 1'000'000.0;
      const double avg_ms =
          static_cast<double>(stat.total_ns) / static_cast<double>(stat.calls) / 1'000'000.0;
      const double min_ms = static_cast<double>(stat.min_ns) / 1'000'000.0;
      const double max_ms = static_cast<double>(stat.max_ns) / 1'000'000.0;
      const double max_thread_ms = static_cast<double>(max_thread_total_ns[id]) / 1'000'000.0;

      // clang-format off
      std::cout << std::left << std::setw(45) << scopes_[id].name
                << std::right << std::setw(10) << stat.calls
                << std::setw(14) << std::fixed << std::setprecision(3) << wall_ms
                << std::setw(16) << cpu_total_ms
                << std::setw(14) << avg_ms
                << std::setw(14) << min_ms
                << std::setw(14) << max_ms
                << std::setw(18) << max_thread_ms
                << '\n';
      // clang-format on
    }
  }

  mutable std::mutex mutex_;

  std::vector<ScopeInfo> scopes_;
  std::unordered_map<std::string, ScopeId> ids_by_name_;

  std::vector<std::vector<ScopeStat>> finished_thread_stats_;

  std::uint32_t next_thread_id_ = 0;
};

ProfilerStorage& Storage() {
  static auto* storage = new ProfilerStorage();
  return *storage;
}

std::uint64_t NowNs() {
  static const auto start = std::chrono::steady_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start)
          .count());
}

std::uint64_t ToNs(std::chrono::steady_clock::duration duration) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
}

struct ThreadState {
  ThreadState() : thread_id(Storage().RegisterThread()) {}

  ~ThreadState() {
    Storage().AddFinishedThreadStats(std::move(stats));
  }

  void AddSample(ScopeId id, std::uint64_t start_ns, std::uint64_t end_ns) {
    if (id >= stats.size()) {
      stats.resize(static_cast<std::size_t>(id) + 1);
    }

    ::profkit::AddSample(stats[id], start_ns, end_ns);
  }

  std::uint32_t thread_id = 0;
  std::vector<ScopeStat> stats;
};

ThreadState& CurrentThreadState() {
  thread_local ThreadState state;
  return state;
}

void ResetCurrentThreadStats() {
  CurrentThreadState().stats.clear();
}

}  // namespace

ScopeId RegisterScope(std::string_view name) {
  return Storage().RegisterScope(name);
}

ScopeTimer::ScopeTimer(ScopeId scope_id) : scope_id_(scope_id), start_ns_(NowNs()) {}

ScopeTimer::~ScopeTimer() {
  Stop();
}

void ScopeTimer::Stop() {
  if (stopped_) {
    return;
  }

  CurrentThreadState().AddSample(scope_id_, start_ns_, NowNs());
  stopped_ = true;
}

Session::Session(std::string_view name) : timer_(RegisterScope(name)) {}

Session::~Session() {
  timer_.Stop();
  PrintReport();
}

void PrintReport() {
  Storage().PrintReportWithCurrentThreadStats(CurrentThreadState().stats);
}

void Reset() {
  Storage().Reset();
  ResetCurrentThreadStats();
}

void DumpAndReset() {
  PrintReport();
  Reset();
}
}  // namespace profkit