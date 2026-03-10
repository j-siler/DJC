cmake_minimum_required(VERSION 3.20)
project(DJCoreTimingHarness LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_library(djcore_timing
    src/BeatTimeMap.cpp
    src/DeckTimingEngine.cpp
    src/SyncEngine.cpp
    src/DebugState.cpp
    src/DeckTransitions.cpp
    src/SimulationState.cpp
    src/SimulationAdvance.cpp
    src/TempoMath.cpp
    src/SampleTracks.cpp
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

add_executable(load_track_scenario
    tests/LoadTrackScenarioMain.cpp
)

target_link_libraries(load_track_scenario PRIVATE djcore_timing)

add_executable(tempo_control_scenario
    tests/TempoControlScenarioMain.cpp
)

target_link_libraries(tempo_control_scenario PRIVATE djcore_timing)

add_executable(djcore_qt_harness
    src/qt/Main.cpp
    src/qt/MainWindow.cpp
    src/qt/DeckPanel.cpp
    src/qt/HistoryLineEdit.cpp
)

target_link_libraries(djcore_qt_harness
    PRIVATE
        djcore_timing
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
