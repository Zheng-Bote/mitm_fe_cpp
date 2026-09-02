/**
 * SPDX-FileComment: DashboardWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file DashboardWidget.cpp
 * @brief DashboardWidget
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "DashboardWidget.h"
#include "ApiClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QFile>
#include <QProcessEnvironment>
#include <QDateTime>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "Config.h"
#include "schematas/admin_audit_logs_generated.h"
#include "schematas/system_logs_generated.h"
#include "schematas/job_audit_logs_generated.h"
#include "schematas/transformation_errors_generated.h"

using json = nlohmann::json;

DashboardWidget::DashboardWidget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);

    auto headerLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton("Refresh Dashboard", this);
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    auto gridLayout = new QGridLayout();
    
    // Cards / Labels for Dashboard stats
    m_healthLabel = new QLabel("System Health: Unknown", this);
    m_healthLabel->setStyleSheet("font-size: 16px; padding: 15px; background-color: #2b2b2b; border-radius: 8px; color: white;");
    m_healthLabel->setAlignment(Qt::AlignCenter);
    
    m_engineLabel = new QLabel("Engine Info: Unknown", this);
    m_engineLabel->setStyleSheet("font-size: 16px; padding: 15px; background-color: #2b2b2b; border-radius: 8px; color: white;");
    m_engineLabel->setAlignment(Qt::AlignCenter);

    m_jobsLabel = new QLabel("Total Scheduled Jobs: Unknown", this);
    m_jobsLabel->setStyleSheet("font-size: 16px; padding: 15px; background-color: #2b2b2b; border-radius: 8px; color: white;");
    m_jobsLabel->setAlignment(Qt::AlignCenter);

    m_dbInfoLabel = new QLabel("DB Info: Unknown", this);
    m_dbInfoLabel->setStyleSheet("font-size: 16px; padding: 15px; background-color: #2b2b2b; border-radius: 8px; color: white;");
    m_dbInfoLabel->setAlignment(Qt::AlignCenter);

    m_dlqCursorLabel = new QLabel("DLQ Cursors: Unknown", this);
    m_dlqCursorLabel->setStyleSheet("font-size: 16px; padding: 15px; background-color: #552222; border-radius: 8px; color: white;");
    m_dlqCursorLabel->setAlignment(Qt::AlignCenter);

    gridLayout->addWidget(m_healthLabel, 0, 0);
    gridLayout->addWidget(m_engineLabel, 0, 1);
    gridLayout->addWidget(m_dbInfoLabel, 1, 0);
    gridLayout->addWidget(m_dlqCursorLabel, 1, 1);
    gridLayout->addWidget(m_jobsLabel, 2, 0, 1, 2);

    m_adminLogsLabel = new QLabel("Admin Audit Logs: Unknown", this);
    m_adminLogsLabel->setStyleSheet("font-size: 16px; padding: 15px; background-color: #2b2b2b; border-radius: 8px; color: white;");
    m_adminLogsLabel->setAlignment(Qt::AlignCenter);

    m_systemLogsLabel = new QLabel("System Logs: Unknown", this);
    m_systemLogsLabel->setStyleSheet("font-size: 16px; padding: 15px; background-color: #2b2b2b; border-radius: 8px; color: white;");
    m_systemLogsLabel->setAlignment(Qt::AlignCenter);

    m_jobLogsLabel = new QLabel("Job Audit Logs: Unknown", this);
    m_jobLogsLabel->setStyleSheet("font-size: 16px; padding: 15px; background-color: #2b2b2b; border-radius: 8px; color: white;");
    m_jobLogsLabel->setAlignment(Qt::AlignCenter);

    m_transformErrorsLabel = new QLabel("Transformation Errors: Unknown", this);
    m_transformErrorsLabel->setStyleSheet("font-size: 16px; padding: 15px; background-color: #552222; border-radius: 8px; color: white;");
    m_transformErrorsLabel->setAlignment(Qt::AlignCenter);

    gridLayout->addWidget(m_adminLogsLabel, 3, 0, 1, 2);
    gridLayout->addWidget(m_systemLogsLabel, 4, 0, 1, 2);
    gridLayout->addWidget(m_jobLogsLabel, 5, 0, 1, 2);
    gridLayout->addWidget(m_transformErrorsLabel, 6, 0, 1, 2);

    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch(); // Push elements to the top

    connect(m_refreshButton, &QPushButton::clicked, this, &DashboardWidget::onRefreshClicked);
}

void DashboardWidget::onRefreshClicked() {
    spdlog::info("Refreshing Dashboard Widgets...");
    m_healthLabel->setText("System Health: Loading...");
    m_engineLabel->setText("Engine Info: Loading...");
    m_jobsLabel->setText("Total Scheduled Jobs: Loading...");
    m_adminLogsLabel->setText("Admin Audit Logs: Loading...");
    m_systemLogsLabel->setText("System Logs: Loading...");
    m_jobLogsLabel->setText("Job Audit Logs: Loading...");
    m_transformErrorsLabel->setText("Transformation Errors: Loading...");
    m_dbInfoLabel->setText("DB Info: Loading...");
    m_dlqCursorLabel->setText("DLQ Cursors: Loading...");

    fetchHealth();
    fetchInfo();
    fetchJobs();
    fetchAdminLogsStats();
    fetchSystemLogsStats();
    fetchJobLogsStats();
    fetchTransformErrorsStats();
    fetchDashboardStats();
}

void DashboardWidget::fetchHealth() {
    mitm::api::ApiClient::instance().get("/health",
        [this](const QByteArray& data, QNetworkReply* reply) {
            m_healthLabel->setText("System Health: Healthy 🟢");
        },
        [this](int statusCode, const QString& errorString) {
            m_healthLabel->setText("System Health: Offline 🔴\n(" + errorString + ")");
        }
    );
}

void DashboardWidget::fetchInfo() {
    mitm::api::ApiClient::instance().get("/info",
        [this](const QByteArray& data, QNetworkReply* reply) {
            try {
                json j = json::parse(data.toStdString());
                QString text = QString("Engine: %1\nVersion: %2")
                    .arg(QString::fromStdString(j.value("name", "Unknown")))
                    .arg(QString::fromStdString(j.value("version", "Unknown")));
                m_engineLabel->setText(text);
            } catch (...) {
                m_engineLabel->setText("Engine Info: Parse Error");
            }
        },
        [this](int statusCode, const QString& errorString) {
            m_engineLabel->setText("Engine Info: N/A 🔴");
        }
    );
}

void DashboardWidget::fetchJobs() {
    mitm::api::ApiClient::instance().get("/admin/jobs",
        [this](const QByteArray& data, QNetworkReply* reply) {
            try {
                json j = json::parse(data.toStdString());
                if (j.is_array()) {
                    m_jobsLabel->setText(QString("Total Scheduled Jobs: %1 📦").arg(j.size()));
                } else {
                    m_jobsLabel->setText("Total Scheduled Jobs: 0");
                }
            } catch (...) {
                m_jobsLabel->setText("Jobs: Parse Error");
            }
        },
        [this](int statusCode, const QString& errorString) {
            m_jobsLabel->setText("Jobs: Auth Error / Offline 🔴");
        }
    );
}

void DashboardWidget::fetchAdminLogsStats() {
    mitm::api::ApiClient::instance().get("/admin/logs/admin-audit_bin",
        [this](const QByteArray& data, QNetworkReply* reply) {
            try {
                flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
                if (!schematas::VerifyAdminAuditLogListBuffer(verifier)) {
                    m_adminLogsLabel->setText("Admin Audit Logs: Parse Error");
                } else {
                    auto list = schematas::GetAdminAuditLogList(data.constData());
                    if (list && list->logs()) {
                        auto arr = list->logs();
                        std::string oldestTs = "";
                        for (int i = 0; i < arr->size(); ++i) {
                            auto log = arr->Get(i);
                            if (!log || !log->ts()) continue;
                            std::string ts = log->ts()->c_str();
                            if (oldestTs.empty() || ts < oldestTs) oldestTs = ts;
                        }
                        QString displayTs = "N/A";
                        if (!oldestTs.empty()) {
                            QDateTime dt = QDateTime::fromString(QString::fromStdString(oldestTs), Qt::ISODate);
                            displayTs = dt.isValid() ? dt.toString("yyyy-MM-dd HH:mm:ss") : QString::fromStdString(oldestTs);
                        }
                        m_adminLogsLabel->setText(QString("Admin Audit Logs: %1 📋 (Oldest: %2)").arg(arr->size()).arg(displayTs));
                    } else {
                        m_adminLogsLabel->setText("Admin Audit Logs: 0");
                    }
                }
            } catch (...) {
                m_adminLogsLabel->setText("Admin Audit Logs: Parse Error");
            }
        },
        [this](int statusCode, const QString& errorString) {
            m_adminLogsLabel->setText("Admin Audit Logs: Error 🔴");
        }
    );
}

void DashboardWidget::fetchSystemLogsStats() {
    mitm::api::ApiClient::instance().get("/admin/logs/system_bin",
        [this](const QByteArray& data, QNetworkReply* reply) {
            try {
                flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
                if (!schematas::VerifySystemLogListBuffer(verifier)) {
                    m_systemLogsLabel->setText("System Logs: Parse Error");
                } else {
                    auto list = schematas::GetSystemLogList(data.constData());
                    if (list && list->logs()) {
                        auto arr = list->logs();
                        std::string oldestTs = "";
                        for (int i = 0; i < arr->size(); ++i) {
                            auto log = arr->Get(i);
                            if (!log || !log->ts()) continue;
                            std::string ts = log->ts()->c_str();
                            if (oldestTs.empty() || ts < oldestTs) oldestTs = ts;
                        }
                        QString displayTs = "N/A";
                        if (!oldestTs.empty()) {
                            QDateTime dt = QDateTime::fromString(QString::fromStdString(oldestTs), Qt::ISODate);
                            displayTs = dt.isValid() ? dt.toString("yyyy-MM-dd HH:mm:ss") : QString::fromStdString(oldestTs);
                        }
                        m_systemLogsLabel->setText(QString("System Logs: %1 📋 (Oldest: %2)").arg(arr->size()).arg(displayTs));
                    } else {
                        m_systemLogsLabel->setText("System Logs: 0");
                    }
                }
            } catch (...) {
                m_systemLogsLabel->setText("System Logs: Parse Error");
            }
        },
        [this](int statusCode, const QString& errorString) {
            m_systemLogsLabel->setText("System Logs: Error 🔴");
        }
    );
}

void DashboardWidget::fetchJobLogsStats() {
    mitm::api::ApiClient::instance().get("/admin/logs/job-audit_bin",
        [this](const QByteArray& data, QNetworkReply* reply) {
            try {
                flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
                if (!schematas::VerifyJobAuditLogListBuffer(verifier)) {
                    m_jobLogsLabel->setText("Job Audit Logs: Parse Error");
                } else {
                    auto list = schematas::GetJobAuditLogList(data.constData());
                    if (list && list->logs()) {
                        auto arr = list->logs();
                        std::string oldestTs = "";
                        for (int i = 0; i < arr->size(); ++i) {
                            auto log = arr->Get(i);
                            if (!log || !log->ts()) continue;
                            std::string ts = log->ts()->c_str();
                            if (oldestTs.empty() || ts < oldestTs) oldestTs = ts;
                        }
                        QString displayTs = "N/A";
                        if (!oldestTs.empty()) {
                            QDateTime dt = QDateTime::fromString(QString::fromStdString(oldestTs), Qt::ISODate);
                            displayTs = dt.isValid() ? dt.toString("yyyy-MM-dd HH:mm:ss") : QString::fromStdString(oldestTs);
                        }
                        m_jobLogsLabel->setText(QString("Job Audit Logs: %1 📋 (Oldest: %2)").arg(arr->size()).arg(displayTs));
                    } else {
                        m_jobLogsLabel->setText("Job Audit Logs: 0");
                    }
                }
            } catch (...) {
                m_jobLogsLabel->setText("Job Audit Logs: Parse Error");
            }
        },
        [this](int statusCode, const QString& errorString) {
            m_jobLogsLabel->setText("Job Audit Logs: Error 🔴");
        }
    );
}

void DashboardWidget::fetchTransformErrorsStats() {
    mitm::api::ApiClient::instance().get("/admin/transformation/errors_bin",
        [this](const QByteArray& data, QNetworkReply* reply) {
            try {
                flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
                if (!schematas::VerifyTransformationErrorListBuffer(verifier)) {
                    m_transformErrorsLabel->setText("Transformation Errors: Parse Error");
                } else {
                    auto list = schematas::GetTransformationErrorList(data.constData());
                    if (list && list->errors()) {
                        auto arr = list->errors();
                        std::string oldestTs = "";
                        for (int i = 0; i < arr->size(); ++i) {
                            auto log = arr->Get(i);
                            if (!log || !log->created_at()) continue;
                            std::string ts = log->created_at()->c_str();
                            if (oldestTs.empty() || ts < oldestTs) oldestTs = ts;
                        }
                        QString displayTs = "N/A";
                        if (!oldestTs.empty()) {
                            QDateTime dt = QDateTime::fromString(QString::fromStdString(oldestTs), Qt::ISODate);
                            displayTs = dt.isValid() ? dt.toString("yyyy-MM-dd HH:mm:ss") : QString::fromStdString(oldestTs);
                        }
                        m_transformErrorsLabel->setText(QString("Transformation Errors: %1 📋 (Oldest: %2)").arg(arr->size()).arg(displayTs));
                    } else {
                        m_transformErrorsLabel->setText("Transformation Errors: 0");
                    }
                }
            } catch (...) {
                m_transformErrorsLabel->setText("Transformation Errors: Parse Error");
            }
        },
        [this](int statusCode, const QString& errorString) {
            m_transformErrorsLabel->setText("Transformation Errors: Error 🔴");
        }
    );
}


void DashboardWidget::fetchDashboardStats() {
    mitm::api::ApiClient::instance().get("/admin/dashboard/stats",
        [this](const QByteArray& data, QNetworkReply* reply) {
            try {
                auto j = json::parse(data.toStdString());
                QString dbName = QString::fromStdString(j.value("db_name", "Unknown"));
                QString dbVersion = QString::fromStdString(j.value("db_version", "Unknown"));
                QString dbSize = QString::fromStdString(j.value("db_size", "Unknown"));
                int dlqCount = j.value("dlq_count", 0);
                
                m_dbInfoLabel->setText(QString("DB: %1 %2\nSize: %3").arg(dbName).arg(dbVersion).arg(dbSize));
                m_dlqCursorLabel->setText(QString("DLQ Cursors: %1").arg(dlqCount));
            } catch (...) {
                m_dbInfoLabel->setText("DB Info: Parse Error");
                m_dlqCursorLabel->setText("DLQ Cursors: Parse Error");
            }
        },
        [this](int statusCode, const QString& errorString) {
            m_dbInfoLabel->setText("DB Info: Error 🔴");
            m_dlqCursorLabel->setText("DLQ Cursors: Error 🔴");
        }
    );
}
