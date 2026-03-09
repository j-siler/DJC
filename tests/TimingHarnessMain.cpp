// TimingHarnessMain.cpp
// DJCoreTimingHarness
//
// Deterministic math harness validating:
// - BeatTimeMap inversion correctness
// - Negative-time / negative-beat extrapolation correctness
// - Extrapolation beyond the last marker
// - DeckTimingEngine block advancement
// - Quantized beat jumps landing on-grid
// - Long-run drift behavior
// - Two-deck phase convergence with a simple controller
//
// SPDX-License-Identifier: MIT

#include "djcore/BeatTimeMap.hpp"
#include "djcore/DeckTimingEngine.hpp"
#include "djcore/SyncEngine.hpp"
#include "djcore/DebugState.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct TestResult {
  std::string name;
  bool        pass{false};
  std::string message;
};

constexpr double kEpsT = 1e-9;

bool nearly_equal(double a, double b, double eps) { return std::abs(a - b) <= eps; }

void fail_if(bool cond, const std::string& msg) {
  if (cond) {
    throw std::runtime_error(msg);
  }
}

djcore::BeatTimeMap make_constant_bpm_map(double bpm, double duration_seconds) {
  const double                    beats = (bpm / 60.0) * duration_seconds;
  std::vector<djcore::WarpMarker> m;
  m.emplace_back(0.0, 0.0);
  m.emplace_back(duration_seconds, beats);
  return djcore::BeatTimeMap(std::move(m));
}

djcore::BeatTimeMap make_two_segment_map() {
  std::vector<djcore::WarpMarker> m;
  m.emplace_back(0.0, 0.0);
  m.emplace_back(60.0, 120.0);  // 120 BPM
  m.emplace_back(120.0, 260.0); // then 140 BPM
  return djcore::BeatTimeMap(std::move(m));
}

TestResult test1_constant_bpm_inversion() {
  TestResult r;
  r.name = "T1 Constant BPM inversion";

  try {
    const auto map = make_constant_bpm_map(128.0, 60.0);

    fail_if(!nearly_equal(map.time_to_beat(30.0), 64.0, 1e-12), "t=30s expected b=64");
    fail_if(!nearly_equal(map.beat_to_time(64.0), 30.0, 1e-12), "b=64 expected t=30s");
    fail_if(!nearly_equal(map.instantaneous_bpm(10.0), 128.0, 1e-12), "instantaneous BPM mismatch");

    std::mt19937_64                        rng(123456);
    std::uniform_real_distribution<double> dist(0.0, 60.0);
    for (int i = 0; i < 20000; ++i) {
      const double t  = dist(rng);
      const double b  = map.time_to_beat(t);
      const double t2 = map.beat_to_time(b);
      fail_if(std::abs(t2 - t) > kEpsT, "invert time->beat->time exceeded eps");
    }

    r.pass = true;
  } catch (const std::exception& e) {
    r.pass    = false;
    r.message = e.what();
  }
  return r;
}

TestResult test2_tempo_change_inversion() {
  TestResult r;
  r.name = "T2 Tempo change inversion";

  try {
    const auto map = make_two_segment_map();

    fail_if(!nearly_equal(map.instantaneous_bpm(10.0), 120.0, 1e-12), "segment1 BPM mismatch");
    fail_if(!nearly_equal(map.instantaneous_bpm(90.0), 140.0, 1e-12), "segment2 BPM mismatch");

    std::mt19937_64                        rng(654321);
    std::uniform_real_distribution<double> dist(0.0, 120.0);
    for (int i = 0; i < 40000; ++i) {
      const double t  = dist(rng);
      const double b  = map.time_to_beat(t);
      const double t2 = map.beat_to_time(b);
      fail_if(std::abs(t2 - t) > 5e-9, "invert time->beat->time exceeded eps (tempo change)");
    }

    r.pass = true;
  } catch (const std::exception& e) {
    r.pass    = false;
    r.message = e.what();
  }
  return r;
}

TestResult test3_negative_domain_constant_bpm() {
  TestResult r;
  r.name = "T3 Negative-domain constant BPM";

  try {
    const auto map = make_constant_bpm_map(120.0, 60.0);

    fail_if(!nearly_equal(map.time_to_beat(-0.5), -1.0, 1e-12), "t=-0.5s expected b=-1");
    fail_if(!nearly_equal(map.time_to_beat(-2.0), -4.0, 1e-12), "t=-2.0s expected b=-4");
    fail_if(!nearly_equal(map.beat_to_time(-1.0), -0.5, 1e-12), "b=-1 expected t=-0.5");
    fail_if(!nearly_equal(map.beat_to_time(-4.0), -2.0, 1e-12), "b=-4 expected t=-2.0");
    fail_if(!nearly_equal(map.instantaneous_bpm(-10.0), 120.0, 1e-12), "negative-time BPM mismatch");

    std::mt19937_64                        rng(777777);
    std::uniform_real_distribution<double> dist(-30.0, 60.0);
    for (int i = 0; i < 30000; ++i) {
      const double t  = dist(rng);
      const double b  = map.time_to_beat(t);
      const double t2 = map.beat_to_time(b);
      fail_if(std::abs(t2 - t) > kEpsT, "invert time->beat->time exceeded eps across negative region");
    }

    r.pass = true;
  } catch (const std::exception& e) {
    r.pass    = false;
    r.message = e.what();
  }
  return r;
}

TestResult test4_extrapolation_beyond_map_extents() {
  TestResult r;
  r.name = "T4 Extrapolation beyond map extents";

  try {
    const auto map = make_two_segment_map();

    // Before first marker: first segment slope (120 BPM => 2 beats/sec)
    fail_if(!nearly_equal(map.time_to_beat(-1.0), -2.0, 1e-12), "pre-first-marker time extrapolation mismatch");
    fail_if(!nearly_equal(map.beat_to_time(-2.0), -1.0, 1e-12), "pre-first-marker beat extrapolation mismatch");

    // After last marker: last segment slope (140 BPM => 7/3 beats/sec)
    fail_if(!nearly_equal(map.time_to_beat(126.0), 274.0, 1e-12), "post-last-marker time extrapolation mismatch");
    fail_if(!nearly_equal(map.beat_to_time(274.0), 126.0, 1e-12), "post-last-marker beat extrapolation mismatch");

    std::mt19937_64                        rng(888888);
    std::uniform_real_distribution<double> dist(-20.0, 150.0);
    for (int i = 0; i < 50000; ++i) {
      const double t  = dist(rng);
      const double b  = map.time_to_beat(t);
      const double t2 = map.beat_to_time(b);
      fail_if(std::abs(t2 - t) > 5e-9, "invert time->beat->time exceeded eps across full extrapolated span");
    }

    r.pass = true;
  } catch (const std::exception& e) {
    r.pass    = false;
    r.message = e.what();
  }
  return r;
}

TestResult test5_quantized_jump_lands_on_grid() {
  TestResult r;
  r.name = "T5 Quantized jump lands on-grid";

  try {
    const auto               map = make_constant_bpm_map(128.0, 180.0);
    djcore::DeckTimingEngine eng;
    djcore::DeckTimingState  deck;
    deck.t_seconds      = 10.0;
    deck.playing        = true;
    deck.quantize       = true;
    deck.quantize_beats = 1.0;

    const double b_now  = map.time_to_beat(deck.t_seconds);
    const double target = b_now + 17.3;

    eng.schedule_jump_to_beat(deck, map, target);

    // Apply seek at the next block boundary (start-of-block), but do not advance time yet.
    eng.apply_pending_seek_if_any(deck);

    const double b_after_seek = map.time_to_beat(deck.t_seconds);
    const double frac         = b_after_seek - std::floor(b_after_seek);
    fail_if(std::abs(frac) > 1e-10, "post-seek beat is not integer (quantize failed)");

    r.pass = true;
  } catch (const std::exception& e) {
    r.pass    = false;
    r.message = e.what();
  }
  return r;
}

TestResult test6_long_run_drift() {
  TestResult r;
  r.name = "T6 Long-run drift (1 hour simulated)";

  try {
    const auto               map = make_two_segment_map();
    djcore::DeckTimingEngine eng;
    djcore::DeckTimingState  deck;
    std::cout << "\nSample initial state dump:\n";
    djcore::print_debug(std::cout, deck, map);
    deck.t_seconds = 0.0;
    deck.playing   = true;

    const double Fs    = 48000.0;
    const int    block = 512;
    const double dt    = block / Fs;

    const double sim_seconds = 3600.0;
    const int    steps       = static_cast<int>(sim_seconds / dt);
    const int    every       = std::max(1, static_cast<int>(1.0 / dt)); // ~1 Hz checks

    for (int i = 0; i < steps; ++i) {
      eng.on_audio_block(deck, dt);

      if ((i % every) == 0) {
        const double t  = deck.t_seconds;
        const double b  = map.time_to_beat(t);
        const double t2 = map.beat_to_time(b);
        fail_if(std::abs(t2 - t) > 2e-7, "drift exceeded tolerance");
      }
    }

    r.pass = true;
    std::cout << "\nSample final state dump:\n";
    djcore::print_debug(std::cout, deck, map);
  } catch (const std::exception& e) {
    r.pass    = false;
    r.message = e.what();
  }
  return r;
}

TestResult test7_two_deck_phase_convergence() {
  TestResult r;
  r.name = "T7 Two-deck phase convergence";

  try {
    const auto                  map = make_constant_bpm_map(128.0, 300.0);
    djcore::DeckTimingEngine    eng;
    djcore::SyncEngine          sync(djcore::SyncControllerConfig{.kp = 0.35, .ki = 0.00, .max_correction = 0.02});
    djcore::SyncControllerState ctrl;

    djcore::DeckTimingState master;
    djcore::DeckTimingState follower;

    master.t_seconds   = 0.0;
    follower.t_seconds = map.beat_to_time(0.37); // initial offset

    const double Fs    = 48000.0;
    const int    block = 512;
    const double dt    = block / Fs;

    double phase_err_end = 0.0;

    const int steps = static_cast<int>(20.0 / dt);
    const int every = std::max(1, static_cast<int>(0.1 / dt));

    for (int i = 0; i < steps; ++i) {
      follower.r_sync_correction = sync.compute_rate_correction(map, master, map, follower, ctrl, dt);
      eng.on_audio_block(master, dt);
      eng.on_audio_block(follower, dt);

      if ((i % every) == 0) {
        const double bM     = master.beat_index(map);
        const double bF     = follower.beat_index(map);
        const double phaseM = bM - std::floor(bM);
        const double phaseF = bF - std::floor(bF);
        phase_err_end       = djcore::SyncEngine::wrap_to_half(phaseM - phaseF);
      }
    }

    fail_if(std::abs(phase_err_end) > 0.10, "phase did not converge sufficiently (<0.10 beats)");

    r.pass = true;
  } catch (const std::exception& e) {
    r.pass    = false;
    r.message = e.what();
  }
  return r;
}

} // namespace

int main() {
  std::vector<TestResult> results;
  results.push_back(test1_constant_bpm_inversion());
  results.push_back(test2_tempo_change_inversion());
  results.push_back(test3_negative_domain_constant_bpm());
  results.push_back(test4_extrapolation_beyond_map_extents());
  results.push_back(test5_quantized_jump_lands_on_grid());
  results.push_back(test6_long_run_drift());
  results.push_back(test7_two_deck_phase_convergence());

  bool all_pass = true;

  std::cout << "DJCoreTimingHarness\n";
  for (const auto& tr : results) {
    std::cout << std::left << std::setw(40) << tr.name << (tr.pass ? " PASS" : " FAIL");
    if (!tr.pass && !tr.message.empty()) {
      std::cout << "  (" << tr.message << ")";
    }
    std::cout << "\n";
    all_pass = all_pass && tr.pass;
  }
  return all_pass ? 0 : 1;
}
