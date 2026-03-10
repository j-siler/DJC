#pragma once

#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QWidget>

class DeckPanel final : public QWidget {
    Q_OBJECT

public:
    explicit DeckPanel(const QString& deck_id, QWidget* parent = nullptr);

    void set_tracks(const QStringList& track_ids);
    QString selected_track_id() const;
    void set_effective_bpm_text(const QString& text);
    void set_dump_text(const QString& text);

signals:
    void load_requested(const QString& track_id);
    void unload_requested();
    void play_requested();
    void pause_requested();
    void toggle_requested();

private:
    QString m_deck_id;

    QLabel* m_title_label {nullptr};
    QLabel* m_effective_bpm_label {nullptr};
    QComboBox* m_track_selector {nullptr};
    QPushButton* m_load_button {nullptr};
    QPushButton* m_unload_button {nullptr};
    QPushButton* m_play_button {nullptr};
    QPushButton* m_pause_button {nullptr};
    QPushButton* m_toggle_button {nullptr};
    QPlainTextEdit* m_dump_text {nullptr};
};
