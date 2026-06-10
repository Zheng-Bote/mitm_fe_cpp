/**
 * SPDX-FileComment: TargetCredentialsWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file TargetCredentialsWidget.h
 * @brief Widget to manage source_credentials.
 * @version 0.2.0
 * @date 2026-06-08
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QNetworkAccessManager>

class TargetCredentialsWidget : public QWidget {
    Q_OBJECT

public:
    explicit TargetCredentialsWidget(QWidget *parent = nullptr);
    ~TargetCredentialsWidget() override = default;

private slots:
    void onRefresh();
    void onAddTarget();
    void onEditTarget();

private:
    void setupUi();
    void fetchTargets();
    QString getAuthHeader();

    QTableWidget* m_table;
    QPushButton* m_refreshBtn;
    QPushButton* m_addBtn;
    QPushButton* m_editBtn;
    QNetworkAccessManager* m_networkManager;
};
