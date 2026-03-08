// DjCoreDomainTypes.hpp
// DJ Core Domain Model
//
// First-pass domain data structures for the timing core.
//
// Design intent:
// - Track owns descriptive/static timing data and related metadata.
// - Deck owns dynamic playback state and control/policy state.
// - Beat/phase/BPM/bar and similar musical quantities remain derived queries.
// - Hot cues are represented as a subset/presentation of cue points, not as a
//   second authoritative collection.
//
// This header is intentionally light on behavior. Its main purpose is to make
// ownership explicit before transport/control semantics are encoded.
//
// SPDX-License-Identifier: MIT

#pragma once

#include "BeatTimeMap.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace djcore {

// -----------------------------------------------------------------------------
// Small scalar wrappers
// -----------------------------------------------------------------------------

struct TrackTime final {
    double seconds {0.0};
};

struct BeatPosition final {
    double beats {0.0};
};

struct PlaybackRate final {
    double scalar {1.0};
};

// -----------------------------------------------------------------------------
// IDs / handles
// -----------------------------------------------------------------------------

using TrackId = std::string;
using CueId   = std::string;
using LoopId  = std::string;
using DeckId  = std::string;

// Deck owns the fact that a Track is loaded, not Track content itself.
struct LoadedTrackRef final {
    TrackId track_id;
};

// -----------------------------------------------------------------------------
// Enums
// -----------------------------------------------------------------------------

enum class TransportState {
    Paused,
    Playing
};

enum class SyncMode {
    Off,
    BeatSync
};

enum class JogInteractionMode {
    Nudge,
    Search,
    PausedFrameMove,
    Scratch
};

enum class CuePointRole : std::uint32_t {
    None          = 0,
    MainCue       = 1u << 0,
    HotCue        = 1u << 1,
    MemoryPoint   = 1u << 2,
    LoadCue       = 1u << 3,
    LoopInAnchor  = 1u << 4,
    LoopOutAnchor = 1u << 5,
    GenericMarker = 1u << 6
};

constexpr CuePointRole operator|(CuePointRole a, CuePointRole b) noexcept {
    using U = std::underlying_type_t<CuePointRole>;
    return static_cast<CuePointRole>(static_cast<U>(a) | static_cast<U>(b));
}

constexpr CuePointRole operator&(CuePointRole a, CuePointRole b) noexcept {
    using U = std::underlying_type_t<CuePointRole>;
    return static_cast<CuePointRole>(static_cast<U>(a) & static_cast<U>(b));
}

constexpr CuePointRole& operator|=(CuePointRole& a, CuePointRole b) noexcept {
    a = a | b;
    return a;
}

constexpr bool has_role(CuePointRole mask, CuePointRole role) noexcept {
    using U = std::underlying_type_t<CuePointRole>;
    return (static_cast<U>(mask) & static_cast<U>(role)) != 0;
}

// -----------------------------------------------------------------------------
// Track-owned descriptive data
// -----------------------------------------------------------------------------

struct HotCueBinding final {
    std::uint32_t slot {0};
};

struct CuePoint final {
    CueId id;
    std::string label;

    // Track-owned descriptive position.
    TrackTime track_time;

    // Optional convenience hint. Authoritative mapping still lives in BeatTimeMap.
    std::optional<BeatPosition> beat_hint;

    // A cue may play multiple roles simultaneously; this avoids duplicate truth.
    CuePointRole roles {CuePointRole::GenericMarker};

    // Present only when this cue is exposed through the hot-cue bank.
    std::optional<HotCueBinding> hot_cue;

    // Room for later UI/state decoration without splitting the model.
    std::optional<std::string> color;
};

struct SavedLoop final {
    LoopId id;
    std::string label;

    // Track-owned descriptive endpoints.
    TrackTime in_track_time;
    TrackTime out_track_time;

    // Optional hints only.
    std::optional<BeatPosition> in_beat_hint;
    std::optional<BeatPosition> out_beat_hint;

    bool locked {false};
};

struct TrackMetadata final {
    std::string title;
    std::string artist;
    std::string album;
};

struct TrackAnalysisInfo final {
    std::optional<std::string> nominal_key;
    std::string analysis_version;
    std::uint64_t grid_edit_revision {0};
};

struct Track final {
    // Authoritative descriptive identity / media linkage.
    TrackId track_id;
    std::string audio_source_ref;

    // Authoritative descriptive timing data.
    TrackTime track_origin_T0;
    TrackTime duration_nominal;
    BeatTimeMap beat_time_map;

    // Track-owned descriptive musical/navigation data.
    std::vector<CuePoint> cue_points;
    std::vector<SavedLoop> saved_loops;

    // Non-timing descriptive data.
    TrackMetadata metadata;
    TrackAnalysisInfo analysis;
};

// -----------------------------------------------------------------------------
// Deck-owned dynamic playback / control state
// -----------------------------------------------------------------------------

struct QuantizeState final {
    bool enabled {false};
    BeatPosition resolution {1.0};
};

struct TempoState final {
    // UI-facing values.
    double slider_percent {0.0};
    double range_percent {6.0};

    // Timing-facing rate factors. These line up with the current harness split.
    PlaybackRate user_rate {1.0};
    PlaybackRate nudge_rate {1.0};
    PlaybackRate sync_rate_correction {1.0};

    double effective_scalar() const noexcept {
        return user_rate.scalar * nudge_rate.scalar * sync_rate_correction.scalar;
    }
};

struct CueState final {
    // Needed to model normal Cue semantics cleanly.
    std::optional<TrackTime> main_cue_track_time;

    // Explicitly identified in the property inventory.
    bool preview_active {false};
};

struct LoopState final {
    // Deck owns the fact that a loop is active now.
    bool active {false};

    // If a saved Track loop is active, this refers to it.
    std::optional<LoopId> active_loop_id;

    // Manual/current loop endpoints on the deck.
    std::optional<TrackTime> in_track_time;
    std::optional<TrackTime> out_track_time;
};

struct SyncState final {
    SyncMode mode {SyncMode::Off};
    bool master_flag {false};

    // Control/policy state. Audible implementation belongs to audio/rendering later.
    bool key_sync_enabled {false};
};

struct JogState final {
    JogInteractionMode interaction_mode {JogInteractionMode::Nudge};
    bool vinyl_mode_enabled {false};
    bool needle_search_enabled {true};
};

struct BeatJumpState final {
    BeatPosition increment {4.0};
};

struct Deck final {
    DeckId deck_id;

    // Deck-owned loaded-track fact.
    std::optional<LoadedTrackRef> loaded_track;

    // Authoritative dynamic playback coordinate in track time.
    TrackTime p_track;

    // High-level transport state. Pause is distinct from zero playback rate.
    TransportState transport_state {TransportState::Paused};

    // Scheduled repositioning applied at the next safe update boundary.
    std::optional<TrackTime> pending_seek_track_time;

    // Control/policy state.
    bool slip_enabled {false};
    QuantizeState quantize;
    TempoState tempo;
    CueState cue;
    LoopState loop;
    SyncState sync;
    JogState jog;
    BeatJumpState beat_jump;

    // Presentation / control-surface configuration.
    std::uint32_t hot_cue_bank_size {8};

    // Authoritative playback rate lives here as the product of deck-owned timing
    // rate components, without storing a second copy elsewhere.
    double effective_playback_rate_scalar() const noexcept {
        return tempo.effective_scalar();
    }
};

// -----------------------------------------------------------------------------
// Intentionally derived, not stored
// -----------------------------------------------------------------------------
// These values should be queried from Deck + Track rather than cached as
// authoritative object state unless a real performance need is proven:
//
// - current beat position
// - instantaneous BPM
// - current bar number
// - phase
// - beat-in-bar
// - time until next beat
// - time since last beat
// - active loop length in beats
// - active loop length in track time
// - sync target beat position

} // namespace djcore
