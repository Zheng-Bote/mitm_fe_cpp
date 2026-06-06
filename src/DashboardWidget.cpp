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
    auto env = QProcessEnvironment::systemEnvironment();
    QString osUser = env.value("USER", env.value("USERNAME", "unknown"));
    QString token = "helo_linux"; // Fallback
    
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
    QNetworkRequest request(QUrl("http://localhost:8080/health"));
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
    QNetworkRequest request(QUrl("http://localhost:8080/info"));
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
    QNetworkRequest request(QUrl("http://localhost:8080/admin/jobs"));
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
