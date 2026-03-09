#include "djcore/DeckTransitions.hpp"

namespace djcore {
namespace {

void clear_live_loop_state(Deck& deck) {
    deck.loop.active = false;
    deck.loop.active_loop_id.reset();
    deck.loop.in_track_time.reset();
    deck.loop.out_track_time.reset();
}

void clear_live_seek_and_preview_state(Deck& deck) {
    deck.pending_seek_track_time.reset();
    deck.cue.preview_active = false;
}

void neutralize_transient_rate_modifiers(Deck& deck) {
    deck.tempo.nudge_rate.scalar = 1.0;
    deck.tempo.sync_rate_correction.scalar = 1.0;
}

} // namespace

std::optional<TrackTime> find_cue_time_by_role(const Track& track,
                                               CuePointRole role) {
    for (const auto& cue : track.cue_points) {
        if (has_role(cue.roles, role)) {
            return cue.track_time;
        }
    }
    return std::nullopt;
}

TrackTime choose_initial_load_position(const Track& track) {
    if (const auto load_cue = find_cue_time_by_role(track, CuePointRole::LoadCue)) {
        return *load_cue;
    }

    return track.nominal_start_T0;
}

void load_track(Deck& deck, const Track& track) {
    const TrackTime initial_position = choose_initial_load_position(track);

    deck.loaded_track = LoadedTrackRef{track.track_id};

    // A fresh load starts paused.
    deck.transport_state = TransportState::Paused;

    // The deck's authoritative live position becomes the chosen initial load point.
    deck.p_track = initial_position;

    // A new load invalidates old discontinuity-sensitive live state.
    clear_live_seek_and_preview_state(deck);
    clear_live_loop_state(deck);
    neutralize_transient_rate_modifiers(deck);

    // First-pass policy:
    // after load, Cue should have an immediately meaningful stored position.
    deck.cue.main_cue_track_time = initial_position;
}

void unload_track(Deck& deck) {
    deck.loaded_track.reset();
    deck.transport_state = TransportState::Paused;
    deck.p_track = TrackTime{0.0};

    clear_live_seek_and_preview_state(deck);
    clear_live_loop_state(deck);
    neutralize_transient_rate_modifiers(deck);

    deck.cue.main_cue_track_time.reset();
}

bool play(Deck& deck) {
    if (!deck.loaded_track.has_value()) {
        return false;
    }

    if (deck.transport_state == TransportState::Playing) {
        return false;
    }

    deck.cue.preview_active = false;
    deck.transport_state = TransportState::Playing;
    return true;
}

bool pause(Deck& deck) {
    if (!deck.loaded_track.has_value()) {
        return false;
    }

    if (deck.transport_state == TransportState::Paused) {
        return false;
    }

    deck.cue.preview_active = false;
    deck.transport_state = TransportState::Paused;
    return true;
}

} // namespace djcore
