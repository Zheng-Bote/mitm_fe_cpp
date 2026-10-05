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
#include <QTimer>
#include <QSettings>
#include <QCheckBox>
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
    m_autoRefreshCheckbox = new QCheckBox("Auto-Refresh (5s)", this);
    
    QSettings settings;
    m_autoRefreshCheckbox->setChecked(settings.value("AutoRefresh/Dashboard", false).toBool());
    
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_autoRefreshCheckbox);
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

    gridLayout->addWidget(m_healthLabel, 0, 0, 1, 2);
    gridLayout->addWidget(m_engineLabel, 1, 0);
    gridLayout->addWidget(m_dbInfoLabel, 1, 1);
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
    gridLayout->addWidget(m_dlqCursorLabel, 7, 0, 1, 2);

    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch(); // Push elements to the top

    connect(m_refreshButton, &QPushButton::clicked, this, &DashboardWidget::onRefreshClicked);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &DashboardWidget::refreshData);
    connect(m_autoRefreshCheckbox, &QCheckBox::toggled, this, &DashboardWidget::onAutoRefreshToggled);
    
    if (m_autoRefreshCheckbox->isChecked()) {
        m_timer->start(5000);
    }
}

void DashboardWidget::onAutoRefreshToggled(bool checked) {
    QSettings settings;
    settings.setValue("AutoRefresh/Dashboard", checked);
    if (checked) {
        m_timer->start(5000);
        refreshData();
    } else {
        m_timer->stop();
    }
}

void DashboardWidget::onRefreshClicked() {
    spdlog::info("Manually refreshing Dashboard Widgets...");
    m_healthLabel->setText("System Health: Loading...");
    m_engineLabel->setText("Engine Info: Loading...");
    m_jobsLabel->setText("Total Scheduled Jobs: Loading...");
    m_adminLogsLabel->setText("Admin Audit Logs: Loading...");
    m_systemLogsLabel->setText("System Logs: Loading...");
    m_jobLogsLabel->setText("Job Audit Logs: Loading...");
    m_transformErrorsLabel->setText("Transformation Errors: Loading...");
    m_dbInfoLabel->setText("DB Info: Loading...");
    m_dlqCursorLabel->setText("DLQ Cursors: Loading...");

    refreshData();
}

void DashboardWidget::refreshData() {
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
                QString text = QString("Engine: %1")
                    .arg(QString::fromStdString(j.value("name", "Unknown")));
                
                if (j.contains("core_components") && j["core_components"].is_array()) {
                    for (const auto& comp : j["core_components"]) {
                        QString compName = QString::fromStdString(comp.value("name", "Unknown"));
                        QString compVer = QString::fromStdString(comp.value("version", "Unknown"));
                        text += QString("\n%1 v%2").arg(compName).arg(compVer);
                    }
                }
                
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
    mitm::api::ApiClient::instance().get("/api/v1/jobs",
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






void DashboardWidget::fetchDashboardStats() {
    mitm::api::ApiClient::instance().get("/api/v1/system/dashboard",
        [this](const QByteArray& data, QNetworkReply* reply) {
            try {
                auto j = json::parse(data.toStdString());
                
                QString dbName = QString::fromStdString(j.value("db_name", "Unknown"));
                QString dbVersion = QString::fromStdString(j.value("db_version", "Unknown"));
                QString dbSize = QString::fromStdString(j.value("db_size", "Unknown"));
                
                auto stats = j.value("stats", json::object());
                
                auto parseMetric = [](const json& m, const QString& prefix, const QString& icon) -> QString {
                    int count = m.value("count", 0);
                    QString oldest = "N/A";
                    if (m.contains("oldest") && !m["oldest"].is_null()) {
                        oldest = QString::fromStdString(m.value("oldest", ""));
                        QDateTime dt = QDateTime::fromString(oldest, Qt::ISODate);
                        if (dt.isValid()) {
                            oldest = dt.toString("yyyy-MM-dd HH:mm:ss");
                        }
                    }
                    return QString("%1: %2 %3 (Oldest: %4)").arg(prefix).arg(count).arg(icon).arg(oldest);
                };
                
                m_dbInfoLabel->setText(QString("DB: %1 %2<br/>Size: %3").arg(dbName).arg(dbVersion).arg(dbSize));
                
                m_dlqCursorLabel->setText(parseMetric(stats.value("dlq", json::object()), "DLQ Cursors", "📦"));
                m_transformErrorsLabel->setText(parseMetric(stats.value("transformation_errors", json::object()), "Transformation Errors", "🔴"));
                m_systemLogsLabel->setText(parseMetric(stats.value("system_logs", json::object()), "System Logs", "📋"));
                m_adminLogsLabel->setText(parseMetric(stats.value("admin_audit_logs", json::object()), "Admin Audit Logs", "🛡️"));
                m_jobLogsLabel->setText(parseMetric(stats.value("job_audit_logs", json::object()), "Job Audit Logs", "📋"));
                
                int jobsCount = stats.value("total_scheduled_jobs", 0);
                m_jobsLabel->setText(QString("Total Scheduled Jobs: %1").arg(jobsCount));
                
                fetchHealth();
                fetchInfo();
                
            } catch (const std::exception& e) {
                m_dbInfoLabel->setText(QString("DB Info: Parse Error (%1)").arg(e.what()));
            }
        },
        [this](int statusCode, const QString& errorString) {
            m_dbInfoLabel->setText("DB Info: Error 🔴");
        }
    );
}
