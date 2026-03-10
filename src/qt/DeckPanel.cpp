#include "DeckPanel.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>

DeckPanel::DeckPanel(const QString& deck_id, QWidget* parent)
    : QWidget(parent),
      m_deck_id(deck_id) {
    m_title_label = new QLabel(QString("Deck %1").arg(m_deck_id), this);
    m_effective_bpm_label = new QLabel("Effective BPM: --", this);

    m_track_selector = new QComboBox(this);
    m_track_selector->setEditable(false);

    m_load_button = new QPushButton("Load", this);
    m_unload_button = new QPushButton("Unload", this);
    m_play_button = new QPushButton("Play", this);
    m_pause_button = new QPushButton("Pause", this);
    m_toggle_button = new QPushButton("Toggle", this);

    m_dump_text = new QPlainTextEdit(this);
    m_dump_text->setReadOnly(true);
    m_dump_text->setLineWrapMode(QPlainTextEdit::NoWrap);

    auto* top_layout = new QVBoxLayout(this);
    auto* header_layout = new QHBoxLayout();
    auto* select_layout = new QHBoxLayout();
    auto* button_layout = new QHBoxLayout();

    header_layout->addWidget(m_title_label);
    header_layout->addStretch(1);
    header_layout->addWidget(m_effective_bpm_label);

    select_layout->addWidget(new QLabel("Track:", this));
    select_layout->addWidget(m_track_selector, 1);
    select_layout->addWidget(m_load_button);
    select_layout->addWidget(m_unload_button);

    button_layout->addWidget(m_play_button);
    button_layout->addWidget(m_pause_button);
    button_layout->addWidget(m_toggle_button);
    button_layout->addStretch(1);

    top_layout->addLayout(header_layout);
    top_layout->addLayout(select_layout);
    top_layout->addLayout(button_layout);
    top_layout->addWidget(m_dump_text, 1);

    connect(m_load_button, &QPushButton::clicked, this, [this]() {
        emit load_requested(selected_track_id());
    });
    connect(m_unload_button, &QPushButton::clicked, this, &DeckPanel::unload_requested);
    connect(m_play_button, &QPushButton::clicked, this, &DeckPanel::play_requested);
    connect(m_pause_button, &QPushButton::clicked, this, &DeckPanel::pause_requested);
    connect(m_toggle_button, &QPushButton::clicked, this, &DeckPanel::toggle_requested);
}

void DeckPanel::set_tracks(const QStringList& track_ids) {
    const QString current = selected_track_id();

    m_track_selector->clear();
    m_track_selector->addItems(track_ids);

    const int idx = m_track_selector->findText(current);
    if (idx >= 0) {
        m_track_selector->setCurrentIndex(idx);
    }
}

QString DeckPanel::selected_track_id() const {
    return m_track_selector->currentText();
}

void DeckPanel::set_effective_bpm_text(const QString& text) {
    m_effective_bpm_label->setText(QString("Effective BPM: %1").arg(text));
}

void DeckPanel::set_dump_text(const QString& text) {
    m_dump_text->setPlainText(text);
}
