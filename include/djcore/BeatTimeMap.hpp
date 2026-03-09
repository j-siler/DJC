// BeatTimeMap.hpp
// DJCoreTimingHarness
//
// A piecewise-linear mapping between audio time (seconds) and beat index (beats),
// defined by monotonic warp markers (time_seconds, beat_index).
//
// Semantics:
// - The mapping is linear and signed across its full domain.
// - Before the first marker and after the last marker, the mapping extrapolates
//   using the slope of the first and last segments respectively.
// - This allows valid beat/time queries for regions before nominal track start,
//   including negative track times and beat indices.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>
#include <stdexcept>
#include <cstddef>
#include "WarpMarker.hpp"

namespace djcore {

class BeatTimeMap final {
public:
    BeatTimeMap() = default;
    explicit BeatTimeMap(std::vector<WarpMarker> markers);

    void set_markers(std::vector<WarpMarker> markers);

    const std::vector<WarpMarker>& markers() const noexcept { return m_markers; }

    double time_to_beat(double t_seconds) const;
    double beat_to_time(double beat_index) const;
    double instantaneous_bpm(double t_seconds) const;

private:
    struct Segment final {
        double t0 {0.0}, t1 {0.0};
        double b0 {0.0}, b1 {0.0};
        double db_dt {0.0}; // beats/sec
        double dt_db {0.0}; // sec/beat
    };

    std::vector<WarpMarker> m_markers;
    std::vector<Segment> m_segments;

    void validate_and_build();
    std::size_t segment_for_time(double t_seconds) const;
    std::size_t segment_for_beat(double beat_index) const;
};

} // namespace djcore
