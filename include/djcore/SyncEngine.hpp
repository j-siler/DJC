    // SyncEngine.hpp
    // DJCoreTimingHarness
    //
    // Minimal phase sync controller for testing (not production-ready).
    //
    // SPDX-License-Identifier: MIT

    #pragma once

    #include <cmath>
    #include <algorithm>
    #include "DeckTimingEngine.hpp"

    namespace djcore {

    struct SyncControllerConfig final {
        double kp {0.20};
        double ki {0.00};
        double max_correction {0.02}; // +/- 2%
    };

    struct SyncControllerState final {
        double integral {0.0};
        void reset() { integral = 0.0; }
    };

    class SyncEngine final {
    public:
        explicit SyncEngine(SyncControllerConfig cfg = {}) : m_cfg(cfg) {}

        double compute_rate_correction(const BeatTimeMap& map_master,
                                       const DeckTimingState& master,
                                       const BeatTimeMap& map_follower,
                                       const DeckTimingState& follower,
                                       SyncControllerState& controller_state,
                                       double dt_seconds) const;

        static double wrap_to_half(double x);

    private:
        SyncControllerConfig m_cfg;
    };

    } // namespace djcore
