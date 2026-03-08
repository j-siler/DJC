    // BeatTimeMap.cpp
    // DJCoreTimingHarness
    //
    // SPDX-License-Identifier: MIT

    #include "djcore/BeatTimeMap.hpp"

    #include <algorithm>
    #include <cmath>

    namespace djcore {

    BeatTimeMap::BeatTimeMap(std::vector<WarpMarker> markers)
        : m_markers(std::move(markers)) {
        validate_and_build();
    }

    void BeatTimeMap::set_markers(std::vector<WarpMarker> markers) {
        m_markers = std::move(markers);
        validate_and_build();
    }

    void BeatTimeMap::validate_and_build() {
        if (m_markers.size() < 2) {
            throw std::invalid_argument("BeatTimeMap requires at least 2 warp markers.");
        }

        for (std::size_t i = 1; i < m_markers.size(); ++i) {
            const auto& prev = m_markers[i - 1];
            const auto& cur  = m_markers[i];
            if (!(cur.time_seconds > prev.time_seconds)) {
                throw std::invalid_argument("Warp markers must have strictly increasing time_seconds.");
            }
            if (!(cur.beat_index > prev.beat_index)) {
                throw std::invalid_argument("Warp markers must have strictly increasing beat_index.");
            }
        }

        m_segments.clear();
        m_segments.reserve(m_markers.size() - 1);

        for (std::size_t i = 0; i + 1 < m_markers.size(); ++i) {
            const auto& a = m_markers[i];
            const auto& b = m_markers[i + 1];

            const double dt = b.time_seconds - a.time_seconds;
            const double db = b.beat_index   - a.beat_index;

            if (dt <= 0.0 || db <= 0.0) {
                throw std::invalid_argument("Warp marker segment must have positive dt and db.");
            }

            Segment s;
            s.t0 = a.time_seconds; s.t1 = b.time_seconds;
            s.b0 = a.beat_index;   s.b1 = b.beat_index;
            s.db_dt = db / dt;
            s.dt_db = dt / db;

            if (!std::isfinite(s.db_dt) || !std::isfinite(s.dt_db)) {
                throw std::invalid_argument("Non-finite slope in warp segment.");
            }

            m_segments.push_back(s);
        }
    }

    std::size_t BeatTimeMap::segment_for_time(double t_seconds) const {
        if (t_seconds <= m_segments.front().t0) return 0;
        if (t_seconds >= m_segments.back().t1) return m_segments.size() - 1;

        const auto it = std::upper_bound(
            m_segments.begin(), m_segments.end(), t_seconds,
            [](double t, const Segment& s) { return t < s.t1; }
        );
        const std::size_t idx = static_cast<std::size_t>(std::distance(m_segments.begin(), it));
        return (idx >= m_segments.size()) ? (m_segments.size() - 1) : idx;
    }

    std::size_t BeatTimeMap::segment_for_beat(double beat_index) const {
        if (beat_index <= m_segments.front().b0) return 0;
        if (beat_index >= m_segments.back().b1) return m_segments.size() - 1;

        const auto it = std::upper_bound(
            m_segments.begin(), m_segments.end(), beat_index,
            [](double b, const Segment& s) { return b < s.b1; }
        );
        const std::size_t idx = static_cast<std::size_t>(std::distance(m_segments.begin(), it));
        return (idx >= m_segments.size()) ? (m_segments.size() - 1) : idx;
    }

    double BeatTimeMap::time_to_beat(double t_seconds) const {
        const std::size_t idx = segment_for_time(t_seconds);
        const Segment& s = m_segments[idx];

        if (t_seconds <= m_markers.front().time_seconds) {
            return m_markers.front().beat_index;
        }

        return s.b0 + (t_seconds - s.t0) * s.db_dt;
    }

    double BeatTimeMap::beat_to_time(double beat_index) const {
        const std::size_t idx = segment_for_beat(beat_index);
        const Segment& s = m_segments[idx];

        if (beat_index <= m_markers.front().beat_index) {
            return m_markers.front().time_seconds;
        }

        return s.t0 + (beat_index - s.b0) * s.dt_db;
    }

    double BeatTimeMap::instantaneous_bpm(double t_seconds) const {
        const std::size_t idx = segment_for_time(t_seconds);
        const Segment& s = m_segments[idx];
        return 60.0 * s.db_dt;
    }

    } // namespace djcore
