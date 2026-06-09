/**
 * SPDX-FileComment: AdminLogsWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file AdminLogsWidget.h
 * @brief AdminLogsWidget
 * @version 0.2.0
 * @date 2026-06-09
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

class AdminLogsWidget : public QWidget {
    Q_OBJECT
public:
    explicit AdminLogsWidget(QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onExportCsv();

private:
    QTableView *m_tableView;
    QStandardItemModel *m_model;
    QPushButton *m_refreshButton;
    QPushButton *m_exportButton;
    QNetworkAccessManager *m_networkManager;
    QString getAuthHeader();
};
