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
#include <spdlog/spdlog.h>

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

    m_dlqTable = new QTableWidget(0, 4, this);
    m_dlqTable->setHorizontalHeaderLabels({"Timestamp", "Component", "Error Message", "Payload Snippet"});
    m_dlqTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_dlqTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_dlqTable);

    connect(m_refreshButton, &QPushButton::clicked, this, &DlqWidget::onRefresh);
    connect(m_requeueButton, &QPushButton::clicked, this, &DlqWidget::onRequeue);

    // Initial mock load
    onRefresh();
}

void DlqWidget::onRefresh() {
    m_dlqTable->setRowCount(0);
    // Insert conceptual mock data
    m_dlqTable->insertRow(0);
    m_dlqTable->setItem(0, 0, new QTableWidgetItem("2026-06-06 14:02:11"));
    m_dlqTable->setItem(0, 1, new QTableWidgetItem("Delivery Layer"));
    m_dlqTable->setItem(0, 2, new QTableWidgetItem("HTTP 401 Unauthorized"));
    m_dlqTable->setItem(0, 3, new QTableWidgetItem("{\"id\":1204,\"name\":\"..."));
    spdlog::info("DLQ refreshed (Mock Data)");
}

void DlqWidget::onRequeue() {
    spdlog::info("Requeueing selected DLQ items... (Concept)");
}
