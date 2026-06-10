/**
 * SPDX-FileComment: DlqWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file DlqWidget.cpp
 * @brief DlqWidget
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "DlqWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <spdlog/spdlog.h>
#include "Config.h"

DlqWidget::DlqWidget(QWidget *parent) : QWidget(parent) {
    auto mainLayout = new QVBoxLayout(this);

    auto headerLayout = new QHBoxLayout();
    headerLayout->addWidget(new QLabel("<b>Dead Letter Queue (DLQ) & Cursors</b>", this));
    headerLayout->addStretch();
    m_refreshButton = new QPushButton("Refresh DLQ", this);
    m_requeueButton = new QPushButton("Requeue Selected", this);
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_requeueButton);
    mainLayout->addLayout(headerLayout);

    m_networkManager = new QNetworkAccessManager(this);

    m_dlqTable = new QTableWidget(0, 4, this);
    m_dlqTable->setHorizontalHeaderLabels({"Timestamp", "Component", "Error Message", "Payload Snippet"});
    m_dlqTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_dlqTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_dlqTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_dlqTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_dlqTable);

    connect(m_refreshButton, &QPushButton::clicked, this, &DlqWidget::onRefresh);
    connect(m_requeueButton, &QPushButton::clicked, this, &DlqWidget::onRequeue);

    // Fill initial mock data
    onRefresh();
}

void DlqWidget::onRefresh() {
    m_dlqTable->setRowCount(0);
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QNetworkRequest request(QUrl(host + "/admin/dlq"));
    QString authHeader = mitm::config::ConfigManager::GetInstance().GetAuthHeader();
    request.setRawHeader("Authorization", authHeader.toLocal8Bit());

    auto reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("Failed to fetch DLQ: {}", reply->errorString().toStdString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) return;

        QJsonArray arr = doc.array();
        m_dlqTable->setRowCount(arr.size());

        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject obj = arr[i].toObject();
            
            // Format timestamp slightly
            QString failedAt = obj["failed_at"].toString();
            failedAt.replace("T", " ").replace("Z", "");

            m_dlqTable->setItem(i, 0, new QTableWidgetItem(failedAt));
            m_dlqTable->setItem(i, 1, new QTableWidgetItem(obj["error_code"].toString()));
            m_dlqTable->setItem(i, 2, new QTableWidgetItem(obj["error_message"].toString()));
            m_dlqTable->setItem(i, 3, new QTableWidgetItem(obj["payload"].toString()));
        }
        m_dlqTable->resizeColumnsToContents();
        spdlog::info("DLQ refreshed with {} items", arr.size());
    });
}

void DlqWidget::onRequeue() {
    spdlog::info("Requeueing selected DLQ items... (Concept)");
}
