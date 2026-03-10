#include "djcore/SampleTracks.hpp"

#include <vector>

namespace djcore {
namespace {

BeatTimeMap make_constant_bpm_map(double bpm,
                                  double start_seconds,
                                  double end_seconds,
                                  double anchor_time_seconds,
                                  double anchor_beat) {
    const double beats_per_second = bpm / 60.0;

    std::vector<WarpMarker> markers;
    markers.emplace_back(
        start_seconds,
        anchor_beat + (start_seconds - anchor_time_seconds) * beats_per_second);
    markers.emplace_back(
        end_seconds,
        anchor_beat + (end_seconds - anchor_time_seconds) * beats_per_second);

    return BeatTimeMap(std::move(markers));
}

} // namespace

Track make_sample_track(const char* track_id,
                        const char* title,
                        const char* artist,
                        double bpm,
                        double nominal_start_seconds,
                        double duration_seconds,
                        double load_cue_seconds) {
    Track track;

    track.track_id = track_id;
    track.audio_source_ref = std::string("demo://") + track_id;
    track.nominal_start_T0 = TrackTime{nominal_start_seconds};
    track.duration_nominal = TrackTime{duration_seconds};

    const double preroll_start = load_cue_seconds - 8.0;
    const double map_end = nominal_start_seconds + duration_seconds + 16.0;

    track.beat_time_map = make_constant_bpm_map(
        bpm,
        preroll_start,
        map_end,
        nominal_start_seconds,
        0.0);

    track.musical_display_anchor.anchor_beat = BeatPosition{0.0};
    track.musical_display_anchor.beats_per_bar = 4;
    track.musical_display_anchor.display_measure = 1;
    track.musical_display_anchor.display_beat_in_measure = 1;

    track.metadata.title = title;
    track.metadata.artist = artist;
    track.metadata.album = "Sample Library";

    track.analysis.nominal_key = std::nullopt;
    track.analysis.analysis_version = "sample-1";
    track.analysis.grid_edit_revision = 0;

    CuePoint load_cue;
    load_cue.id = std::string(track_id) + ":load";
    load_cue.label = "Load Cue";
    load_cue.track_time = TrackTime{load_cue_seconds};
    load_cue.beat_hint = BeatPosition{
        track.beat_time_map.time_to_beat(load_cue_seconds)};
    load_cue.roles = CuePointRole::LoadCue | CuePointRole::MainCue;
    track.cue_points.push_back(load_cue);

    CuePoint hot1;
    hot1.id = std::string(track_id) + ":hot1";
    hot1.label = "Hot 1";
    hot1.track_time = TrackTime{nominal_start_seconds + 16.0};
    hot1.beat_hint = BeatPosition{
        track.beat_time_map.time_to_beat(hot1.track_time.seconds)};
    hot1.roles = CuePointRole::HotCue | CuePointRole::MemoryPoint;
    hot1.hot_cue = HotCueBinding{0};
    track.cue_points.push_back(hot1);

    CuePoint hot2;
    hot2.id = std::string(track_id) + ":hot2";
    hot2.label = "Hot 2";
    hot2.track_time = TrackTime{nominal_start_seconds + 32.0};
    hot2.beat_hint = BeatPosition{
        track.beat_time_map.time_to_beat(hot2.track_time.seconds)};
    hot2.roles = CuePointRole::HotCue | CuePointRole::MemoryPoint;
    hot2.hot_cue = HotCueBinding{1};
    track.cue_points.push_back(hot2);

    SavedLoop loop;
    loop.id = std::string(track_id) + ":loop1";
    loop.label = "Loop 1";
    loop.in_track_time = TrackTime{nominal_start_seconds + 16.0};
    loop.out_track_time = TrackTime{nominal_start_seconds + 24.0};
    loop.in_beat_hint = BeatPosition{
        track.beat_time_map.time_to_beat(loop.in_track_time.seconds)};
    loop.out_beat_hint = BeatPosition{
        track.beat_time_map.time_to_beat(loop.out_track_time.seconds)};
    loop.locked = false;
    track.saved_loops.push_back(loop);

    return track;
}

std::vector<Track> make_sample_tracks() {
    std::vector<Track> tracks;

    tracks.push_back(make_sample_track(
        "track_demo_001",
        "Demo Track One",
        "Test Artist A",
        120.0,
        0.0,
        180.0,
        -2.0));

    tracks.push_back(make_sample_track(
        "track_demo_002",
        "Demo Track Two",
        "Test Artist B",
        128.0,
        0.0,
        220.0,
        -1.875));

    tracks.push_back(make_sample_track(
        "track_demo_003",
        "Demo Track Three",
        "Test Artist C",
        124.0,
        0.0,
        210.0,
        -1.935483870967742));

    return tracks;
}

} // namespace djcore
