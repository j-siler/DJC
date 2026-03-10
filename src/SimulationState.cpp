#include "djcore/SimulationState.hpp"
#include "djcore/SampleTracks.hpp"

#include <iomanip>
#include <sstream>

namespace djcore {

std::string WallClockTime::to_string() const {
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << hour << ':'
        << std::setw(2) << minute << ':'
        << std::setw(2) << second << '.'
        << std::setw(3) << millisecond;
    return oss.str();
}

Deck* SimulationState::deck_by_id(std::string_view deck_id) {
    if (deck_id == "A" || deck_id == "a") {
        return &deck_a;
    }
    if (deck_id == "B" || deck_id == "b") {
        return &deck_b;
    }
    return nullptr;
}

const Deck* SimulationState::deck_by_id(std::string_view deck_id) const {
    if (deck_id == "A" || deck_id == "a") {
        return &deck_a;
    }
    if (deck_id == "B" || deck_id == "b") {
        return &deck_b;
    }
    return nullptr;
}

Track* SimulationState::track_by_id(std::string_view track_id) {
    for (auto& track : library) {
        if (track.track_id == track_id) {
            return &track;
        }
    }
    return nullptr;
}

const Track* SimulationState::track_by_id(std::string_view track_id) const {
    for (const auto& track : library) {
        if (track.track_id == track_id) {
            return &track;
        }
    }
    return nullptr;
}

SimulationState make_default_simulation_state() {
    SimulationState state;
    state.wall_clock = WallClockTime{21, 0, 0, 0};
    state.library = make_sample_tracks();

    state.deck_a.deck_id = "A";
    state.deck_b.deck_id = "B";

    state.deck_a.hot_cue_bank_size = 8;
    state.deck_b.hot_cue_bank_size = 8;

    return state;
}

} // namespace djcore
