#include "HistoryLineEdit.hpp"

#include <QKeyEvent>

HistoryLineEdit::HistoryLineEdit(QWidget* parent)
    : QLineEdit(parent) {
}

void HistoryLineEdit::add_history_entry(const QString& command) {
    const QString trimmed = command.trimmed();
    if (trimmed.isEmpty()) {
        reset_navigation_state();
        return;
    }

    if (m_history.isEmpty() || m_history.back() != trimmed) {
        m_history.push_back(trimmed);
    }

    reset_navigation_state();
}

void HistoryLineEdit::clear_history() {
    m_history.clear();
    reset_navigation_state();
}

void HistoryLineEdit::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Up:
        if (m_history.isEmpty()) {
            return;
        }

        if (!m_browsing_history) {
            m_pending_edit_line = text();
            m_browsing_history = true;
            m_history_index = m_history.size() - 1;
            show_history_entry(m_history_index);
            return;
        }

        if (m_history_index > 0) {
            --m_history_index;
            show_history_entry(m_history_index);
        }
        return;

    case Qt::Key_Down:
        if (!m_browsing_history) {
            return;
        }

        if (m_history_index < m_history.size() - 1) {
            ++m_history_index;
            show_history_entry(m_history_index);
            return;
        }

        setText(m_pending_edit_line);
        setCursorPosition(text().size());
        reset_navigation_state();
        return;

    default:
        break;
    }

    QLineEdit::keyPressEvent(event);
}

void HistoryLineEdit::show_history_entry(int index) {
    if (index < 0 || index >= m_history.size()) {
        return;
    }

    setText(m_history[index]);
    setCursorPosition(text().size());
}

void HistoryLineEdit::reset_navigation_state() {
    m_history_index = -1;
    m_pending_edit_line.clear();
    m_browsing_history = false;
}
