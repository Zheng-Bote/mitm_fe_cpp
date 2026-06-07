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

    fetchHealth();
    fetchInfo();
    fetchJobs();
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
