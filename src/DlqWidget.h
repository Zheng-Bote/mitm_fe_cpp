#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class DlqWidget : public QWidget {
    Q_OBJECT

public:
    explicit DlqWidget(QWidget *parent = nullptr);
    ~DlqWidget() override = default;

private slots:
    void onRefresh();
    void onRequeue();

private:
    QTableWidget* m_dlqTable;
    QPushButton* m_refreshButton;
    QPushButton* m_requeueButton;
};
