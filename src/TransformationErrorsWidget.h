/**
 * SPDX-FileComment: TransformationErrorsWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QNetworkAccessManager>

class TransformationErrorsWidget : public QWidget {
    Q_OBJECT

public:
    explicit TransformationErrorsWidget(QWidget *parent = nullptr);
    ~TransformationErrorsWidget() override = default;

private slots:
    void onRefresh();
    void onExportCsv();

private:
    QString getAuthHeader();

    QTableView* m_tableView;
    QStandardItemModel* m_model;
    QPushButton* m_refreshButton;
    QPushButton* m_exportButton;
    QNetworkAccessManager* m_networkManager;
};
