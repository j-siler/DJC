    // WarpMarker.hpp
    // DJCoreTimingHarness
    //
    // Defines the basic warp marker used by BeatTimeMap:
    // a mapping between audio time (seconds) and musical beat index.
    //
    // SPDX-License-Identifier: MIT

    #pragma once

    namespace djcore {

    struct WarpMarker final {
        double time_seconds {0.0};  // Audio time (seconds)
        double beat_index   {0.0};  // Continuous beat index (beats)

        constexpr WarpMarker() = default;
        constexpr WarpMarker(double t, double b) : time_seconds(t), beat_index(b) {}
    };

    } // namespace djcore
