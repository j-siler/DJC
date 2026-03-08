    // DeckTimingEngine.hpp
    // DJCoreTimingHarness
    //
    // Deterministic deck timing engine:
    // - advances track time by audio block duration * effective rate
    // - applies seeks at block boundaries
    // - provides quantized beat jumps
    //
    // Notes on semantics:
    // - `apply_pending_seek_if_any()` updates the deck's current time to the seek target.
    //   This corresponds to the *start* of an audio block after a seek.
    // - `on_audio_block()` applies any pending seek first, then advances the deck time
    //   by the block duration. After `on_audio_block()`, `t_seconds` corresponds to the
    //   *end* of the processed block.
    //
    // SPDX-License-Identifier: MIT

    #pragma once

    #include <optional>
    #include <cmath>
    #include "BeatTimeMap.hpp"

    namespace djcore {

    struct DeckTimingState final {
        double t_seconds {0.0};
        bool   playing   {true};

        double rate              {1.0};
        double rate_nudge        {1.0};
        double r_sync_correction {1.0};

        bool   quantize       {false};
        double quantize_beats {1.0};

        std::optional<double> pending_seek_t;

        double beat_index(const BeatTimeMap& map) const { return map.time_to_beat(t_seconds); }
        double phase(const BeatTimeMap& map) const {
            const double b = beat_index(map);
            return b - std::floor(b);
        }
        double instantaneous_bpm(const BeatTimeMap& map) const { return map.instantaneous_bpm(t_seconds); }
    };

    class DeckTimingEngine final {
    public:
        // Apply pending seek only (no advancement).
        void apply_pending_seek_if_any(DeckTimingState& s) const;

        // Advance by one audio block (applies any pending seek first).
        void on_audio_block(DeckTimingState& s, double block_dt_seconds) const;

        static double quantize_beat(double beat, double q_beats);

        void schedule_jump_to_beat(DeckTimingState& s, const BeatTimeMap& map, double target_beat) const;

        static void schedule_jump_to_time(DeckTimingState& s, double target_time_seconds) {
            s.pending_seek_t = target_time_seconds;
        }
    };

    } // namespace djcore
