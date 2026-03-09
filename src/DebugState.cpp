#include "djcore/DebugState.hpp"

#include <cmath>
#include <iomanip>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>

namespace djcore {
namespace {

template<typename T>
void append_optional_scalar(std::ostringstream& oss, const std::string& label, const std::optional<T>& value, int precision) {
  oss << "  " << label << ": ";
  if (value.has_value()) {
    if constexpr (std::is_same_v<T, TrackTime>) {
      oss << std::fixed << std::setprecision(precision) << value->seconds;
    } else if constexpr (std::is_same_v<T, BeatPosition>) {
      oss << std::fixed << std::setprecision(precision) << value->beats;
    } else {
      oss << *value;
    }
  } else {
    oss << "null";
  }
  oss << '\n';
}

std::string bool_string(bool v) { return v ? "true" : "false"; }

std::string transport_state_string(TransportState s) {
  switch (s) {
  case TransportState::Paused: return "Paused";
  case TransportState::Playing: return "Playing";
  }
  return "<unknown>";
}

std::string sync_mode_string(SyncMode s) {
  switch (s) {
  case SyncMode::Off: return "Off";
  case SyncMode::BeatSync: return "BeatSync";
  }
  return "<unknown>";
}

std::string jog_mode_string(JogInteractionMode m) {
  switch (m) {
  case JogInteractionMode::Nudge: return "Nudge";
  case JogInteractionMode::Search: return "Search";
  case JogInteractionMode::PausedFrameMove: return "PausedFrameMove";
  case JogInteractionMode::Scratch: return "Scratch";
  }
  return "<unknown>";
}

std::string cue_roles_string(CuePointRole roles) {
  if (roles == CuePointRole::None) {
    return "None";
  }

  std::ostringstream oss;
  bool               first = true;
  auto               add   = [&](CuePointRole role, const char* name) {
    if (has_role(roles, role)) {
      if (!first) {
        oss << '|';
      }
      oss << name;
      first = false;
    }
  };

  add(CuePointRole::MainCue, "MainCue");
  add(CuePointRole::HotCue, "HotCue");
  add(CuePointRole::MemoryPoint, "MemoryPoint");
  add(CuePointRole::LoadCue, "LoadCue");
  add(CuePointRole::LoopInAnchor, "LoopInAnchor");
  add(CuePointRole::LoopOutAnchor, "LoopOutAnchor");
  add(CuePointRole::GenericMarker, "GenericMarker");

  return oss.str();
}

void append_timing_harness_authoritative(std::ostringstream& oss, const DeckTimingState& deck, int precision) {
  oss << "DeckTimingState\n";
  oss << "  t_seconds: " << std::fixed << std::setprecision(precision) << deck.t_seconds << '\n';
  oss << "  playing: " << bool_string(deck.playing) << '\n';
  oss << "  rate: " << std::fixed << std::setprecision(precision) << deck.rate << '\n';
  oss << "  rate_nudge: " << std::fixed << std::setprecision(precision) << deck.rate_nudge << '\n';
  oss << "  r_sync_correction: " << std::fixed << std::setprecision(precision) << deck.r_sync_correction << '\n';
  oss << "  effective_rate: " << std::fixed << std::setprecision(precision)
      << (deck.rate * deck.rate_nudge * deck.r_sync_correction) << '\n';
  oss << "  quantize: " << bool_string(deck.quantize) << '\n';
  oss << "  quantize_beats: " << std::fixed << std::setprecision(precision) << deck.quantize_beats << '\n';
  oss << "  pending_seek_t: ";
  if (deck.pending_seek_t.has_value()) {
    oss << std::fixed << std::setprecision(precision) << *deck.pending_seek_t;
  } else {
    oss << "null";
  }
  oss << '\n';
}

void append_timing_harness_derived(std::ostringstream& oss, const DeckTimingState& deck, const BeatTimeMap& map, int precision) {
  const double beat  = deck.beat_index(map);
  const double phase = deck.phase(map);
  const double bpm   = deck.instantaneous_bpm(map);

  oss << "Derived\n";
  oss << "  beat_index: " << std::fixed << std::setprecision(precision) << beat << '\n';
  oss << "  phase: " << std::fixed << std::setprecision(precision) << phase << '\n';
  oss << "  instantaneous_bpm: " << std::fixed << std::setprecision(precision) << bpm << '\n';
}

void append_track_summary(std::ostringstream& oss, const Track& track, int precision) {
  oss << "Track\n";
  oss << "  track_id: " << track.track_id << '\n';
  oss << "  audio_source_ref: " << track.audio_source_ref << '\n';
  oss << "  nominal_start_T0: " << std::fixed << std::setprecision(precision) << track.nominal_start_T0.seconds << '\n';
  oss << "  duration_nominal: " << std::fixed << std::setprecision(precision) << track.duration_nominal.seconds << '\n';
  oss << "  musical_anchor.beat: " << std::fixed << std::setprecision(precision) << track.musical_display_anchor.anchor_beat.beats
      << '\n';
  oss << "  musical_anchor.beats_per_bar: " << track.musical_display_anchor.beats_per_bar << '\n';
  oss << "  musical_anchor.display_measure: " << track.musical_display_anchor.display_measure << '\n';
  oss << "  musical_anchor.display_beat_in_measure: " << track.musical_display_anchor.display_beat_in_measure << '\n';
  oss << "  cue_points.size: " << track.cue_points.size() << '\n';
  oss << "  saved_loops.size: " << track.saved_loops.size() << '\n';
  oss << "  metadata.title: " << track.metadata.title << '\n';
  oss << "  metadata.artist: " << track.metadata.artist << '\n';
  oss << "  metadata.album: " << track.metadata.album << '\n';
  oss << "  analysis.nominal_key: " << (track.analysis.nominal_key.has_value() ? *track.analysis.nominal_key : std::string{"null"})
      << '\n';
  oss << "  analysis.analysis_version: " << track.analysis.analysis_version << '\n';
  oss << "  analysis.grid_edit_revision: " << track.analysis.grid_edit_revision << '\n';
}

void append_deck_authoritative(std::ostringstream& oss, const Deck& deck, int precision) {
  oss << "Deck\n";
  oss << "  deck_id: " << deck.deck_id << '\n';
  oss << "  loaded_track: ";
  if (deck.loaded_track.has_value()) {
    oss << deck.loaded_track->track_id;
  } else {
    oss << "null";
  }
  oss << '\n';
  oss << "  p_track: " << std::fixed << std::setprecision(precision) << deck.p_track.seconds << '\n';
  oss << "  transport_state: " << transport_state_string(deck.transport_state) << '\n';
  append_optional_scalar(oss, "pending_seek_track_time", deck.pending_seek_track_time, precision);

  oss << "  slip_enabled: " << bool_string(deck.slip_enabled) << '\n';

  oss << "  quantize.enabled: " << bool_string(deck.quantize.enabled) << '\n';
  oss << "  quantize.resolution: " << std::fixed << std::setprecision(precision) << deck.quantize.resolution.beats << '\n';

  oss << "  tempo.slider_percent: " << std::fixed << std::setprecision(precision) << deck.tempo.slider_percent << '\n';
  oss << "  tempo.range_percent: " << std::fixed << std::setprecision(precision) << deck.tempo.range_percent << '\n';
  oss << "  tempo.user_rate: " << std::fixed << std::setprecision(precision) << deck.tempo.user_rate.scalar << '\n';
  oss << "  tempo.nudge_rate: " << std::fixed << std::setprecision(precision) << deck.tempo.nudge_rate.scalar << '\n';
  oss << "  tempo.sync_rate_correction: " << std::fixed << std::setprecision(precision) << deck.tempo.sync_rate_correction.scalar
      << '\n';
  oss << "  tempo.effective: " << std::fixed << std::setprecision(precision) << deck.effective_playback_rate_scalar() << '\n';

  append_optional_scalar(oss, "cue.main_cue_track_time", deck.cue.main_cue_track_time, precision);
  oss << "  cue.preview_active: " << bool_string(deck.cue.preview_active) << '\n';

  oss << "  loop.active: " << bool_string(deck.loop.active) << '\n';
  oss << "  loop.active_loop_id: ";
  if (deck.loop.active_loop_id.has_value()) {
    oss << *deck.loop.active_loop_id;
  } else {
    oss << "null";
  }
  oss << '\n';
  append_optional_scalar(oss, "loop.in_track_time", deck.loop.in_track_time, precision);
  append_optional_scalar(oss, "loop.out_track_time", deck.loop.out_track_time, precision);

  oss << "  sync.mode: " << sync_mode_string(deck.sync.mode) << '\n';
  oss << "  sync.master_flag: " << bool_string(deck.sync.master_flag) << '\n';
  oss << "  sync.key_sync_enabled: " << bool_string(deck.sync.key_sync_enabled) << '\n';

  oss << "  jog.interaction_mode: " << jog_mode_string(deck.jog.interaction_mode) << '\n';
  oss << "  jog.vinyl_mode_enabled: " << bool_string(deck.jog.vinyl_mode_enabled) << '\n';
  oss << "  jog.needle_search_enabled: " << bool_string(deck.jog.needle_search_enabled) << '\n';

  oss << "  beat_jump.increment: " << std::fixed << std::setprecision(precision) << deck.beat_jump.increment.beats << '\n';
  oss << "  hot_cue_bank_size: " << deck.hot_cue_bank_size << '\n';
}

void append_deck_derived(std::ostringstream& oss, const Deck& deck, const Track& track, int precision) {
  const double beat  = track.beat_time_map.time_to_beat(deck.p_track.seconds);
  const double bpm   = track.beat_time_map.instantaneous_bpm(deck.p_track.seconds);
  const double phase = beat - std::floor(beat);

  oss << "Derived\n";
  oss << "  beat_index: " << std::fixed << std::setprecision(precision) << beat << '\n';
  oss << "  instantaneous_bpm: " << std::fixed << std::setprecision(precision) << bpm << '\n';
  oss << "  phase: " << std::fixed << std::setprecision(precision) << phase << '\n';

  // Provisional display-only labeling derived from the musical display anchor.
  // This is useful for inspection, but should not be treated as final policy
  // for pre-anchor formatting.
  const double anchor_beat   = track.musical_display_anchor.anchor_beat.beats;
  const auto   beats_per_bar = static_cast<long long>(track.musical_display_anchor.beats_per_bar);
  if (beats_per_bar > 0) {
    const long long whole_beats_from_anchor = static_cast<long long>(std::floor(beat - anchor_beat + 1e-12));
    const long long offset_bars             = whole_beats_from_anchor / beats_per_bar;
    const long long offset_beats            = whole_beats_from_anchor % beats_per_bar;
    const long long display_measure         = track.musical_display_anchor.display_measure + offset_bars;
    long long       display_beat            = track.musical_display_anchor.display_beat_in_measure + offset_beats;

    while (display_beat <= 0) {
      display_beat += beats_per_bar;
    }
    while (display_beat > beats_per_bar) {
      display_beat -= beats_per_bar;
    }

    oss << "  display_measure_beat_provisional: " << display_measure << ':' << display_beat << '\n';
  }
}

} // namespace

std::string to_debug_string(const DeckTimingState& deck, const DebugPrintOptions& opts) {
  std::ostringstream oss;
  append_timing_harness_authoritative(oss, deck, opts.precision);
  return oss.str();
}

std::string to_debug_string(const DeckTimingState& deck, const BeatTimeMap& map, const DebugPrintOptions& opts) {
  std::ostringstream oss;
  append_timing_harness_authoritative(oss, deck, opts.precision);
  append_timing_harness_derived(oss, deck, map, opts.precision);
  return oss.str();
}

void print_debug(std::ostream& os, const DeckTimingState& deck, const DebugPrintOptions& opts) {
  os << to_debug_string(deck, opts);
}

void print_debug(std::ostream& os, const DeckTimingState& deck, const BeatTimeMap& map, const DebugPrintOptions& opts) {
  os << to_debug_string(deck, map, opts);
}

std::string to_debug_string(const Deck& deck, const DebugPrintOptions& opts) {
  std::ostringstream oss;
  append_deck_authoritative(oss, deck, opts.precision);
  return oss.str();
}

std::string to_debug_string(const Track& track, const DebugPrintOptions& opts) {
  std::ostringstream oss;
  append_track_summary(oss, track, opts.precision);
  return oss.str();
}

std::string to_debug_string(const Deck& deck, const Track& track, const DebugPrintOptions& opts) {
  std::ostringstream oss;
  append_deck_authoritative(oss, deck, opts.precision);
  append_track_summary(oss, track, opts.precision);
  append_deck_derived(oss, deck, track, opts.precision);
  return oss.str();
}

void print_debug(std::ostream& os, const Deck& deck, const DebugPrintOptions& opts) { os << to_debug_string(deck, opts); }

void print_debug(std::ostream& os, const Track& track, const DebugPrintOptions& opts) { os << to_debug_string(track, opts); }

void print_debug(std::ostream& os, const Deck& deck, const Track& track, const DebugPrintOptions& opts) {
  os << to_debug_string(deck, track, opts);
}

} // namespace djcore
