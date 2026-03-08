cmake_minimum_required(VERSION 3.20)
project(DJCoreTimingHarness LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(djcore_timing
    src/BeatTimeMap.cpp
    src/DeckTimingEngine.cpp
    src/SyncEngine.cpp
)

target_include_directories(djcore_timing
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (MSVC)
    target_compile_options(djcore_timing PRIVATE /W4)
else()
    target_compile_options(djcore_timing PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(timing_harness
    tests/TimingHarnessMain.cpp
)

target_link_libraries(timing_harness PRIVATE djcore_timing)
