    // DeckTimingEngine.cpp
    // DJCoreTimingHarness
    //
    // SPDX-License-Identifier: MIT

    #include "djcore/DeckTimingEngine.hpp"

    namespace djcore {

    void DeckTimingEngine::apply_pending_seek_if_any(DeckTimingState& s) const {
        if (s.pending_seek_t.has_value()) {
            s.t_seconds = *s.pending_seek_t;
            s.pending_seek_t.reset();
        }
    }

    void DeckTimingEngine::on_audio_block(DeckTimingState& s, double block_dt_seconds) const {
        if (!s.playing) return;

        // Apply seek at block boundary (start of block).
        apply_pending_seek_if_any(s);

        // Advance to end-of-block position.
        const double r = s.rate * s.rate_nudge * s.r_sync_correction;
        s.t_seconds += block_dt_seconds * r;
    }

    double DeckTimingEngine::quantize_beat(double beat, double q_beats) {
        if (q_beats <= 0.0) return beat;
        return std::round(beat / q_beats) * q_beats;
    }

    void DeckTimingEngine::schedule_jump_to_beat(DeckTimingState& s, const BeatTimeMap& map, double target_beat) const {
        if (s.quantize) {
            target_beat = quantize_beat(target_beat, s.quantize_beats);
        }
        s.pending_seek_t = map.beat_to_time(target_beat);
    }

    } // namespace djcore
