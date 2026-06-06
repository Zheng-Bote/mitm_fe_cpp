#include "AuditLogsWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QProcessEnvironment>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

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
    m_model = new QStandardItemModel(0, 4, this);
    m_model->setHorizontalHeaderLabels({"ID", "Timestamp", "Run ID", "Message"});
    
    m_tableView->setModel(m_model);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSortingEnabled(true);
    
    mainLayout->addWidget(m_tableView);

    connect(m_refreshButton, &QPushButton::clicked, this, &AuditLogsWidget::onRefresh);
    connect(m_exportButton, &QPushButton::clicked, this, &AuditLogsWidget::onExportCsv);
}

QString AuditLogsWidget::getAuthHeader() {
    auto env = QProcessEnvironment::systemEnvironment();
    QString osUser = env.value("USER", env.value("USERNAME", "unknown"));
    QString token = "helo_linux"; 
    
    QFile configFile("../../../../data/config.json");
    if (!configFile.exists()) configFile.setFileName("../../data/config.json");
    
    if (configFile.open(QIODevice::ReadOnly)) {
        try {
            json configJson = json::parse(configFile.readAll().toStdString());
            if (configJson.contains("admins") && configJson["admins"].is_array()) {
                for (const auto& admin : configJson["admins"]) {
                    if (admin.value("username", "") == osUser.toStdString()) {
                        token = QString::fromStdString(admin.value("token", ""));
                        break;
                    }
                }
            }
        } catch (...) {}
    }
    
    QString credentials = osUser + ":" + token;
    return "Basic " + credentials.toLocal8Bit().toBase64();
}

void AuditLogsWidget::onRefresh() {
    m_refreshButton->setEnabled(false);
    
    QUrl url("http://localhost:8080/admin/logs/job-audit");
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
                        rowItems << new QStandardItem(QString::fromStdString(log.value("ts", "")));
                        rowItems << new QStandardItem(QString::number(log.value("run_id", 0)));
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
