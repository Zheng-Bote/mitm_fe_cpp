/**
 * SPDX-FileComment: DlqWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file DlqWidget.h
 * @brief DlqWidget
 * @version 0.2.0
 * @date 2026-06-07
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
#include <QNetworkReply>

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
    QNetworkAccessManager* m_networkManager;
};
