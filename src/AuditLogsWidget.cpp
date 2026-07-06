/**
 * SPDX-FileComment: AuditLogsWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file AuditLogsWidget.cpp
 * @brief AuditLogsWidget
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "AuditLogsWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QDateTime>
#include <QTimeZone>
#include <QProcessEnvironment>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "Config.h"

using json = nlohmann::json;

AuditLogsWidget::AuditLogsWidget(QWidget *parent)
    : QWidget(parent), m_networkManager(new QNetworkAccessManager(this))
{
    auto mainLayout = new QVBoxLayout(this);

    auto headerLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton("Refresh Job Audits", this);
    m_exportButton = new QPushButton("Export CSV", this);
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_exportButton);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    m_tableView = new QTableView(this);
    m_model = new QStandardItemModel(0, 5, this);
    QTimeZone tz = QTimeZone::systemTimeZone();
    QString tsHeader = QString("Timestamp (%1)").arg(QString(tz.id()));
    m_model->setHorizontalHeaderLabels({"ID", tsHeader, "Run ID", "Component", "Message"});
    
    m_tableView->setModel(m_model);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSortingEnabled(true);
    
    mainLayout->addWidget(m_tableView);

    connect(m_refreshButton, &QPushButton::clicked, this, &AuditLogsWidget::onRefresh);
    connect(m_exportButton, &QPushButton::clicked, this, &AuditLogsWidget::onExportCsv);
}

QString AuditLogsWidget::getAuthHeader() {
    return mitm::config::ConfigManager::GetInstance().GetAuthHeader();
}

void AuditLogsWidget::onRefresh() {
    m_refreshButton->setEnabled(false);
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/logs/job-audit");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_refreshButton->setEnabled(true);

        if (reply->error() == QNetworkReply::NoError) {
            m_model->setRowCount(0);
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    for (const auto& log : data) {
                        QList<QStandardItem*> rowItems;
                        rowItems << new QStandardItem(QString::number(log.value("id", 0)));
                        
                        QString rawTs = QString::fromStdString(log.value("ts", ""));
                        QDateTime dt = QDateTime::fromString(rawTs, Qt::ISODate);
                        QString tsStr = dt.isValid() ? dt.toLocalTime().toString("yyyy-MM-dd HH:mm:ss") : rawTs;
                        rowItems << new QStandardItem(tsStr);
                        
                        rowItems << new QStandardItem(QString::number(log.value("run_id", 0)));
                        rowItems << new QStandardItem(QString::fromStdString(log.value("component", "")));
                        rowItems << new QStandardItem(QString::fromStdString(log.value("message", "")));
                        m_model->appendRow(rowItems);
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("JSON parsing error in AuditLogs: {}", e.what());
            }
        } else {
            spdlog::error("AuditLogs API failed: {}", reply->errorString().toStdString());
            m_model->setRowCount(0);
            m_model->appendRow({new QStandardItem("Error"), new QStandardItem(reply->errorString())});
        }
        m_tableView->resizeColumnsToContents();
        reply->deleteLater();
    });
}

void AuditLogsWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Audit Logs", "audit_logs.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot open file for writing.");
        return;
    }

    QTextStream out(&file);
    // Write headers
    for (int col = 0; col < m_model->columnCount(); ++col) {
        out << "\"" << m_model->horizontalHeaderItem(col)->text() << "\"";
        if (col < m_model->columnCount() - 1) out << ",";
    }
    out << "\n";

    // Write data
    for (int row = 0; row < m_model->rowCount(); ++row) {
        for (int col = 0; col < m_model->columnCount(); ++col) {
            auto item = m_model->item(row, col);
            QString text = item ? item->text() : "";
            text.replace("\"", "\"\""); // Escape quotes
            out << "\"" << text << "\"";
            if (col < m_model->columnCount() - 1) out << ",";
        }
        out << "\n";
    }

    file.close();
    QMessageBox::information(this, "Export Successful", "Logs exported successfully to\n" + fileName);
}
