// DomainTypes.hpp
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
// - Nominal track start is distinct from the musical anchor used for derived
//   bar/beat display labeling.
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
//
// These wrappers exist mainly to make intent clearer in signatures and fields.
// They are still just doubles underneath for now.

struct TrackTime final {
    // Signed time in track coordinates.
    //
    // Important:
    // - This is not "raw file time" unless the file was authored that way.
    // - This may be negative.
    // - T = 0 is not a hard boundary; it is only a named origin if the Track
    //   chooses to use it that way.
    double seconds {0.0};
};

struct BeatPosition final {
    // Signed continuous beat coordinate.
    //
    // Examples:
    // - 0.0 may mean the beat chosen as internal origin.
    // - 3.5 means halfway between beats 3 and 4 in the internal coordinate
    //   system.
    // - Negative values are allowed.
    //
    // This is not the same thing as displayed measure:beat notation.
    double beats {0.0};
};

struct PlaybackRate final {
    // Multiplicative playback-rate scalar.
    //
    // Examples:
    // - 1.0  = nominal forward speed
    // - 0.5  = half speed
    // - 1.06 = +6%
    //
    // Signed reverse playback is not ruled out by this type, though the rest
    // of the system does not yet define full reverse semantics.
    double scalar {1.0};
};

// -----------------------------------------------------------------------------
// IDs / handles
// -----------------------------------------------------------------------------

using TrackId = std::string;
using CueId   = std::string;
using LoopId  = std::string;
using DeckId  = std::string;

// Deck owns the fact that a Track is loaded, but not the Track object itself.
struct LoadedTrackRef final {
    // Stable identifier of the Track currently loaded into the Deck.
    TrackId track_id;
};

// -----------------------------------------------------------------------------
// Enums
// -----------------------------------------------------------------------------

enum class TransportState {
    // Playback is not advancing because the transport is stopped/paused.
    //
    // Important:
    // - This is semantically different from "playing at rate 0".
    // - Pause is transport state, not merely a numeric rate value.
    Paused,

    // Playback is actively advancing according to the deck's effective rate.
    Playing
};

enum class SyncMode {
    // No sync behavior is active.
    Off,

    // Deck participates in beat sync.
    //
    // For now this is intentionally coarse; later this may split into more
    // precise policies such as phase-only, tempo+phase, soft/hard sync, etc.
    BeatSync
};

enum class JogInteractionMode {
    // Jog modifies playback slightly while playing.
    Nudge,

    // Jog acts as a search/seek control.
    Search,

    // Jog moves position while transport is paused.
    PausedFrameMove,

    // Future scratch/vinyl mode behavior.
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
    // Which performance-bank slot exposes this cue as a hot cue.
    //
    // This is presentation/interaction metadata attached to a cue point, not a
    // second cue object.
    std::uint32_t slot {0};
};

struct CuePoint final {
    // Stable identity for this stored cue-like marker.
    CueId id;

    // Human-readable label.
    std::string label;

    // Authoritative stored position of this cue/marker in track coordinates.
    //
    // This is the main fact about the cue.
    TrackTime track_time;

    // Optional cached hint about beat location.
    //
    // Important:
    // - This is not authoritative.
    // - The true beat<->time relationship belongs to BeatTimeMap.
    // - This may be useful for UI or import/export, but should not replace
    //   querying the map.
    std::optional<BeatPosition> beat_hint;

    // Bitmask of roles this cue plays.
    //
    // A cue may have multiple roles at once.
    // Example: it might be both a HotCue and a MemoryPoint.
    CuePointRole roles {CuePointRole::GenericMarker};

    // Present only if this cue is exposed as a hot cue.
    std::optional<HotCueBinding> hot_cue;

    // Optional display metadata for UI.
    std::optional<std::string> color;
};

struct SavedLoop final {
    // Stable identity of this stored loop.
    LoopId id;

    // Human-readable label.
    std::string label;

    // Stored descriptive loop in-point in track coordinates.
    TrackTime in_track_time;

    // Stored descriptive loop out-point in track coordinates.
    TrackTime out_track_time;

    // Optional cached beat hints.
    //
    // These do not replace BeatTimeMap as the authoritative mapping.
    std::optional<BeatPosition> in_beat_hint;
    std::optional<BeatPosition> out_beat_hint;

    // If true, UI/policy may prevent casual editing or overwriting.
    bool locked {false};
};

struct TrackMetadata final {
    // Human-facing media metadata.
    std::string title;
    std::string artist;
    std::string album;
};

struct TrackAnalysisInfo final {
    // Optional analyzed or assigned musical key.
    std::optional<std::string> nominal_key;

    // Version tag for analysis pipeline/provenance.
    std::string analysis_version;

    // Revision counter for beat-grid or timing-map edits.
    std::uint64_t grid_edit_revision {0};
};

struct MusicalDisplayAnchor final {
    // Internal beat coordinate that should be labeled as the display anchor.
    //
    // Example:
    // if anchor_beat = 32.0 and display_measure:display_beat = 1:1,
    // then internal beat 32.0 is shown to the user as 1:1.
    BeatPosition anchor_beat {0.0};

    // Meter assumption for bar/beat display.
    //
    // This is a display convention for now, not a full meter map.
    std::int32_t beats_per_bar {4};

    // Display label assigned to anchor_beat.
    //
    // Normally this will be 1.
    std::int64_t display_measure {1};

    // Display beat number within the bar assigned to anchor_beat.
    //
    // Normally this will be 1.
    std::int32_t display_beat_in_measure {1};
};

struct Track final {
    // Stable identity of the Track within library/session/application context.
    TrackId track_id;

    // Reference to the underlying audio asset.
    //
    // This might be a file path, URI, database key, content hash, etc.
    std::string audio_source_ref;

    // Nominal track start in track coordinates.
    //
    // Important:
    // - This is a convenience origin/default load point.
    // - It does NOT imply that no audio exists before it.
    // - It does NOT forbid cues before it.
    TrackTime nominal_start_T0;

    // Nominal extent of the track in track coordinates.
    //
    // This is descriptive metadata, not a hard semantic barrier.
    TrackTime duration_nominal;

    // Authoritative beat <-> track-time mapping.
    //
    // This is where tempo changes / warp markers live conceptually.
    BeatTimeMap beat_time_map;

    // Display-only anchor for deriving human-friendly measure:beat labels.
    //
    // This does not affect the underlying signed linear beat/time system.
    MusicalDisplayAnchor musical_display_anchor;

    // Stored cue-like objects owned by the Track.
    //
    // This includes ordinary cue points, hot cues, memory points, load cues,
    // and similar markers via roles/flags.
    std::vector<CuePoint> cue_points;

    // Stored loops owned by the Track.
    std::vector<SavedLoop> saved_loops;

    // Human-facing metadata.
    TrackMetadata metadata;

    // Analysis-related metadata.
    TrackAnalysisInfo analysis;
};

// -----------------------------------------------------------------------------
// Deck-owned dynamic playback / control state
// -----------------------------------------------------------------------------

struct QuantizeState final {
    // Whether quantized operations are enabled on this Deck.
    //
    // This affects actions such as beat jump, cue recall, or loop operations,
    // depending on policy chosen later.
    bool enabled {false};

    // Grid resolution to which quantized actions should snap.
    //
    // Example values:
    // - 1.0   = one beat
    // - 0.5   = half-beat
    // - 4.0   = one bar in 4/4
    BeatPosition resolution {1.0};
};

struct TempoState final {
    // UI-facing slider position, usually thought of as pitch/tempo fader
    // percentage relative to nominal.
    //
    // This is not itself the final playback scalar; it is controller state.
    double slider_percent {0.0};

    // Active pitch/tempo range selection in percent.
    //
    // Examples: 6, 10, 16, 50, 100
    double range_percent {6.0};

    // User-requested base playback rate derived from slider and range.
    //
    // This is the persistent deck tempo setting, before nudge or sync
    // correction.
    PlaybackRate user_rate {1.0};

    // Temporary rate adjustment from jog bend/nudge behavior.
    //
    // This is generally transient.
    PlaybackRate nudge_rate {1.0};

    // Additional correction factor applied by sync controller logic.
    //
    // This separates user intent from sync intervention.
    PlaybackRate sync_rate_correction {1.0};

    // Combined effective playback scalar currently requested by deck timing.
    //
    // This is derived from the three factors above and is not separately stored.
    double effective_scalar() const noexcept {
        return user_rate.scalar * nudge_rate.scalar * sync_rate_correction.scalar;
    }
};

struct CueState final {
    // Main cue position currently stored for this Deck in track coordinates.
    //
    // This is deck state, not track metadata, because "current cue" is part of
    // live playback/control context.
    std::optional<TrackTime> main_cue_track_time;

    // Whether the deck is in a cue-preview/held-cue state.
    //
    // The exact semantics still need to be defined in transition code.
    bool preview_active {false};
};

struct LoopState final {
    // Whether a loop is currently active in live deck playback.
    //
    // This is deck state because it changes during performance.
    bool active {false};

    // If the currently active loop came from a Track-owned saved loop, this
    // identifies which one.
    std::optional<LoopId> active_loop_id;

    // Live/manual loop in-point currently active or being edited on the deck.
    std::optional<TrackTime> in_track_time;

    // Live/manual loop out-point currently active or being edited on the deck.
    std::optional<TrackTime> out_track_time;
};

struct SyncState final {
    // Whether sync is active and in what broad mode.
    SyncMode mode {SyncMode::Off};

    // Whether this deck is currently marked as master.
    //
    // This is control state, not a guarantee about any wider arbitration policy.
    bool master_flag {false};

    // Whether key-sync behavior is requested for this deck.
    //
    // This is stored here as deck control state even though the audible
    // implementation belongs to audio/rendering.
    bool key_sync_enabled {false};
};

struct JogState final {
    // Current interpretation of jog-wheel movement.
    JogInteractionMode interaction_mode {JogInteractionMode::Nudge};

    // Whether vinyl-style interaction mode is enabled.
    //
    // Exact semantics remain to be defined later.
    bool vinyl_mode_enabled {false};

    // Whether direct needle-search style repositioning is allowed/armed.
    bool needle_search_enabled {true};
};

struct BeatJumpState final {
    // The currently selected beat-jump increment.
    //
    // Example: 4.0 means jump by four beats when the user presses jump forward
    // or backward.
    BeatPosition increment {4.0};
};

struct Deck final {
    // Stable identity of this Deck.
    DeckId deck_id;

    // Which Track is currently loaded on this Deck, if any.
    //
    // This is the deck-owned fact of loading, not ownership of Track content.
    std::optional<LoadedTrackRef> loaded_track;

    // Authoritative current playhead position in track coordinates.
    //
    // This is one of the most important deck state variables.
    // It may be negative.
    TrackTime p_track;

    // High-level transport mode.
    //
    // This answers "is the deck playing or paused?" independently of numeric
    // playback rate.
    TransportState transport_state {TransportState::Paused};

    // Requested reposition to be applied at the next safe timing boundary.
    //
    // This is useful when a control action wants to move the playhead but the
    // actual deck engine applies such moves in a controlled/update-safe place.
    std::optional<TrackTime> pending_seek_track_time;

    // Whether slip mode is enabled.
    //
    // Slip semantics are not yet fully modeled, but the deck needs to know
    // whether it is armed.
    bool slip_enabled {false};

    // Quantize policy/settings currently active on this Deck.
    QuantizeState quantize;

    // Tempo/pitch-related live control state.
    TempoState tempo;

    // Cue-related live state.
    CueState cue;

    // Loop-related live state.
    LoopState loop;

    // Sync/master/key-sync related live state.
    SyncState sync;

    // Jog/needle/vinyl interaction policy state.
    JogState jog;

    // Current beat-jump size selection.
    BeatJumpState beat_jump;

    // Number of hot-cue slots the deck/control surface expects to expose.
    //
    // This is configuration/presentation state, not track content.
    std::uint32_t hot_cue_bank_size {8};

    // Effective playback rate requested by this deck right now.
    //
    // Derived from tempo state rather than stored independently.
    double effective_playback_rate_scalar() const noexcept {
        return tempo.effective_scalar();
    }
};

// -----------------------------------------------------------------------------
// Intentionally derived, not stored
// -----------------------------------------------------------------------------
//
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
// - displayed measure:beat for current position

} // namespace djcore
