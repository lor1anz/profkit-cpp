cmake_minimum_required(VERSION 3.24)

project(profkit-cpp
    VERSION 0.1.0
    LANGUAGES CXX
)

option(PROFKIT_ENABLE "Enable profkit instrumentation" ON)
option(PROFKIT_BUILD_EXAMPLES "Build profkit examples" ON)

add_library(profkit
    src/profiler.cpp
)

add_library(profkit::profkit ALIAS profkit)

target_compile_features(profkit PUBLIC cxx_std_20)

target_include_directories(profkit PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(profkit PUBLIC
    $<$<BOOL:${PROFKIT_ENABLE}>:PROFKIT_ENABLE=1>
)

if(PROFKIT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()