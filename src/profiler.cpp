#include "profkit/profiler.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
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
};

void AddSample(ScopeStat& stat, std::uint64_t elapsed_ns) {
  ++stat.calls;
  stat.total_ns += elapsed_ns;
  stat.min_ns = std::min(stat.min_ns, elapsed_ns);
  stat.max_ns = std::max(stat.max_ns, elapsed_ns);
}

void MergeStat(ScopeStat& dst, const ScopeStat& src) {
  if (src.calls == 0) {
    return;
  }

  dst.calls += src.calls;
  dst.total_ns += src.total_ns;
  dst.min_ns = std::min(dst.min_ns, src.min_ns);
  dst.max_ns = std::max(dst.max_ns, src.max_ns);
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

    const ScopeId id = static_cast<ScopeId>(scopes_.size());

    scopes_.push_back(ScopeInfo{.name = key});
    ids_by_name_.emplace(scopes_.back().name, id);
    finished_stats_.push_back(ScopeStat{});

    return id;
  }

  std::uint32_t RegisterThread() {
    std::lock_guard lock(mutex_);
    return next_thread_id_++;
  }

  std::size_t ScopeCount() const {
    std::lock_guard lock(mutex_);
    return scopes_.size();
  }

  void MergeFinishedThreadStats(const std::vector<ScopeStat>& stats) {
    std::lock_guard lock(mutex_);

    if (finished_stats_.size() < stats.size()) {
      finished_stats_.resize(stats.size());
    }

    for (ScopeId id = 0; id < stats.size(); ++id) {
      MergeStat(finished_stats_[id], stats[id]);
    }
  }

  void PrintReportWithCurrentThreadStats(const std::vector<ScopeStat>& current_thread_stats) {
    std::lock_guard lock(mutex_);

    std::vector<ScopeStat> total_stats = finished_stats_;

    if (total_stats.size() < current_thread_stats.size()) {
      total_stats.resize(current_thread_stats.size());
    }

    for (ScopeId id = 0; id < current_thread_stats.size(); ++id) {
      MergeStat(total_stats[id], current_thread_stats[id]);
    }

    std::cout << "\nProfkit report\n";
    std::cout << "==============\n";

    for (ScopeId id = 0; id < scopes_.size(); ++id) {
      const auto& scope = scopes_[id];
      const auto& stat = total_stats[id];

      if (stat.calls == 0) {
        continue;
      }

      const double total_ms = static_cast<double>(stat.total_ns) / 1'000'000.0;
      const double avg_us =
          static_cast<double>(stat.total_ns) / static_cast<double>(stat.calls) / 1'000.0;
      const double min_us = static_cast<double>(stat.min_ns) / 1'000.0;
      const double max_us = static_cast<double>(stat.max_ns) / 1'000.0;

      std::cout << scope.name << '\n';
      std::cout << "  calls:    " << stat.calls << '\n';
      std::cout << "  total ms: " << total_ms << '\n';
      std::cout << "  avg us:   " << avg_us << '\n';
      std::cout << "  min us:   " << min_us << '\n';
      std::cout << "  max us:   " << max_us << '\n';
    }
  }

 private:
  mutable std::mutex mutex_;

  std::vector<ScopeInfo> scopes_;
  std::unordered_map<std::string, ScopeId> ids_by_name_;

  std::vector<ScopeStat> finished_stats_;

  std::uint32_t next_thread_id_ = 0;
};

ProfilerStorage& Storage() {
  static auto* storage = new ProfilerStorage();
  return *storage;
}

std::uint64_t ToNs(std::chrono::steady_clock::duration duration) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
}

struct ThreadState {
  ThreadState() : thread_id(Storage().RegisterThread()) {}

  ~ThreadState() {
    Storage().MergeFinishedThreadStats(stats);
  }

  void AddSample(ScopeId id, std::uint64_t elapsed_ns) {
    if (id >= stats.size()) {
      stats.resize(static_cast<std::size_t>(id) + 1);
    }

    ::profkit::AddSample(stats[id], elapsed_ns);
  }

  std::uint32_t thread_id = 0;
  std::vector<ScopeStat> stats;
};

ThreadState& CurrentThreadState() {
  thread_local ThreadState state;
  return state;
}

}  // namespace

ScopeId RegisterScope(std::string_view name) {
  return Storage().RegisterScope(name);
}

ScopeTimer::ScopeTimer(ScopeId scope_id)
    : scope_id_(scope_id), start_(std::chrono::steady_clock::now()) {}

ScopeTimer::~ScopeTimer() {
  const auto end = std::chrono::steady_clock::now();
  CurrentThreadState().AddSample(scope_id_, ToNs(end - start_));
}

void PrintReport() {
  Storage().PrintReportWithCurrentThreadStats(CurrentThreadState().stats);
}
}  // namespace profkit