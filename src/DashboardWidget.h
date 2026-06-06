#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QNetworkAccessManager>

class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget() override = default;

private slots:
    void onRefreshClicked();

private:
    void fetchInfo();
    void fetchHealth();
    void fetchJobs();
    QString getAuthHeader();

    QLabel* m_healthLabel;
    QLabel* m_engineLabel;
    QLabel* m_jobsLabel;
    QPushButton* m_refreshButton;
    QNetworkAccessManager* m_networkManager;
};
