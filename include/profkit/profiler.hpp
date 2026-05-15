#pragma once

#include <chrono>
#include <string_view>

namespace profkit {

enum class TimeUnit {
  kMilliseconds,
  kSeconds,
};

using ScopeId = std::uint32_t;

ScopeId RegisterScope(std::string_view name);

class ScopeTimer {
 public:
  explicit ScopeTimer(ScopeId scope_id);
  ~ScopeTimer();

  void Stop();

  ScopeTimer(const ScopeTimer&) = delete;
  ScopeTimer& operator=(const ScopeTimer&) = delete;

  ScopeTimer(ScopeTimer&&) = delete;
  ScopeTimer& operator=(ScopeTimer&&) = delete;

 private:
  ScopeId scope_id_;
  std::uint64_t start_ns_;
  bool stopped_ = false;
};

class Session {
 public:
  explicit Session(std::string_view name);
  ~Session();

  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;

  Session(Session&&) = delete;
  Session& operator=(Session&&) = delete;

 private:
  ScopeTimer timer_;
};

void SetTimeUnit(TimeUnit unit);
TimeUnit GetTimeUnit();

void PrintReport();
void Reset();
void DumpAndReset();

void PushScope(ScopeId id);
void PopScope();

}  // namespace profkit

#define PROFKIT_CONCAT_IMPL(a, b) a##b
#define PROFKIT_CONCAT(a, b) PROFKIT_CONCAT_IMPL(a, b)

#define PROF_DUMP() ::profkit::PrintReport()
#define PROF_RESET() ::profkit::Reset()
#define PROF_DUMP_AND_RESET() ::profkit::DumpAndReset()

#define PROF_USE_SECONDS() ::profkit::SetTimeUnit(::profkit::TimeUnit::kSeconds)
#define PROF_USE_MILLISECONDS() ::profkit::SetTimeUnit(::profkit::TimeUnit::kMilliseconds)

#if defined(__GNUC__) || defined(__clang__)
#define PROFKIT_FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define PROFKIT_FUNCTION_NAME __FUNCSIG__
#else
#define PROFKIT_FUNCTION_NAME __func__
#endif

#if defined(PROFKIT_ENABLE) && PROFKIT_ENABLE

#define PROF_SCOPE(name)                                                        \
  static const ::profkit::ScopeId PROFKIT_CONCAT(profkit_scope_id_, __LINE__) = \
      ::profkit::RegisterScope(name);                                           \
  ::profkit::ScopeTimer PROFKIT_CONCAT(profkit_timer_,                          \
                                       __LINE__)(PROFKIT_CONCAT(profkit_scope_id_, __LINE__))

#define PROF_FUNCTION() PROF_SCOPE(PROFKIT_FUNCTION_NAME)

#define PROF_SESSION(name) ::profkit::Session PROFKIT_CONCAT(profkit_session_, __LINE__)(name)

#define PROF_PUSH(name)                                                         \
  static const ::profkit::ScopeId PROFKIT_CONCAT(profkit_scope_id_, __LINE__) = \
      ::profkit::RegisterScope(name);                                           \
  ::profkit::PushScope(PROFKIT_CONCAT(profkit_scope_id_, __LINE__))

#define PROF_POP() ::profkit::PopScope()

#else

#define PROF_SCOPE(name) static_cast<void>(0)
#define PROF_FUNCTION() static_cast<void>(0)
#define PROF_SESSION(name) static_cast<void>(0)
#define PROF_PUSH(name) static_cast<void>(0)
#define PROF_POP(name) static_cast<void>(0)

#endif