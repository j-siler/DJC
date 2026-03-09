#pragma once

#include "DomainTypes.hpp"
#include "DeckTimingEngine.hpp"
#include "BeatTimeMap.hpp"

#include <iosfwd>
#include <string>

namespace djcore {

struct DebugPrintOptions final {
    int precision {6};
    bool include_labels {true};
};

// -----------------------------------------------------------------------------
// Current timing harness view
// -----------------------------------------------------------------------------

std::string to_debug_string(const DeckTimingState& deck,
                            const DebugPrintOptions& opts = {});

std::string to_debug_string(const DeckTimingState& deck,
                            const BeatTimeMap& map,
                            const DebugPrintOptions& opts = {});

void print_debug(std::ostream& os,
                 const DeckTimingState& deck,
                 const DebugPrintOptions& opts = {});

void print_debug(std::ostream& os,
                 const DeckTimingState& deck,
                 const BeatTimeMap& map,
                 const DebugPrintOptions& opts = {});

// -----------------------------------------------------------------------------
// Newer domain model view
// -----------------------------------------------------------------------------

std::string to_debug_string(const Deck& deck,
                            const DebugPrintOptions& opts = {});

std::string to_debug_string(const Track& track,
                            const DebugPrintOptions& opts = {});

std::string to_debug_string(const Deck& deck,
                            const Track& track,
                            const DebugPrintOptions& opts = {});

void print_debug(std::ostream& os,
                 const Deck& deck,
                 const DebugPrintOptions& opts = {});

void print_debug(std::ostream& os,
                 const Track& track,
                 const DebugPrintOptions& opts = {});

void print_debug(std::ostream& os,
                 const Deck& deck,
                 const Track& track,
                 const DebugPrintOptions& opts = {});

} // namespace djcore
