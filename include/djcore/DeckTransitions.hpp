#pragma once

#include "djcore/DomainTypes.hpp"

#include <optional>

namespace djcore {

// Returns the first cue time carrying the requested role, or null if none exists.
std::optional<TrackTime> find_cue_time_by_role(const Track& track,
                                               CuePointRole role);

// Policy used by the first-pass load transition:
// 1) first cue marked LoadCue, if present
// 2) otherwise Track.nominal_start_T0
TrackTime choose_initial_load_position(const Track& track);

// Load a Track onto a Deck.
//
// First-pass semantics:
// - loaded_track becomes track.track_id
// - transport becomes Paused
// - p_track becomes the chosen initial load position
// - pending seek is cleared
// - live loop state is cleared
// - cue preview is cleared
// - deck main cue is initialized to the chosen load position
// - transient rate modifiers are neutralized
//
// Deck-local control settings such as quantize, sync mode, master flag,
// beat-jump increment, and hot-cue bank size are preserved.
void load_track(Deck& deck, const Track& track);

// Remove any loaded Track from the Deck and clear live playback context.
void unload_track(Deck& deck);

// Basic transport toggles.
// These are intentionally simple and policy-light for now.
bool play(Deck& deck);
bool pause(Deck& deck);

} // namespace djcore
