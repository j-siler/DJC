// TimingHarnessMain.cpp
// DJCoreTimingHarness
//
// Deterministic math harness validating:
// - BeatTimeMap inversion correctness
// - DeckTimingEngine block advancement
// - Quantized beat jumps landing on-grid
// - Long-run drift behavior
// - Two-deck phase convergence with a simple controller
//
// SPDX-License-Identifier: MIT

#include "djcore/BeatTimeMap.hpp"
#include "djcore/DeckTimingEngine.hpp"
#include "djcore/SyncEngine.hpp"
#include "DjCoreDomainTypes.hpp"

#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <cmath>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace {

struct TestResult {
  std::string name;
  bool        pass{false};
  std::string message;
};

constexpr double kEpsT = 1e-9;

bool nearly_equal(double a, double b, double eps) { return std::abs(a - b) <= eps; }

void fail_if(bool cond, const std::string& msg) {
  if (cond) throw std::runtime_error(msg);
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

TestResult test3_quantized_jump_lands_on_grid() {
  TestResult r;
  r.name = "T3 Quantized jump lands on-grid";

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

TestResult test4_long_run_drift() {
  TestResult r;
  r.name = "T4 Long-run drift (1 hour simulated)";

  try {
    const auto               map = make_two_segment_map();
    djcore::DeckTimingEngine eng;
    djcore::DeckTimingState  deck;
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
  } catch (const std::exception& e) {
    r.pass    = false;
    r.message = e.what();
  }
  return r;
}

TestResult test5_two_deck_phase_convergence() {
  TestResult r;
  r.name = "T5 Two-deck phase convergence";

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
  results.push_back(test3_quantized_jump_lands_on_grid());
  results.push_back(test4_long_run_drift());
  results.push_back(test5_two_deck_phase_convergence());

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
