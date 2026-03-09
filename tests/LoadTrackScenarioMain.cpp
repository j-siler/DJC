#include "djcore/BeatTimeMap.hpp"
#include "djcore/DebugState.hpp"
#include "djcore/DeckTransitions.hpp"
#include "djcore/DomainTypes.hpp"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void fail_if(bool cond, const std::string& msg) {
    if (cond) {
        throw std::runtime_error(msg);
    }
}

djcore::BeatTimeMap make_demo_map() {
    std::vector<djcore::WarpMarker> markers;

    // Constant 120 BPM with meaningful pre-roll before nominal start.
    // At t = -2.0 sec we are at beat -4.0.
    markers.emplace_back(-2.0, -4.0);
    markers.emplace_back(180.0, 360.0);

    return djcore::BeatTimeMap(std::move(markers));
}

djcore::Track make_demo_track() {
    djcore::Track track;

    track.track_id = "track_demo_001";
    track.audio_source_ref = "demo://track_demo_001";
    track.nominal_start_T0 = djcore::TrackTime{0.0};
    track.duration_nominal = djcore::TrackTime{180.0};
    track.beat_time_map = make_demo_map();

    track.musical_display_anchor.anchor_beat = djcore::BeatPosition{0.0};
    track.musical_display_anchor.beats_per_bar = 4;
    track.musical_display_anchor.display_measure = 1;
    track.musical_display_anchor.display_beat_in_measure = 1;

    track.metadata.title = "Demo Track";
    track.metadata.artist = "Test Artist";
    track.metadata.album = "Timing Tests";

    djcore::CuePoint load_cue;
    load_cue.id = "cue_load";
    load_cue.label = "Load Cue";
    load_cue.track_time = djcore::TrackTime{-2.0};
    load_cue.beat_hint = djcore::BeatPosition{-4.0};
    load_cue.roles = djcore::CuePointRole::LoadCue | djcore::CuePointRole::MainCue;

    track.cue_points.push_back(load_cue);

    return track;
}

void configure_unloaded_deck_with_nontrivial_state(djcore::Deck& deck) {
    deck.deck_id = "A";

    // A few persistent deck-local settings that should survive track load.
    deck.quantize.enabled = true;
    deck.quantize.resolution = djcore::BeatPosition{1.0};

    deck.tempo.slider_percent = 3.0;
    deck.tempo.range_percent = 10.0;
    deck.tempo.user_rate.scalar = 1.03;

    deck.sync.mode = djcore::SyncMode::BeatSync;
    deck.sync.master_flag = false;
    deck.sync.key_sync_enabled = true;

    deck.beat_jump.increment = djcore::BeatPosition{4.0};
    deck.hot_cue_bank_size = 8;

    // Some stale live state that load_track() should clear/reset.
    deck.pending_seek_track_time = djcore::TrackTime{99.0};
    deck.cue.preview_active = true;
    deck.loop.active = true;
    deck.loop.in_track_time = djcore::TrackTime{32.0};
    deck.loop.out_track_time = djcore::TrackTime{40.0};
    deck.tempo.nudge_rate.scalar = 0.98;
    deck.tempo.sync_rate_correction.scalar = 1.01;
}

void verify_post_load_state(const djcore::Deck& deck, const djcore::Track& track) {
    fail_if(!deck.loaded_track.has_value(), "deck should have a loaded track");
    fail_if(deck.loaded_track->track_id != track.track_id, "loaded track id mismatch");

    fail_if(deck.transport_state != djcore::TransportState::Paused,
            "deck should be paused immediately after load");

    fail_if(std::abs(deck.p_track.seconds - (-2.0)) > 1e-12,
            "load position should come from LoadCue at t=-2.0");

    fail_if(deck.pending_seek_track_time.has_value(),
            "pending seek should be cleared by load");

    fail_if(deck.cue.preview_active,
            "cue preview should be cleared by load");

    fail_if(!deck.cue.main_cue_track_time.has_value(),
            "main cue should be initialized on load");

    fail_if(std::abs(deck.cue.main_cue_track_time->seconds - (-2.0)) > 1e-12,
            "main cue should be initialized to load position");

    fail_if(deck.loop.active, "active loop should be cleared by load");
    fail_if(deck.loop.in_track_time.has_value(), "loop in-point should be cleared by load");
    fail_if(deck.loop.out_track_time.has_value(), "loop out-point should be cleared by load");

    fail_if(std::abs(deck.tempo.nudge_rate.scalar - 1.0) > 1e-12,
            "nudge rate should be neutralized by load");

    fail_if(std::abs(deck.tempo.sync_rate_correction.scalar - 1.0) > 1e-12,
            "sync correction should be neutralized by load");

    // Persistent deck-local settings should survive.
    fail_if(!deck.quantize.enabled, "quantize setting should survive load");
    fail_if(std::abs(deck.quantize.resolution.beats - 1.0) > 1e-12,
            "quantize resolution should survive load");
    fail_if(std::abs(deck.tempo.user_rate.scalar - 1.03) > 1e-12,
            "user tempo setting should survive load");
    fail_if(deck.sync.mode != djcore::SyncMode::BeatSync,
            "sync mode should survive load");
    fail_if(!deck.sync.key_sync_enabled,
            "key sync setting should survive load");
}

} // namespace

int main() {
    try {
        constexpr const char* wall_clock = "21:00:00.000";

        djcore::Deck deck;
        configure_unloaded_deck_with_nontrivial_state(deck);

        const djcore::Track track = make_demo_track();

        std::cout << "Load Track scenario\n";
        std::cout << "Wall clock: " << wall_clock << "\n\n";

        std::cout << "Pre-state (authoritative only):\n";
        djcore::print_debug(std::cout, deck);
        std::cout << "\n";

        djcore::load_track(deck, track);

        verify_post_load_state(deck, track);

        std::cout << "Post-state (authoritative + derived):\n";
        djcore::print_debug(std::cout, deck, track);
        std::cout << "\n";

        std::cout << "RESULT: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "RESULT: FAIL\n";
        std::cout << e.what() << "\n";
        return 1;
    }
}
