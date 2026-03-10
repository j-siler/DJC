#pragma once

#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QString>

#include "DeckPanel.hpp"
#include "HistoryLineEdit.hpp"
#include "djcore/SimulationState.hpp"

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void build_ui();
    void wire_signals();
    void refresh_view();

    QString make_deck_dump(const djcore::Deck& deck) const;
    bool run_command(const QString& command_text);

    bool load_track_on_deck(const QString& deck_id, const QString& track_id);
    bool unload_deck(const QString& deck_id);
    bool play_deck(const QString& deck_id);
    bool pause_deck(const QString& deck_id);
    bool toggle_deck(const QString& deck_id);

    void step_simulation_seconds(double delta_seconds, const QString& label);
    void set_status(const QString& text);

private:
    djcore::SimulationState m_state;

    QLabel* m_wall_clock_label {nullptr};

    QPushButton* m_step_10ms_button {nullptr};
    QPushButton* m_step_100ms_button {nullptr};
    QPushButton* m_step_1block_button {nullptr};

    DeckPanel* m_deck_a_panel {nullptr};
    DeckPanel* m_deck_b_panel {nullptr};
    HistoryLineEdit* m_command_line {nullptr};
    QLabel* m_status_label {nullptr};
};
