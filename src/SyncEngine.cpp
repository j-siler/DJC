    // SyncEngine.cpp
    // DJCoreTimingHarness
    //
    // SPDX-License-Identifier: MIT

    #include "djcore/SyncEngine.hpp"

    namespace djcore {

    double SyncEngine::wrap_to_half(double x) {
        x = std::fmod(x, 1.0);
        if (x < -0.5) x += 1.0;
        if (x >= 0.5) x -= 1.0;
        return x;
    }

    double SyncEngine::compute_rate_correction(const BeatTimeMap& map_master,
                                               const DeckTimingState& master,
                                               const BeatTimeMap& map_follower,
                                               const DeckTimingState& follower,
                                               SyncControllerState& controller_state,
                                               double dt_seconds) const {
        (void)map_follower;

        const double bM = master.beat_index(map_master);
        const double bF = follower.beat_index(map_master); // same map assumption for harness

        const double phaseM = bM - std::floor(bM);
        const double phaseF = bF - std::floor(bF);

        const double phase_err = wrap_to_half(phaseM - phaseF);

        controller_state.integral += phase_err * dt_seconds;

        double corr = 1.0 + (m_cfg.kp * phase_err) + (m_cfg.ki * controller_state.integral);

        const double lo = 1.0 - m_cfg.max_correction;
        const double hi = 1.0 + m_cfg.max_correction;
        corr = std::clamp(corr, lo, hi);

        return corr;
    }

    } // namespace djcore
