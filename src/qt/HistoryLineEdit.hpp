#pragma once

#include <QLineEdit>
#include <QStringList>
#include <QWidget>

class HistoryLineEdit final : public QLineEdit {
public:
    explicit HistoryLineEdit(QWidget* parent = nullptr);

    void add_history_entry(const QString& command);
    void clear_history();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void show_history_entry(int index);
    void reset_navigation_state();

private:
    QStringList m_history;
    int m_history_index {-1};
    QString m_pending_edit_line;
    bool m_browsing_history {false};
};
