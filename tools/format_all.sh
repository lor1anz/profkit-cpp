#!/usr/bin/env bash
set -e

find src include examples benchmark \
  \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" -o -name "*.cxx" \) \
  -exec clang-format -i {} +