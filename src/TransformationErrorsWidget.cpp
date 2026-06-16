/**
 * SPDX-FileComment: TransformationErrorsWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TransformationErrorsWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "Config.h"

using json = nlohmann::json;

TransformationErrorsWidget::TransformationErrorsWidget(QWidget *parent)
    : QWidget(parent), m_networkManager(new QNetworkAccessManager(this))
{
    auto mainLayout = new QVBoxLayout(this);

    auto headerLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton("Refresh Transformation Errors", this);
    m_exportButton = new QPushButton("Export CSV", this);
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_exportButton);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    m_tableView = new QTableView(this);
    m_model = new QStandardItemModel(0, 7, this);
    m_model->setHorizontalHeaderLabels({"ID", "Raw Ingestion ID", "Topic", "Failed Field", "Rule Name", "Error Message", "Created At"});
    
    m_tableView->setModel(m_model);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSortingEnabled(true);
    
    mainLayout->addWidget(m_tableView);

    connect(m_refreshButton, &QPushButton::clicked, this, &TransformationErrorsWidget::onRefresh);
    connect(m_exportButton, &QPushButton::clicked, this, &TransformationErrorsWidget::onExportCsv);

    onRefresh(); // Initial load
}

QString TransformationErrorsWidget::getAuthHeader() {
    return mitm::config::ConfigManager::GetInstance().GetAuthHeader();
}

void TransformationErrorsWidget::onRefresh() {
    m_refreshButton->setEnabled(false);
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/transformation/errors");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_refreshButton->setEnabled(true);

        if (reply->error() == QNetworkReply::NoError) {
            m_tableView->setSortingEnabled(false);
            m_model->setRowCount(0);
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    for (const auto& log : data) {
                        QList<QStandardItem*> rowItems;
                        rowItems << new QStandardItem(QString::fromStdString(log.value("id", "")));
                        rowItems << new QStandardItem(QString::fromStdString(log.value("raw_ingestion_id", "")));
                        rowItems << new QStandardItem(QString::fromStdString(log.value("topic", "")));
                        rowItems << new QStandardItem(QString::fromStdString(log.value("failed_field", "")));
                        rowItems << new QStandardItem(QString::fromStdString(log.value("rule_name", "")));
                        rowItems << new QStandardItem(QString::fromStdString(log.value("error_message", "")));
                        
                        QString ts = QString::fromStdString(log.value("created_at", ""));
                        QDateTime dt = QDateTime::fromString(ts, Qt::ISODate);
                        if (dt.isValid()) {
                            ts = dt.toString("yyyy-MM-dd HH:mm:ss");
                        }
                        rowItems << new QStandardItem(ts);
                        
                        m_model->appendRow(rowItems);
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("JSON parsing error in TransformationErrors: {}", e.what());
            }
            m_tableView->setSortingEnabled(true);
        } else {
            spdlog::error("TransformationErrors API failed: {}", reply->errorString().toStdString());
            m_tableView->setSortingEnabled(false);
            m_model->setRowCount(0);
            m_model->appendRow({new QStandardItem("Error"), new QStandardItem(reply->errorString())});
            m_tableView->setSortingEnabled(true);
        }
        m_tableView->resizeColumnsToContents();
        reply->deleteLater();
    });
}

void TransformationErrorsWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Transformation Errors", "transformation_errors.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot open file for writing.");
        return;
    }

    QTextStream out(&file);
    for (int col = 0; col < m_model->columnCount(); ++col) {
        out << "\"" << m_model->horizontalHeaderItem(col)->text() << "\"";
        if (col < m_model->columnCount() - 1) out << ",";
    }
    out << "\n";

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
