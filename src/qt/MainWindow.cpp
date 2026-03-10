#include "MainWindow.hpp"

#include <QHBoxLayout>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>
#include <optional>
#include <sstream>

#include "djcore/DebugState.hpp"
#include "djcore/DeckTransitions.hpp"
#include "djcore/SimulationAdvance.hpp"
#include "djcore/TempoMath.hpp"

namespace {

std::optional<double> current_effective_bpm(const djcore::SimulationState& state,
                                            const djcore::Deck& deck) {
    if (!deck.loaded_track.has_value()) {
        return std::nullopt;
    }

    const djcore::Track* track = state.track_by_id(deck.loaded_track->track_id);
    if (track == nullptr) {
        return std::nullopt;
    }

    const double effective_bpm =
        djcore::effective_bpm_at_playhead(deck, *track);

    if (!std::isfinite(effective_bpm)) {
        return std::nullopt;
    }

    return effective_bpm;
}

QString effective_bpm_text(const djcore::SimulationState& state,
                           const djcore::Deck& deck) {
    const auto bpm = current_effective_bpm(state, deck);
    if (!bpm.has_value()) {
        return "--";
    }

    return QString::number(*bpm, 'f', 3);
}

std::optional<djcore::WallClockTime> parse_wall_clock(const QString& input) {
    const QStringList parts = input.split(':');
    if (parts.size() != 3) {
        return std::nullopt;
    }

    bool ok_hour = false;
    bool ok_minute = false;

    const int hour = parts[0].toInt(&ok_hour);
    const int minute = parts[1].toInt(&ok_minute);

    const QStringList sec_parts = parts[2].split('.');
    if (sec_parts.size() != 2) {
        return std::nullopt;
    }

    bool ok_second = false;
    bool ok_msec = false;

    const int second = sec_parts[0].toInt(&ok_second);
    const int millisecond = sec_parts[1].toInt(&ok_msec);

    if (!ok_hour || !ok_minute || !ok_second || !ok_msec) {
        return std::nullopt;
    }

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 59 || millisecond < 0 || millisecond > 999) {
        return std::nullopt;
    }

    return djcore::WallClockTime{hour, minute, second, millisecond};
}

bool parse_bool_token(const QString& token, bool* out_value) {
    const QString lowered = token.trimmed().toLower();
    if (lowered == "true" || lowered == "1" || lowered == "on") {
        *out_value = true;
        return true;
    }
    if (lowered == "false" || lowered == "0" || lowered == "off") {
        *out_value = false;
        return true;
    }
    return false;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_state(djcore::make_default_simulation_state()) {
    build_ui();
    wire_signals();
    refresh_view();

    setWindowTitle("DJ Core Qt Harness");
    resize(1400, 900);
}

void MainWindow::build_ui() {
    auto* central = new QWidget(this);
    auto* root_layout = new QVBoxLayout(central);
    auto* header_layout = new QHBoxLayout();
    auto* decks_layout = new QHBoxLayout();
    auto* command_layout = new QHBoxLayout();

    m_wall_clock_label = new QLabel(central);

    m_step_10ms_button = new QPushButton("Step +10 ms", central);
    m_step_100ms_button = new QPushButton("Step +100 ms", central);
    m_step_1block_button = new QPushButton("Step +1 block", central);

    m_deck_a_panel = new DeckPanel("A", central);
    m_deck_b_panel = new DeckPanel("B", central);

    m_command_line = new HistoryLineEdit(central);
    m_command_line->setPlaceholderText(
        "Commands: help | list tracks | load A track_demo_001 | play A | step 10ms | step 1block | set A p_track -2.0 | set A tempo.slider_percent 66.666667 | set A tempo.range_percent 10 | set clock 21:00:00.000");

    m_status_label = new QLabel(central);

    QStringList track_ids;
    for (const auto& track : m_state.library) {
        track_ids.push_back(QString::fromStdString(track.track_id));
    }

    m_deck_a_panel->set_tracks(track_ids);
    m_deck_b_panel->set_tracks(track_ids);

    header_layout->addWidget(m_wall_clock_label);
    header_layout->addStretch(1);
    header_layout->addWidget(m_step_10ms_button);
    header_layout->addWidget(m_step_100ms_button);
    header_layout->addWidget(m_step_1block_button);

    decks_layout->addWidget(m_deck_a_panel, 1);
    decks_layout->addWidget(m_deck_b_panel, 1);

    command_layout->addWidget(new QLabel("Command:", central));
    command_layout->addWidget(m_command_line, 1);

    root_layout->addLayout(header_layout);
    root_layout->addLayout(decks_layout, 1);
    root_layout->addLayout(command_layout);
    root_layout->addWidget(m_status_label);

    setCentralWidget(central);
}

void MainWindow::wire_signals() {
    connect(m_step_10ms_button, &QPushButton::clicked, this, [this]() {
        step_simulation_seconds(0.010, "Step +10 ms");
        refresh_view();
    });

    connect(m_step_100ms_button, &QPushButton::clicked, this, [this]() {
        step_simulation_seconds(0.100, "Step +100 ms");
        refresh_view();
    });

    connect(m_step_1block_button, &QPushButton::clicked, this, [this]() {
        step_simulation_seconds(djcore::kManualStepOneBlockSeconds, "Step +1 block");
        refresh_view();
    });

    connect(m_deck_a_panel, &DeckPanel::load_requested, this, [this](const QString& track_id) {
        if (load_track_on_deck("A", track_id)) {
            set_status(QString("Loaded %1 onto Deck A").arg(track_id));
        }
        refresh_view();
    });

    connect(m_deck_b_panel, &DeckPanel::load_requested, this, [this](const QString& track_id) {
        if (load_track_on_deck("B", track_id)) {
            set_status(QString("Loaded %1 onto Deck B").arg(track_id));
        }
        refresh_view();
    });

    connect(m_deck_a_panel, &DeckPanel::unload_requested, this, [this]() {
        if (unload_deck("A")) {
            set_status("Unloaded Deck A");
        }
        refresh_view();
    });

    connect(m_deck_b_panel, &DeckPanel::unload_requested, this, [this]() {
        if (unload_deck("B")) {
            set_status("Unloaded Deck B");
        }
        refresh_view();
    });

    connect(m_deck_a_panel, &DeckPanel::play_requested, this, [this]() {
        if (play_deck("A")) {
            set_status("Play Deck A");
        }
        refresh_view();
    });

    connect(m_deck_b_panel, &DeckPanel::play_requested, this, [this]() {
        if (play_deck("B")) {
            set_status("Play Deck B");
        }
        refresh_view();
    });

    connect(m_deck_a_panel, &DeckPanel::pause_requested, this, [this]() {
        if (pause_deck("A")) {
            set_status("Pause Deck A");
        }
        refresh_view();
    });

    connect(m_deck_b_panel, &DeckPanel::pause_requested, this, [this]() {
        if (pause_deck("B")) {
            set_status("Pause Deck B");
        }
        refresh_view();
    });

    connect(m_deck_a_panel, &DeckPanel::toggle_requested, this, [this]() {
        if (toggle_deck("A")) {
            set_status("Toggle Deck A");
        }
        refresh_view();
    });

    connect(m_deck_b_panel, &DeckPanel::toggle_requested, this, [this]() {
        if (toggle_deck("B")) {
            set_status("Toggle Deck B");
        }
        refresh_view();
    });

    connect(m_command_line, &QLineEdit::returnPressed, this, [this]() {
        const QString command = m_command_line->text().trimmed();
        if (!command.isEmpty()) {
            m_command_line->add_history_entry(command);
            const bool ok = run_command(command);
            if (ok) {
                refresh_view();
            }
        }
        m_command_line->clear();
    });
}

void MainWindow::refresh_view() {
    m_wall_clock_label->setText(QString("Wall clock: %1").arg(
        QString::fromStdString(m_state.wall_clock.to_string())));

    m_deck_a_panel->set_effective_bpm_text(
        effective_bpm_text(m_state, m_state.deck_a));
    m_deck_b_panel->set_effective_bpm_text(
        effective_bpm_text(m_state, m_state.deck_b));

    m_deck_a_panel->set_dump_text(make_deck_dump(m_state.deck_a));
    m_deck_b_panel->set_dump_text(make_deck_dump(m_state.deck_b));
}

QString MainWindow::make_deck_dump(const djcore::Deck& deck) const {
    std::ostringstream oss;

    if (deck.loaded_track.has_value()) {
        const djcore::Track* track = m_state.track_by_id(deck.loaded_track->track_id);
        if (track != nullptr) {
            djcore::print_debug(oss, deck, *track);
        } else {
            djcore::print_debug(oss, deck);
            oss << "\n[loaded track record not found in library]\n";
        }
    } else {
        djcore::print_debug(oss, deck);
    }

    return QString::fromStdString(oss.str());
}

bool MainWindow::run_command(const QString& command_text) {
    const QStringList parts = command_text.simplified().split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        set_status("Empty command");
        return false;
    }

    const QString verb = parts[0].toLower();

    if (verb == "help") {
        set_status("Commands: help | list tracks | load A track_demo_001 | unload A | play A | pause A | toggle A | step 10ms | step 100ms | step 1block | set A p_track -2.0 | set A tempo.slider_percent 66.666667 | set A tempo.range_percent 10 | set A quantize.enabled true | set clock 21:00:00.000");
        return true;
    }

    if (verb == "list" && parts.size() == 2 && parts[1].toLower() == "tracks") {
        QStringList ids;
        for (const auto& track : m_state.library) {
            ids.push_back(QString::fromStdString(track.track_id));
        }
        set_status(QString("Tracks: %1").arg(ids.join(", ")));
        return true;
    }

    if (verb == "load" && parts.size() == 3) {
        const bool ok = load_track_on_deck(parts[1], parts[2]);
        if (ok) {
            set_status(QString("Loaded %1 onto Deck %2").arg(parts[2], parts[1].toUpper()));
        }
        return ok;
    }

    if (verb == "unload" && parts.size() == 2) {
        const bool ok = unload_deck(parts[1]);
        if (ok) {
            set_status(QString("Unloaded Deck %1").arg(parts[1].toUpper()));
        }
        return ok;
    }

    if (verb == "play" && parts.size() == 2) {
        const bool ok = play_deck(parts[1]);
        if (ok) {
            set_status(QString("Play Deck %1").arg(parts[1].toUpper()));
        }
        return ok;
    }

    if (verb == "pause" && parts.size() == 2) {
        const bool ok = pause_deck(parts[1]);
        if (ok) {
            set_status(QString("Pause Deck %1").arg(parts[1].toUpper()));
        }
        return ok;
    }

    if (verb == "toggle" && parts.size() == 2) {
        const bool ok = toggle_deck(parts[1]);
        if (ok) {
            set_status(QString("Toggle Deck %1").arg(parts[1].toUpper()));
        }
        return ok;
    }

    if (verb == "step" && parts.size() == 2) {
        const QString amount = parts[1].trimmed().toLower();

        if (amount == "1block") {
            step_simulation_seconds(djcore::kManualStepOneBlockSeconds, "Step +1 block");
            return true;
        }

        if (amount.endsWith("ms")) {
            bool ok_ms = false;
            const double milliseconds =
                amount.left(amount.size() - 2).toDouble(&ok_ms);
            if (ok_ms && milliseconds >= 0.0) {
                step_simulation_seconds(milliseconds / 1000.0,
                                        QString("Step +%1").arg(amount));
                return true;
            }
        }

        set_status(QString("Invalid step amount: %1").arg(parts[1]));
        return false;
    }

    if (verb == "set" && parts.size() == 3 && parts[1].toLower() == "clock") {
        const auto parsed = parse_wall_clock(parts[2]);
        if (!parsed.has_value()) {
            set_status("Invalid wall clock format; use HH:MM:SS.mmm");
            return false;
        }
        m_state.wall_clock = *parsed;
        m_state.wall_clock_fractional_milliseconds = 0.0;
        set_status(QString("Wall clock set to %1").arg(parts[2]));
        return true;
    }

    if (verb == "set" && parts.size() == 4) {
        djcore::Deck* deck = m_state.deck_by_id(parts[1].toStdString());
        if (deck == nullptr) {
            set_status(QString("Unknown deck id: %1").arg(parts[1]));
            return false;
        }

        const QString field = parts[2];
        const QString value = parts[3];

        bool ok_num = false;
        const double number = value.toDouble(&ok_num);

        if (field == "p_track" && ok_num) {
            deck->p_track.seconds = number;
            set_status(QString("Deck %1 p_track set to %2").arg(parts[1].toUpper(), value));
            return true;
        }

        if (field == "tempo.slider_percent" && ok_num) {
            deck->tempo.slider_percent = number;
            djcore::recompute_user_rate_from_slider_and_range(deck->tempo);
            set_status(QString("Deck %1 tempo.slider_percent set to %2 -> user_rate %3")
                           .arg(parts[1].toUpper(),
                                value,
                                QString::number(deck->tempo.user_rate.scalar, 'f', 9)));
            return true;
        }

        if (field == "tempo.range_percent" && ok_num) {
            deck->tempo.range_percent = number;
            djcore::recompute_user_rate_from_slider_and_range(deck->tempo);
            set_status(QString("Deck %1 tempo.range_percent set to %2 -> user_rate %3")
                           .arg(parts[1].toUpper(),
                                value,
                                QString::number(deck->tempo.user_rate.scalar, 'f', 9)));
            return true;
        }

        if (field == "beat_jump.increment" && ok_num) {
            deck->beat_jump.increment.beats = number;
            set_status(QString("Deck %1 beat_jump.increment set to %2").arg(parts[1].toUpper(), value));
            return true;
        }

        bool ok_bool = false;
        bool bool_value = false;
        ok_bool = parse_bool_token(value, &bool_value);

        if (field == "quantize.enabled" && ok_bool) {
            deck->quantize.enabled = bool_value;
            set_status(QString("Deck %1 quantize.enabled set to %2").arg(parts[1].toUpper(), value));
            return true;
        }

        if (field == "sync.key_sync_enabled" && ok_bool) {
            deck->sync.key_sync_enabled = bool_value;
            set_status(QString("Deck %1 sync.key_sync_enabled set to %2").arg(parts[1].toUpper(), value));
            return true;
        }

        if (field == "slip_enabled" && ok_bool) {
            deck->slip_enabled = bool_value;
            set_status(QString("Deck %1 slip_enabled set to %2").arg(parts[1].toUpper(), value));
            return true;
        }

        set_status(QString("Unsupported field or invalid value: %1").arg(field));
        return false;
    }

    set_status(QString("Unknown command: %1").arg(command_text));
    return false;
}

bool MainWindow::load_track_on_deck(const QString& deck_id, const QString& track_id) {
    djcore::Deck* deck = m_state.deck_by_id(deck_id.toStdString());
    const djcore::Track* track = m_state.track_by_id(track_id.toStdString());

    if (deck == nullptr) {
        set_status(QString("Unknown deck id: %1").arg(deck_id));
        return false;
    }
    if (track == nullptr) {
        set_status(QString("Unknown track id: %1").arg(track_id));
        return false;
    }

    djcore::load_track(*deck, *track);
    return true;
}

bool MainWindow::unload_deck(const QString& deck_id) {
    djcore::Deck* deck = m_state.deck_by_id(deck_id.toStdString());
    if (deck == nullptr) {
        set_status(QString("Unknown deck id: %1").arg(deck_id));
        return false;
    }

    djcore::unload_track(*deck);
    return true;
}

bool MainWindow::play_deck(const QString& deck_id) {
    djcore::Deck* deck = m_state.deck_by_id(deck_id.toStdString());
    if (deck == nullptr) {
        set_status(QString("Unknown deck id: %1").arg(deck_id));
        return false;
    }

    const bool ok = djcore::play(*deck);
    if (!ok) {
        set_status(QString("Play ignored for Deck %1").arg(deck_id.toUpper()));
    }
    return ok;
}

bool MainWindow::pause_deck(const QString& deck_id) {
    djcore::Deck* deck = m_state.deck_by_id(deck_id.toStdString());
    if (deck == nullptr) {
        set_status(QString("Unknown deck id: %1").arg(deck_id));
        return false;
    }

    const bool ok = djcore::pause(*deck);
    if (!ok) {
        set_status(QString("Pause ignored for Deck %1").arg(deck_id.toUpper()));
    }
    return ok;
}

bool MainWindow::toggle_deck(const QString& deck_id) {
    djcore::Deck* deck = m_state.deck_by_id(deck_id.toStdString());
    if (deck == nullptr) {
        set_status(QString("Unknown deck id: %1").arg(deck_id));
        return false;
    }

    const bool ok = djcore::toggle_play_pause(*deck);
    if (!ok) {
        set_status(QString("Toggle ignored for Deck %1").arg(deck_id.toUpper()));
    }
    return ok;
}

void MainWindow::step_simulation_seconds(double delta_seconds, const QString& label) {
    djcore::advance_simulation(m_state, delta_seconds);
    set_status(QString("%1 -> wall clock %2")
                   .arg(label, QString::fromStdString(m_state.wall_clock.to_string())));
}

void MainWindow::set_status(const QString& text) {
    m_status_label->setText(text);
}
