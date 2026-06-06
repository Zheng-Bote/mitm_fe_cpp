#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class RulesWidget : public QWidget {
    Q_OBJECT

public:
    explicit RulesWidget(QWidget *parent = nullptr);
    ~RulesWidget() override = default;

private slots:
    void onAddRule();

private:
    QTableWidget* m_rulesTable;
    QPushButton* m_addButton;
    QPushButton* m_saveButton;
};
