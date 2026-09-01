/**
 * SPDX-FileComment: SystemLogsWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file SystemLogsWidget.cpp
 * @brief SystemLogsWidget
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "SystemLogsWidget.h"
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
#include "Config.h"
#include "schematas/system_logs_generated.h"

#include "ApiClient.h"

SystemLogsWidget::SystemLogsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);

    auto headerLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton("Refresh System Logs", this);
    m_exportButton = new QPushButton("Export CSV", this);
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_exportButton);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    m_tableView = new QTableView(this);
    m_model = new QStandardItemModel(0, 5, this);
    QTimeZone tz = QTimeZone::systemTimeZone();
    QString tsHeader = QString("Timestamp (%1)").arg(QString(tz.id()));
    m_model->setHorizontalHeaderLabels({"ID", tsHeader, "Level", "Component", "Message"});
    
    m_tableView->setModel(m_model);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSortingEnabled(true);
    
    mainLayout->addWidget(m_tableView);

    connect(m_refreshButton, &QPushButton::clicked, this, &SystemLogsWidget::onRefresh);
    connect(m_exportButton, &QPushButton::clicked, this, &SystemLogsWidget::onExportCsv);
}

void SystemLogsWidget::onRefresh() {
    m_refreshButton->setEnabled(false);
    
    mitm::api::ApiClient::instance().get("/admin/logs/system_bin",
        [this](const QByteArray& data, QNetworkReply*) {
            m_refreshButton->setEnabled(true);
            m_model->setRowCount(0);
            try {
                flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
                if (!schematas::VerifySystemLogListBuffer(verifier)) {
                    spdlog::error("Invalid SystemLogs FlatBuffer data received");
                } else {
                    auto list = schematas::GetSystemLogList(data.constData());
                    if (list && list->logs()) {
                        auto arr = list->logs();
                        for (int i = 0; i < arr->size(); ++i) {
                            auto log = arr->Get(i);
                            if (!log) continue;
                            QList<QStandardItem*> rowItems;
                            rowItems << new QStandardItem(QString::number(log->id()));
                            
                            QString rawTs = log->ts() ? QString::fromUtf8(log->ts()->c_str()) : "";
                            QDateTime dt = QDateTime::fromString(rawTs, Qt::ISODate);
                            QString tsStr = dt.isValid() ? dt.toLocalTime().toString("yyyy-MM-dd HH:mm:ss") : rawTs;
                            rowItems << new QStandardItem(tsStr);
                            
                            QString level = log->level() ? QString::fromUtf8(log->level()->c_str()) : "";
                            QString component = log->component() ? QString::fromUtf8(log->component()->c_str()) : "";
                            QString message = log->message() ? QString::fromUtf8(log->message()->c_str()) : "";

                            rowItems << new QStandardItem(level);
                            rowItems << new QStandardItem(component);
                            rowItems << new QStandardItem(message);
                            m_model->appendRow(rowItems);
                        }
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("FlatBuffers parsing error in SystemLogs: {}", e.what());
            }
            m_tableView->resizeColumnsToContents();
        },
        [this](int /*statusCode*/, const QString& errorString) {
            m_refreshButton->setEnabled(true);
            spdlog::error("SystemLogs API failed: {}", errorString.toStdString());
            m_model->setRowCount(0);
            m_model->appendRow({new QStandardItem("Error"), new QStandardItem(errorString)});
            m_tableView->resizeColumnsToContents();
        }
    );
}

void SystemLogsWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export System Logs", "system_logs.csv", "CSV Files (*.csv)");
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
