/**
 * SPDX-FileComment: AuditLogsWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file AuditLogsWidget.h
 * @brief AuditLogsWidget
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
#include <QDateEdit>
#include <QCheckBox>

class AuditLogsWidget : public QWidget {
    Q_OBJECT

public:
    explicit AuditLogsWidget(QWidget *parent = nullptr);
    ~AuditLogsWidget() override = default;

private slots:
    void onRefresh();
    void onExportCsv();
    void onExportReport();

private:
    void generateExcelReport(const QByteArray& data, const QString& fileName, const QString& jobName, const QString& topic, const QDateTime& startDate, const QDateTime& endDate);
    QTableView* m_tableView;
    QStandardItemModel* m_model;
    QPushButton* m_refreshButton;
    QPushButton* m_exportButton;
    QPushButton* m_exportReportButton;
    QCheckBox* m_useDateRangeCheckbox;
    QDateEdit* m_startDateEdit;
    QDateEdit* m_endDateEdit;
};
