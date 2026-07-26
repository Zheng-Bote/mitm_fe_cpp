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
#include "Config.h"
#include "schematas/transformation_errors_generated.h"

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
    m_model->setHorizontalHeaderLabels({"ID", "Correlation ID", "Topic", "Failed Field", "Rule Name", "Error Message", "Created At"});
    
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
    QUrl url(host + "/admin/transformation/errors_bin");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_refreshButton->setEnabled(true);

        if (reply->error() == QNetworkReply::NoError) {
            m_tableView->setSortingEnabled(false);
            m_model->setRowCount(0);
            try {
                QByteArray data = reply->readAll();
                flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
                if (!schematas::VerifyTransformationErrorListBuffer(verifier)) {
                    spdlog::error("Invalid TransformationErrors FlatBuffer data received");
                } else {
                    auto list = schematas::GetTransformationErrorList(data.constData());
                    if (list && list->errors()) {
                        auto arr = list->errors();
                        for (int i = 0; i < arr->size(); ++i) {
                            auto log = arr->Get(i);
                            if (!log) continue;
                            QList<QStandardItem*> rowItems;
                            rowItems << new QStandardItem(log->id() ? QString::fromUtf8(log->id()->c_str()) : "");
                            rowItems << new QStandardItem(log->correlation_id() ? QString::fromUtf8(log->correlation_id()->c_str()) : "");
                            rowItems << new QStandardItem(log->topic() ? QString::fromUtf8(log->topic()->c_str()) : "");
                            rowItems << new QStandardItem(log->failed_field() ? QString::fromUtf8(log->failed_field()->c_str()) : "");
                            rowItems << new QStandardItem(log->rule_name() ? QString::fromUtf8(log->rule_name()->c_str()) : "");
                            rowItems << new QStandardItem(log->error_message() ? QString::fromUtf8(log->error_message()->c_str()) : "");
                            
                            QString ts = log->created_at() ? QString::fromUtf8(log->created_at()->c_str()) : "";
                            QDateTime dt = QDateTime::fromString(ts, Qt::ISODate);
                            if (dt.isValid()) {
                                ts = dt.toString("yyyy-MM-dd HH:mm:ss");
                            }
                            rowItems << new QStandardItem(ts);
                            
                            m_model->appendRow(rowItems);
                        }
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("FlatBuffers parsing error in TransformationErrors: {}", e.what());
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
