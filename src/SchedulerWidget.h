#pragma once

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <nlohmann/json.hpp>

class SchedulerWidget : public QWidget {
    Q_OBJECT

public:
    explicit SchedulerWidget(QWidget *parent = nullptr);
    ~SchedulerWidget() override = default;

private slots:
    void onRefreshClicked();
    void onApiResponse(QNetworkReply* reply);
    
    // New action slots
    void onAddJob();
    void onEditJob();
    void onDeleteJob();

private:
    QString getAuthHeader();
    nlohmann::json m_currentJobs; // Holds the fetched JSON array

    QTableView* m_tableView;
    QStandardItemModel* m_model;
    QPushButton* m_refreshButton;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QNetworkAccessManager* m_networkManager;
};
