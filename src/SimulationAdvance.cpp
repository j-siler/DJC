#include "djcore/SimulationAdvance.hpp"

#include <cmath>
#include <cstdint>

namespace djcore {
namespace {

constexpr std::int64_t kMillisecondsPerSecond = 1000;
constexpr std::int64_t kSecondsPerMinute = 60;
constexpr std::int64_t kMinutesPerHour = 60;
constexpr std::int64_t kHoursPerDay = 24;
constexpr std::int64_t kMillisecondsPerMinute =
    kMillisecondsPerSecond * kSecondsPerMinute;
constexpr std::int64_t kMillisecondsPerHour =
    kMillisecondsPerMinute * kMinutesPerHour;
constexpr std::int64_t kMillisecondsPerDay =
    kMillisecondsPerHour * kHoursPerDay;

std::int64_t wall_clock_to_total_milliseconds(const WallClockTime& wall_clock) {
    return static_cast<std::int64_t>(wall_clock.hour) * kMillisecondsPerHour +
           static_cast<std::int64_t>(wall_clock.minute) * kMillisecondsPerMinute +
           static_cast<std::int64_t>(wall_clock.second) * kMillisecondsPerSecond +
           static_cast<std::int64_t>(wall_clock.millisecond);
}

WallClockTime wall_clock_from_total_milliseconds(std::int64_t total_milliseconds) {
    total_milliseconds %= kMillisecondsPerDay;
    if (total_milliseconds < 0) {
        total_milliseconds += kMillisecondsPerDay;
    }

    WallClockTime wall_clock;

    wall_clock.hour = static_cast<int>(total_milliseconds / kMillisecondsPerHour);
    total_milliseconds %= kMillisecondsPerHour;

    wall_clock.minute = static_cast<int>(total_milliseconds / kMillisecondsPerMinute);
    total_milliseconds %= kMillisecondsPerMinute;

    wall_clock.second = static_cast<int>(total_milliseconds / kMillisecondsPerSecond);
    total_milliseconds %= kMillisecondsPerSecond;

    wall_clock.millisecond = static_cast<int>(total_milliseconds);

    return wall_clock;
}

void apply_pending_seek_if_any(Deck& deck) {
    if (deck.pending_seek_track_time.has_value()) {
        deck.p_track = *deck.pending_seek_track_time;
        deck.pending_seek_track_time.reset();
    }
}

void apply_simple_loop_wrap_if_needed(Deck& deck, double effective_rate) {
    if (!deck.loop.active) {
        return;
    }
    if (!deck.loop.in_track_time.has_value() || !deck.loop.out_track_time.has_value()) {
        return;
    }

    const double in_time = deck.loop.in_track_time->seconds;
    const double out_time = deck.loop.out_track_time->seconds;

    if (!(out_time > in_time)) {
        return;
    }

    const double loop_length = out_time - in_time;

    if (effective_rate >= 0.0) {
        while (deck.p_track.seconds >= out_time) {
            deck.p_track.seconds -= loop_length;
        }
    } else {
        while (deck.p_track.seconds < in_time) {
            deck.p_track.seconds += loop_length;
        }
    }
}

} // namespace

void advance_wall_clock(SimulationState& state, double delta_seconds) {
    if (!std::isfinite(delta_seconds)) {
        return;
    }

    const double exact_delta_milliseconds =
        delta_seconds * 1000.0 + state.wall_clock_fractional_milliseconds;

    std::int64_t whole_milliseconds = 0;
    if (exact_delta_milliseconds >= 0.0) {
        whole_milliseconds =
            static_cast<std::int64_t>(std::floor(exact_delta_milliseconds));
    } else {
        whole_milliseconds =
            static_cast<std::int64_t>(std::ceil(exact_delta_milliseconds));
    }

    state.wall_clock_fractional_milliseconds =
        exact_delta_milliseconds - static_cast<double>(whole_milliseconds);

    const std::int64_t updated_total_milliseconds =
        wall_clock_to_total_milliseconds(state.wall_clock) + whole_milliseconds;

    state.wall_clock = wall_clock_from_total_milliseconds(updated_total_milliseconds);
}

void advance_deck(Deck& deck, const Track* loaded_track, double delta_seconds) {
    apply_pending_seek_if_any(deck);

    if (loaded_track == nullptr) {
        return;
    }
    if (deck.transport_state != TransportState::Playing) {
        return;
    }
    if (!std::isfinite(delta_seconds)) {
        return;
    }

    const double effective_rate = deck.effective_playback_rate_scalar();
    if (!std::isfinite(effective_rate)) {
        return;
    }

    deck.p_track.seconds += delta_seconds * effective_rate;
    apply_simple_loop_wrap_if_needed(deck, effective_rate);
}

void advance_simulation(SimulationState& state, double delta_seconds) {
    const Track* track_a = nullptr;
    const Track* track_b = nullptr;

    if (state.deck_a.loaded_track.has_value()) {
        track_a = state.track_by_id(state.deck_a.loaded_track->track_id);
    }
    if (state.deck_b.loaded_track.has_value()) {
        track_b = state.track_by_id(state.deck_b.loaded_track->track_id);
    }

    advance_deck(state.deck_a, track_a, delta_seconds);
    advance_deck(state.deck_b, track_b, delta_seconds);
    advance_wall_clock(state, delta_seconds);
}

} // namespace djcore
