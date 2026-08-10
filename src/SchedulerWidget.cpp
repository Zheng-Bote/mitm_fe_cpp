/**
 * SPDX-FileComment: SchedulerWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file SchedulerWidget.cpp
 * @brief SchedulerWidget
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "SchedulerWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QNetworkRequest>
#include <QUrl>
#include <QFile>
#include <QProcessEnvironment>
#include <QProcessEnvironment>
#include <QMessageBox>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <QTimeZone>
#include <QDateTime>
#include "Config.h"
#include "JobEditorDialog.h"

using json = nlohmann::json;

SchedulerWidget::SchedulerWidget(QWidget *parent)
    : QWidget(parent), m_networkManager(new QNetworkAccessManager(this))
{
    auto mainLayout = new QVBoxLayout(this);

    auto headerLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton("Refresh Jobs", this);
    m_addButton = new QPushButton("+ Add Job", this);
    m_editButton = new QPushButton("Edit Selected", this);
    m_deleteButton = new QPushButton("Delete Selected", this);
    m_stopButton = new QPushButton("⏹ Stop Selected", this);
    m_executeButton = new QPushButton("▶ Execute Selected", this);
    
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_addButton);
    headerLayout->addWidget(m_editButton);
    headerLayout->addWidget(m_deleteButton);
    headerLayout->addWidget(m_stopButton);
    headerLayout->addWidget(m_executeButton);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    m_tableView = new QTableView(this);
    m_model = new QStandardItemModel(0, 7, this);
    QString tzName = QTimeZone::systemTimeZoneId();
    m_model->setHorizontalHeaderLabels({"ID", "Name", "Command", "Cron Expr", "Status", QString("Next Run (%1)").arg(tzName), "Active State"});
    
    m_tableView->setModel(m_model);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSortingEnabled(true);
    
    mainLayout->addWidget(m_tableView);

    connect(m_refreshButton, &QPushButton::clicked, this, &SchedulerWidget::onRefreshClicked);
    connect(m_addButton, &QPushButton::clicked, this, &SchedulerWidget::onAddJob);
    connect(m_editButton, &QPushButton::clicked, this, &SchedulerWidget::onEditJob);
    connect(m_deleteButton, &QPushButton::clicked, this, &SchedulerWidget::onDeleteJob);
    connect(m_stopButton, &QPushButton::clicked, this, &SchedulerWidget::onStopJob);
    connect(m_executeButton, &QPushButton::clicked, this, &SchedulerWidget::onExecuteJob);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &SchedulerWidget::onApiResponse);
}

QString SchedulerWidget::getAuthHeader() {
    return mitm::config::ConfigManager::GetInstance().GetAuthHeader();
}

void SchedulerWidget::onRefreshClicked() {
    m_refreshButton->setEnabled(false);
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/jobs");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    m_networkManager->get(request);
}

void SchedulerWidget::onApiResponse(QNetworkReply* reply) {
    m_refreshButton->setEnabled(true);

    if (reply->error() == QNetworkReply::NoError) {
        m_model->setRowCount(0); // clear existing rows
        QByteArray response = reply->readAll();
        
        try {
            m_currentJobs = json::parse(response.toStdString());
            
            if (m_currentJobs.is_array()) {
                for (const auto& job : m_currentJobs) {
                    QList<QStandardItem*> rowItems;
                    rowItems << new QStandardItem(QString::number(job.value("id", 0)));
                    rowItems << new QStandardItem(QString::fromStdString(job.value("name", "")));
                    rowItems << new QStandardItem(QString::fromStdString(job.value("command", "")));
                    rowItems << new QStandardItem(QString::fromStdString(job.value("cron_expr", "")));
                    
                    bool enabled = job.value("enabled", false);
                    auto statusItem = new QStandardItem(enabled ? "Enabled 🟢" : "Disabled 🔴");
                    rowItems << statusItem;

                    QString nextRunStr = "-";
                    if (enabled) {
                        std::string nr = job.value("next_run", "");
                        if (!nr.empty()) {
                            QDateTime dt = QDateTime::fromString(QString::fromStdString(nr), Qt::ISODate);
                            if (dt.isValid()) {
                                nextRunStr = dt.toLocalTime().toString("yyyy-MM-dd HH:mm:ss");
                            }
                        }
                    }
                    rowItems << new QStandardItem(nextRunStr);

                    bool isRunning = job.value("is_running", false);
                    int activePid = job.value("active_pid", 0);
                    QString activeStr = "Idle";
                    if (isRunning) {
                        if (activePid > 0) {
                            activeStr = QString("Running ⚙️ (PID %1)").arg(activePid);
                        } else {
                            activeStr = "Running ⚙️";
                        }
                    }
                    rowItems << new QStandardItem(activeStr);

                    m_model->appendRow(rowItems);
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("JSON parsing error: {}", e.what());
        }
    } else {
        spdlog::error("Scheduler API request failed: {}", reply->errorString().toStdString());
        m_model->setRowCount(0);
        m_model->appendRow({new QStandardItem("Error"), new QStandardItem(reply->errorString())});
    }
    m_tableView->resizeColumnsToContents();

    reply->deleteLater();
}

void SchedulerWidget::onAddJob() {
    JobEditorDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        json newJob = dlg.getJob();
        json payload = json::array({newJob});
        
        QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
        QUrl url(host + "/admin/update-jobs");
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        
        auto reply = m_networkManager->post(request, QByteArray::fromStdString(payload.dump()));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) onRefreshClicked();
            else QMessageBox::critical(this, "Error", "Failed to add job:\n" + reply->errorString());
            reply->deleteLater();
        });
    }
}

void SchedulerWidget::onEditJob() {
    auto selection = m_tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) return;
    int row = selection.first().row();
    QString jobName = m_model->item(row, 1)->text();
    
    json targetJob;
    for (const auto& job : m_currentJobs) {
        if (job.value("name", "") == jobName.toStdString()) {
            targetJob = job;
            break;
        }
    }
    if (targetJob.is_null()) return;

    JobEditorDialog dlg(this);
    dlg.setJob(targetJob);
    if (dlg.exec() == QDialog::Accepted) {
        json updatedJob = dlg.getJob();
        json payload = json::array({updatedJob});
        
        QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
        QUrl url(host + "/admin/update-jobs");
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        
        auto reply = m_networkManager->post(request, QByteArray::fromStdString(payload.dump()));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) onRefreshClicked();
            else QMessageBox::critical(this, "Error", "Failed to update job:\n" + reply->errorString());
            reply->deleteLater();
        });
    }
}

void SchedulerWidget::onDeleteJob() {
    auto selection = m_tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) return;
    int row = selection.first().row();
    QString jobName = m_model->item(row, 1)->text();
    
    if (QMessageBox::question(this, "Delete Job", "Are you sure you want to delete job '" + jobName + "'?") != QMessageBox::Yes) {
        return;
    }
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/delete-job?name=" + jobName);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    auto reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) onRefreshClicked();
        else QMessageBox::critical(this, "Error", "Failed to delete job:\n" + reply->errorString());
        reply->deleteLater();
    });
}

void SchedulerWidget::onStopJob() {
    if (!mitm::config::ConfigManager::GetInstance().HasRole("ADMIN")) {
        QMessageBox::warning(this, "Permission Denied", "Only users with the 'ADMIN' role are allowed to stop jobs.");
        return;
    }

    auto selection = m_tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) return;
    int row = selection.first().row();
    QString jobName = m_model->item(row, 1)->text();
    
    if (QMessageBox::question(this, "Stop Job", "Are you sure you want to stop running job '" + jobName + "'?") != QMessageBox::Yes) {
        return;
    }
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/stop-job?name=" + jobName);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    auto reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Success", "Stop signal sent to job.");
            onRefreshClicked();
        } else {
            QMessageBox::critical(this, "Error", "Failed to stop job:\n" + reply->errorString() + "\n" + QString::fromUtf8(reply->readAll()));
        }
        reply->deleteLater();
    });
}

void SchedulerWidget::onExecuteJob() {
    if (!mitm::config::ConfigManager::GetInstance().HasRole("ADMIN")) {
        QMessageBox::warning(this, "Permission Denied", "Only users with the 'ADMIN' role are allowed to execute jobs manually.");
        return;
    }

    auto selection = m_tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) return;
    int row = selection.first().row();
    QString jobName = m_model->item(row, 1)->text();
    
    if (QMessageBox::question(this, "Execute Job", "Are you sure you want to trigger job '" + jobName + "' now?") != QMessageBox::Yes) {
        return;
    }
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/execute-job?name=" + jobName);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    auto reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Success", "Job execution triggered.");
            onRefreshClicked();
        } else {
            QMessageBox::critical(this, "Error", "Failed to trigger job:\n" + reply->errorString() + "\n" + QString::fromUtf8(reply->readAll()));
        }
        reply->deleteLater();
    });
}

