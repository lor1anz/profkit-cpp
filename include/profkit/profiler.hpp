#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

namespace profkit {

using ScopeId = std::uint32_t;

ScopeId RegisterScope(std::string_view name);

class ScopeTimer {
 public:
  explicit ScopeTimer(ScopeId scope_id);
  ~ScopeTimer();

  ScopeTimer(const ScopeTimer&) = delete;
  ScopeTimer& operator=(const ScopeTimer&) = delete;

  ScopeTimer(ScopeTimer&&) = delete;
  ScopeTimer& operator=(ScopeTimer&&) = delete;

 private:
  ScopeId scope_id_;
  std::chrono::steady_clock::time_point start_;
};

void PrintReport();

}  // namespace profkit

#define PROFKIT_CONCAT_IMPL(a, b) a##b
#define PROFKIT_CONCAT(a, b) PROFKIT_CONCAT_IMPL(a, b)

#if defined(PROFKIT_ENABLE) && PROFKIT_ENABLE

#define PROF_SCOPE(name)                                                        \
  static const ::profkit::ScopeId PROFKIT_CONCAT(profkit_scope_id_, __LINE__) = \
      ::profkit::RegisterScope(name);                                           \
  ::profkit::ScopeTimer PROFKIT_CONCAT(profkit_timer_,                          \
                                       __LINE__)(PROFKIT_CONCAT(profkit_scope_id_, __LINE__))

#define PROF_FUNCTION() PROF_SCOPE(__PRETTY_FUNCTION__)

#define PROF_DUMP() ::profkit::PrintReport();

#else

#define PROF_SCOPE(name) static_cast<void>(0)
#define PROF_FUNCTION() static_cast<void>(0)

#endif