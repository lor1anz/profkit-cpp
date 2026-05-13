#include "profkit/profiler.hpp"

#include <iostream>
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
  std::uint64_t min_ns = UINT64_MAX;
  std::uint64_t max_ns = 0;
};

class ProfilerStorage {
 public:
  ScopeId RegisterScope(std::string_view name) {
    std::lock_guard lock(mutex_);

    const auto it = ids_by_name_.find(std::string{name});
    if (it != ids_by_name_.end()) {
      return it->second;
    }

    const ScopeId id = static_cast<ScopeId>(scopes_.size());
    scopes_.push_back(ScopeInfo{.name = std::string{name}});
    stats_.push_back(ScopeStat{});
    ids_by_name_.emplace(scopes_.back().name, id);

    return id;
  }

  void AddSample(ScopeId id, std::uint64_t elapsed_ns) {
    std::lock_guard lock(mutex_);

    if (id >= stats_.size()) {
      return;
    }

    auto& stat = stats_[id];
    ++stat.calls;
    stat.total_ns += elapsed_ns;
    stat.min_ns = std::min(stat.min_ns, elapsed_ns);
    stat.max_ns = std::max(stat.max_ns, elapsed_ns);
  }

  void PrintReport() {
    std::lock_guard lock(mutex_);

    std::cout << "\nProfkit report\n";
    std::cout << "==============\n";

    for (ScopeId id = 0; id < scopes_.size(); ++id) {
      const auto& scope = scopes_[id];
      const auto& stat = stats_[id];

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
  std::mutex mutex_;
  std::vector<ScopeInfo> scopes_;
  std::vector<ScopeStat> stats_;
  std::unordered_map<std::string, ScopeId> ids_by_name_;
};

ProfilerStorage& Storage() {
  static ProfilerStorage storage;
  return storage;
}

std::uint64_t ToNs(std::chrono::steady_clock::duration duration) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
}

}  // namespace

ScopeId RegisterScope(std::string_view name) {
  return Storage().RegisterScope(name);
}

ScopeTimer::ScopeTimer(ScopeId scope_id)
    : scope_id_(scope_id), start_(std::chrono::steady_clock::now()) {}

ScopeTimer::~ScopeTimer() {
  const auto end = std::chrono::steady_clock::now();
  Storage().AddSample(scope_id_, ToNs(end - start_));
}

void PrintReport() {
  Storage().PrintReport();
}
}  // namespace profkit