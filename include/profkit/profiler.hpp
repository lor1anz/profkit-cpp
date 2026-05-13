#pragma once

#include <chrono>
#include <string_view>

namespace profkit {

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

void PrintReport();

}  // namespace profkit

#define PROFKIT_CONCAT_IMPL(a, b) a##b
#define PROFKIT_CONCAT(a, b) PROFKIT_CONCAT_IMPL(a, b)

#define PROF_DUMP() ::profkit::PrintReport()

#if defined(__GNUC__) || defined(__clang__)
#define PROFKIT_FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define PROFKIT_FUNCTION_NAME __FUNCSIG__
#else
#define PROFKIT_FUNCTION_NAME __func__
#endif

#if defined(PROFKIT_ENABLE) && PROFKIT_ENABLE

#define PROF_SCOPE(name)                                                       \
static const ::profkit::ScopeId PROFKIT_CONCAT(profkit_scope_id_, __LINE__) = \
::profkit::RegisterScope(name);                                          \
::profkit::ScopeTimer PROFKIT_CONCAT(profkit_timer_, __LINE__)(              \
PROFKIT_CONCAT(profkit_scope_id_, __LINE__))

#define PROF_FUNCTION() PROF_SCOPE(PROFKIT_FUNCTION_NAME)

#define PROF_SESSION(name) \
::profkit::Session PROFKIT_CONCAT(profkit_session_, __LINE__)(name)

#else

#define PROF_SCOPE(name) static_cast<void>(0)
#define PROF_FUNCTION() static_cast<void>(0)
#define PROF_SESSION(name) static_cast<void>(0)

#endif