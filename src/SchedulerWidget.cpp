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
    
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_addButton);
    headerLayout->addWidget(m_editButton);
    headerLayout->addWidget(m_deleteButton);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    m_tableView = new QTableView(this);
    m_model = new QStandardItemModel(0, 5, this);
    m_model->setHorizontalHeaderLabels({"ID", "Name", "Command", "Cron Expr", "Status"});
    
    m_tableView->setModel(m_model);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSortingEnabled(true);
    
    mainLayout->addWidget(m_tableView);

    connect(m_refreshButton, &QPushButton::clicked, this, &SchedulerWidget::onRefreshClicked);
    connect(m_addButton, &QPushButton::clicked, this, &SchedulerWidget::onAddJob);
    connect(m_editButton, &QPushButton::clicked, this, &SchedulerWidget::onEditJob);
    connect(m_deleteButton, &QPushButton::clicked, this, &SchedulerWidget::onDeleteJob);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &SchedulerWidget::onApiResponse);
}

QString SchedulerWidget::getAuthHeader() {
    auto env = QProcessEnvironment::systemEnvironment();
    QString osUser = env.value("USER", env.value("USERNAME", "unknown"));
    QString token = "helo_linux"; // Fallback
    
    QFile configFile("../../../../data/config.json");
    if (!configFile.exists()) {
        configFile.setFileName("../../data/config.json");
    }
    
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

void SchedulerWidget::onRefreshClicked() {
    m_refreshButton->setEnabled(false);
    
    QUrl url("http://localhost:8080/admin/jobs");
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

    reply->deleteLater();
}

void SchedulerWidget::onAddJob() {
    JobEditorDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        json newJob = dlg.getJob();
        json payload = json::array({newJob});
        
        QUrl url("http://localhost:8080/admin/update-jobs");
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
        
        QUrl url("http://localhost:8080/admin/update-jobs");
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
    
    QUrl url("http://localhost:8080/admin/delete-job?name=" + jobName);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    auto reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) onRefreshClicked();
        else QMessageBox::critical(this, "Error", "Failed to delete job:\n" + reply->errorString());
        reply->deleteLater();
    });
}
