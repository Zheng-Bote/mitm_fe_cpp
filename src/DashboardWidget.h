/**
 * SPDX-FileComment: DashboardWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file DashboardWidget.h
 * @brief DashboardWidget
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

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
