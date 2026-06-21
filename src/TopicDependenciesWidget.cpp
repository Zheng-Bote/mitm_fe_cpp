/**
 * SPDX-FileComment: TopicDependenciesWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TopicDependenciesWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "Config.h"

using json = nlohmann::json;

TopicDependenciesWidget::TopicDependenciesWidget(QWidget *parent)
    : QWidget(parent), m_networkManager(new QNetworkAccessManager(this))
{
    auto mainLayout = new QVBoxLayout(this);

    auto headerLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton("Refresh", this);
    m_addButton = new QPushButton("Add Topic Dependency", this);
    m_deleteButton = new QPushButton("Delete Selected", this);
    
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_addButton);
    headerLayout->addWidget(m_deleteButton);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    m_tableView = new QTableView(this);
    m_model = new QStandardItemModel(0, 2, this);
    m_model->setHorizontalHeaderLabels({"Topic", "Required Sources"});
    
    m_tableView->setModel(m_model);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    
    mainLayout->addWidget(m_tableView);

    connect(m_refreshButton, &QPushButton::clicked, this, &TopicDependenciesWidget::onRefresh);
    connect(m_addButton, &QPushButton::clicked, this, &TopicDependenciesWidget::onAdd);
    connect(m_deleteButton, &QPushButton::clicked, this, &TopicDependenciesWidget::onDelete);

    onRefresh();
}

QString TopicDependenciesWidget::getAuthHeader() {
    return mitm::config::ConfigManager::GetInstance().GetAuthHeader();
}

void TopicDependenciesWidget::onRefresh() {
    m_refreshButton->setEnabled(false);
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/transformation/topic-dependencies");
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
                    for (const auto& row : data) {
                        QList<QStandardItem*> rowItems;
                        rowItems << new QStandardItem(QString::fromStdString(row.value("topic", "")));
                        
                        QStringList sources;
                        if (row.contains("required_sources") && row["required_sources"].is_array()) {
                            for (const auto& src : row["required_sources"]) {
                                sources << QString::fromStdString(src.get<std::string>());
                            }
                        }
                        rowItems << new QStandardItem(sources.join(", "));
                        
                        m_model->appendRow(rowItems);
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("JSON parsing error in TopicDependencies: {}", e.what());
            }
        } else {
            spdlog::error("TopicDependencies API failed: {}", reply->errorString().toStdString());
            m_model->setRowCount(0);
            m_model->appendRow({new QStandardItem("Error"), new QStandardItem(reply->errorString())});
        }
        reply->deleteLater();
    });
}

void TopicDependenciesWidget::onAdd() {
    bool ok;
    QString topic = QInputDialog::getText(this, "Add Topic Dependency", "Topic Name:", QLineEdit::Normal, "", &ok);
    if (!ok || topic.isEmpty()) return;

    QString sourcesStr = QInputDialog::getText(this, "Add Topic Dependency", "Required Sources (comma separated):", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QStringList sourcesList = sourcesStr.split(",", Qt::SkipEmptyParts);
    QJsonArray sourcesArray;
    for (const QString& src : sourcesList) {
        sourcesArray.append(src.trimmed());
    }

    QJsonObject payload;
    payload["topic"] = topic;
    payload["required_sources"] = sourcesArray;

    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/transformation/topic-dependencies");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            onRefresh();
        } else {
            QMessageBox::critical(this, "Error", "Failed to add topic dependency:\n" + reply->errorString());
        }
        reply->deleteLater();
    });
}

void TopicDependenciesWidget::onDelete() {
    auto selection = m_tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) return;

    int row = selection.first().row();
    QString topic = m_model->item(row, 0)->text();

    if (QMessageBox::question(this, "Confirm Delete", "Delete topic dependency for '" + topic + "'?") != QMessageBox::Yes) {
        return;
    }

    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/transformation/topic-dependencies?topic=" + topic);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());

    QNetworkReply* reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            onRefresh();
        } else {
            QMessageBox::critical(this, "Error", "Failed to delete topic dependency:\n" + reply->errorString());
        }
        reply->deleteLater();
    });
}
