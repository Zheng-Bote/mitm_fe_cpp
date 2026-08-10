/**
 * SPDX-FileComment: SchedulerWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file SchedulerWidget.h
 * @brief SchedulerWidget
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

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
    void onStopJob();
    void onExecuteJob();

private:
    QString getAuthHeader();
    nlohmann::json m_currentJobs; // Holds the fetched JSON array

    QTableView* m_tableView;
    QStandardItemModel* m_model;
    QPushButton* m_refreshButton;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_stopButton;
    QPushButton* m_executeButton;
    QNetworkAccessManager* m_networkManager;
};
