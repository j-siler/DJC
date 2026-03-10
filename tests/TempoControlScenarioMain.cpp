#include "djcore/DeckTransitions.hpp"
#include "djcore/SimulationState.hpp"
#include "djcore/TempoMath.hpp"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void fail_if(bool cond, const std::string& msg) {
    if (cond) {
        throw std::runtime_error(msg);
    }
}

bool nearly_equal(double a, double b, double eps = 1e-12) {
    return std::abs(a - b) <= eps;
}

} // namespace

int main() {
    try {
        {
            djcore::TempoState tempo;
            tempo.slider_percent = 100.0;
            tempo.range_percent = 6.0;
            djcore::recompute_user_rate_from_slider_and_range(tempo);

            fail_if(!nearly_equal(
                        djcore::slider_position_to_tempo_offset_percent(
                            tempo.slider_percent,
                            tempo.range_percent),
                        6.0),
                    "Expected +6% offset at full throw with ±6% range");

            fail_if(!nearly_equal(tempo.user_rate.scalar, 1.06),
                    "Expected user_rate 1.06");
        }

        {
            djcore::TempoState tempo;
            tempo.slider_percent = -50.0;
            tempo.range_percent = 16.0;
            djcore::recompute_user_rate_from_slider_and_range(tempo);

            fail_if(!nearly_equal(
                        djcore::slider_position_to_tempo_offset_percent(
                            tempo.slider_percent,
                            tempo.range_percent),
                        -8.0),
                    "Expected -8% offset at half throw with ±16% range");

            fail_if(!nearly_equal(tempo.user_rate.scalar, 0.92),
                    "Expected user_rate 0.92");
        }

        {
            djcore::TempoState tempo;
            tempo.slider_percent = 25.0;
            tempo.range_percent = 10.0;
            djcore::recompute_user_rate_from_slider_and_range(tempo);

            fail_if(!nearly_equal(
                        djcore::slider_position_to_tempo_offset_percent(
                            tempo.slider_percent,
                            tempo.range_percent),
                        2.5),
                    "Expected +2.5% offset at quarter throw with ±10% range");

            fail_if(!nearly_equal(tempo.user_rate.scalar, 1.025),
                    "Expected user_rate 1.025");
        }

        {
            djcore::SimulationState state = djcore::make_default_simulation_state();
            djcore::Deck& deck = state.deck_a;
            const djcore::Track& track = state.library.front();

            djcore::load_track(deck, track);

            deck.p_track.seconds = 0.0;
            deck.tempo.slider_percent = 66.6666666667;
            deck.tempo.range_percent = 10.0;
            djcore::recompute_user_rate_from_slider_and_range(deck.tempo);

            const double native_bpm =
                track.beat_time_map.instantaneous_bpm(deck.p_track.seconds);
            const double effective_bpm =
                djcore::effective_bpm_at_playhead(deck, track);

            fail_if(!nearly_equal(native_bpm, 120.0, 1e-9),
                    "Expected native BPM 120 at nominal start for sample track");

            fail_if(!nearly_equal(deck.tempo.user_rate.scalar, 1.066666666667, 1e-9),
                    "Expected user_rate about 1.066666666667");

            fail_if(!nearly_equal(effective_bpm, 128.0, 1e-8),
                    "Expected effective BPM 128.0");
        }

        std::cout << "TempoControlScenario PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "TempoControlScenario FAIL\n";
        std::cout << e.what() << "\n";
        return 1;
    }
}
