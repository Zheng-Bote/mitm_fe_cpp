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
#include <QNetworkRequest>
#include <QUrl>
#include <QMessageBox>
#include <spdlog/spdlog.h>
#include "Config.h"
#include "schematas/dlq_generated.h"

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

    m_dlqTable = new QTableWidget(0, 5, this);
    m_dlqTable->setHorizontalHeaderLabels({"ID", "Timestamp", "Component", "Error Message", "Payload Snippet"});
    m_dlqTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_dlqTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_dlqTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_dlqTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_dlqTable);

    connect(m_refreshButton, &QPushButton::clicked, this, &DlqWidget::onRefresh);
    connect(m_requeueButton, &QPushButton::clicked, this, &DlqWidget::onRequeue);

    // Data will only be loaded when the user clicks 'Refresh DLQ'
}

void DlqWidget::onRefresh() {
    m_dlqTable->setRowCount(0);
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QNetworkRequest request(QUrl(host + "/admin/dlq_bin"));
    QString authHeader = mitm::config::ConfigManager::GetInstance().GetAuthHeader();
    request.setRawHeader("Authorization", authHeader.toLocal8Bit());

    auto reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("Failed to fetch DLQ: {}", reply->errorString().toStdString());
            return;
        }

        QByteArray data = reply->readAll();
        flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
        if (!schematas::VerifyDLQEntryListBuffer(verifier)) {
            spdlog::error("Invalid DLQ FlatBuffer data received");
            return;
        }

        auto list = schematas::GetDLQEntryList(data.constData());
        if (!list || !list->entries()) return;

        auto arr = list->entries();
        m_dlqTable->setRowCount(arr->size());

        for (int i = 0; i < arr->size(); ++i) {
            auto obj = arr->Get(i);
            if (!obj) continue;
            
            QString idStr = obj->id() ? QString::fromUtf8(obj->id()->c_str()) : "";
            
            // Format timestamp slightly
            QString failedAt = obj->failed_at() ? QString::fromUtf8(obj->failed_at()->c_str()) : "";
            failedAt.replace("T", " ").replace("Z", "");

            QString errorCode = obj->error_code() ? QString::fromUtf8(obj->error_code()->c_str()) : "";
            
            QString errorMessage = obj->error_message() ? QString::fromUtf8(obj->error_message()->c_str()) : "";
            if (errorMessage.length() > 256) {
                errorMessage = errorMessage.left(253) + "...";
            }
            
            QString payload = obj->payload() ? QString::fromUtf8(obj->payload()->c_str()) : "";
            if (payload.length() > 256) {
                payload = payload.left(253) + "...";
            }

            m_dlqTable->setItem(i, 0, new QTableWidgetItem(idStr));
            m_dlqTable->setItem(i, 1, new QTableWidgetItem(failedAt));
            m_dlqTable->setItem(i, 2, new QTableWidgetItem(errorCode));
            m_dlqTable->setItem(i, 3, new QTableWidgetItem(errorMessage));
            m_dlqTable->setItem(i, 4, new QTableWidgetItem(payload));
        }
        m_dlqTable->resizeColumnsToContents();
        spdlog::info("DLQ refreshed with {} items", arr->size());
    });
}

void DlqWidget::onRequeue() {
    if (!mitm::config::ConfigManager::GetInstance().HasRole("ADMIN")) {
        QMessageBox::warning(this, "Permission Denied", "Only users with the 'ADMIN' role are allowed to requeue DLQ items.");
        return;
    }

    auto selection = m_dlqTable->selectionModel()->selectedRows();
    if (selection.isEmpty()) return;
    int row = selection.first().row();
    QString idStr = m_dlqTable->item(row, 0)->text();
    
    if (QMessageBox::question(this, "Requeue DLQ Item", "Are you sure you want to requeue item '" + idStr + "'?") != QMessageBox::Yes) {
        return;
    }
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/dlq/requeue?id=" + idStr);
    QNetworkRequest request(url);
    QString authHeader = mitm::config::ConfigManager::GetInstance().GetAuthHeader();
    request.setRawHeader("Authorization", authHeader.toLocal8Bit());
    
    auto reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Success", "DLQ item requeued.");
            onRefresh();
        } else {
            QMessageBox::critical(this, "Error", "Failed to requeue DLQ item:\n" + reply->errorString() + "\n" + QString::fromUtf8(reply->readAll()));
        }
        reply->deleteLater();
    });
}
