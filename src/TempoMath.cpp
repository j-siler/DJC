#include "djcore/TempoMath.hpp"

#include <cmath>

namespace djcore {

double slider_position_to_tempo_offset_percent(double slider_percent,
                                               double range_percent) noexcept {
    return (slider_percent * range_percent) / 100.0;
}

double slider_position_to_user_rate_scalar(double slider_percent,
                                           double range_percent) noexcept {
    const double tempo_offset_percent =
        slider_position_to_tempo_offset_percent(slider_percent, range_percent);

    return 1.0 + (tempo_offset_percent / 100.0);
}

void recompute_user_rate_from_slider_and_range(TempoState& tempo) noexcept {
    tempo.user_rate.scalar =
        slider_position_to_user_rate_scalar(tempo.slider_percent,
                                            tempo.range_percent);
}

double effective_bpm_at_playhead(const Deck& deck, const Track& track) noexcept {
    const double native_bpm =
        track.beat_time_map.instantaneous_bpm(deck.p_track.seconds);

    const double effective_rate =
        std::abs(deck.effective_playback_rate_scalar());

    return native_bpm * effective_rate;
}

} // namespace djcore
