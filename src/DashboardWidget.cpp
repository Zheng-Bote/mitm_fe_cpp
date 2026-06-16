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

using json = nlohmann::json;

DashboardWidget::DashboardWidget(QWidget *parent)
    : QWidget(parent), m_networkManager(new QNetworkAccessManager(this))
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

    gridLayout->addWidget(m_healthLabel, 0, 0);
    gridLayout->addWidget(m_engineLabel, 0, 1);
    gridLayout->addWidget(m_jobsLabel, 1, 0, 1, 2);

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

    gridLayout->addWidget(m_adminLogsLabel, 2, 0, 1, 2);
    gridLayout->addWidget(m_systemLogsLabel, 3, 0, 1, 2);
    gridLayout->addWidget(m_jobLogsLabel, 4, 0, 1, 2);
    gridLayout->addWidget(m_transformErrorsLabel, 5, 0, 1, 2);

    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch(); // Push elements to the top

    connect(m_refreshButton, &QPushButton::clicked, this, &DashboardWidget::onRefreshClicked);
}

QString DashboardWidget::getAuthHeader() {
    return mitm::config::ConfigManager::GetInstance().GetAuthHeader();
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

    fetchHealth();
    fetchInfo();
    fetchJobs();
    fetchAdminLogsStats();
    fetchSystemLogsStats();
    fetchJobLogsStats();
    fetchTransformErrorsStats();
}

void DashboardWidget::fetchHealth() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QNetworkRequest request(QUrl(host + "/health"));
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            m_healthLabel->setText("System Health: Healthy 🟢");
        } else {
            m_healthLabel->setText("System Health: Offline 🔴\n(" + reply->errorString() + ")");
        }
        reply->deleteLater();
    });
}

void DashboardWidget::fetchInfo() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QNetworkRequest request(QUrl(host + "/info"));
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            try {
                json data = json::parse(reply->readAll().toStdString());
                QString text = QString("Engine: %1\nVersion: %2")
                    .arg(QString::fromStdString(data.value("name", "Unknown")))
                    .arg(QString::fromStdString(data.value("version", "Unknown")));
                m_engineLabel->setText(text);
            } catch (...) {
                m_engineLabel->setText("Engine Info: Parse Error");
            }
        } else {
            m_engineLabel->setText("Engine Info: N/A 🔴");
        }
        reply->deleteLater();
    });
}

void DashboardWidget::fetchJobs() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QNetworkRequest request(QUrl(host + "/admin/jobs"));
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    m_jobsLabel->setText(QString("Total Scheduled Jobs: %1 📦").arg(data.size()));
                } else {
                    m_jobsLabel->setText("Total Scheduled Jobs: 0");
                }
            } catch (...) {
                m_jobsLabel->setText("Jobs: Parse Error");
            }
        } else {
            m_jobsLabel->setText("Jobs: Auth Error / Offline 🔴");
        }
        reply->deleteLater();
    });
}

void DashboardWidget::fetchAdminLogsStats() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QNetworkRequest request(QUrl(host + "/admin/logs/admin-audit"));
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    std::string oldestTs = "";
                    for (const auto& log : data) {
                        std::string ts = log.value("ts", "");
                        if (oldestTs.empty() || ts < oldestTs) oldestTs = ts;
                    }
                    QString displayTs = "N/A";
                    if (!oldestTs.empty()) {
                        QDateTime dt = QDateTime::fromString(QString::fromStdString(oldestTs), Qt::ISODate);
                        displayTs = dt.isValid() ? dt.toString("yyyy-MM-dd HH:mm:ss") : QString::fromStdString(oldestTs);
                    }
                    m_adminLogsLabel->setText(QString("Admin Audit Logs: %1 \U0001F4CB (Oldest: %2)").arg(data.size()).arg(displayTs));
                } else {
                    m_adminLogsLabel->setText("Admin Audit Logs: 0");
                }
            } catch (...) {
                m_adminLogsLabel->setText("Admin Audit Logs: Parse Error");
            }
        } else {
            m_adminLogsLabel->setText("Admin Audit Logs: Error \U0001F534");
        }
        reply->deleteLater();
    });
}

void DashboardWidget::fetchSystemLogsStats() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QNetworkRequest request(QUrl(host + "/admin/logs/system"));
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    std::string oldestTs = "";
                    for (const auto& log : data) {
                        std::string ts = log.value("ts", "");
                        if (oldestTs.empty() || ts < oldestTs) oldestTs = ts;
                    }
                    QString displayTs = "N/A";
                    if (!oldestTs.empty()) {
                        QDateTime dt = QDateTime::fromString(QString::fromStdString(oldestTs), Qt::ISODate);
                        displayTs = dt.isValid() ? dt.toString("yyyy-MM-dd HH:mm:ss") : QString::fromStdString(oldestTs);
                    }
                    m_systemLogsLabel->setText(QString("System Logs: %1 \U0001F4CB (Oldest: %2)").arg(data.size()).arg(displayTs));
                } else {
                    m_systemLogsLabel->setText("System Logs: 0");
                }
            } catch (...) {
                m_systemLogsLabel->setText("System Logs: Parse Error");
            }
        } else {
            m_systemLogsLabel->setText("System Logs: Error \U0001F534");
        }
        reply->deleteLater();
    });
}

void DashboardWidget::fetchJobLogsStats() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QNetworkRequest request(QUrl(host + "/admin/logs/job-audit"));
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    std::string oldestTs = "";
                    for (const auto& log : data) {
                        std::string ts = log.value("ts", "");
                        if (oldestTs.empty() || ts < oldestTs) oldestTs = ts;
                    }
                    QString displayTs = "N/A";
                    if (!oldestTs.empty()) {
                        QDateTime dt = QDateTime::fromString(QString::fromStdString(oldestTs), Qt::ISODate);
                        displayTs = dt.isValid() ? dt.toString("yyyy-MM-dd HH:mm:ss") : QString::fromStdString(oldestTs);
                    }
                    m_jobLogsLabel->setText(QString("Job Audit Logs: %1 \U0001F4CB (Oldest: %2)").arg(data.size()).arg(displayTs));
                } else {
                    m_jobLogsLabel->setText("Job Audit Logs: 0");
                }
            } catch (...) {
                m_jobLogsLabel->setText("Job Audit Logs: Parse Error");
            }
        } else {
            m_jobLogsLabel->setText("Job Audit Logs: Error \U0001F534");
        }
        reply->deleteLater();
    });
}

void DashboardWidget::fetchTransformErrorsStats() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QNetworkRequest request(QUrl(host + "/admin/transformation/errors"));
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    std::string oldestTs = "";
                    for (const auto& log : data) {
                        std::string ts = log.value("created_at", "");
                        if (oldestTs.empty() || ts < oldestTs) oldestTs = ts;
                    }
                    QString displayTs = "N/A";
                    if (!oldestTs.empty()) {
                        QDateTime dt = QDateTime::fromString(QString::fromStdString(oldestTs), Qt::ISODate);
                        displayTs = dt.isValid() ? dt.toString("yyyy-MM-dd HH:mm:ss") : QString::fromStdString(oldestTs);
                    }
                    m_transformErrorsLabel->setText(QString("Transformation Errors: %1 \U0001F4CB (Oldest: %2)").arg(data.size()).arg(displayTs));
                } else {
                    m_transformErrorsLabel->setText("Transformation Errors: 0");
                }
            } catch (...) {
                m_transformErrorsLabel->setText("Transformation Errors: Parse Error");
            }
        } else {
            m_transformErrorsLabel->setText("Transformation Errors: Error \U0001F534");
        }
        reply->deleteLater();
    });
}

