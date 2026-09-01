/**
 * SPDX-FileComment: SourcesWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file SourcesWidget.cpp
 * @brief Widget to manage source_credentials.
 * @version 0.2.0
 * @date 2026-06-08
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "SourcesWidget.h"
#include "Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QMessageBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <spdlog/spdlog.h>
#include "ApiClient.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

SourcesWidget::SourcesWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onRefresh();
}

void SourcesWidget::setupUi() {
    auto mainLayout = new QVBoxLayout(this);

    auto toolbarLayout = new QHBoxLayout();
    m_refreshBtn = new QPushButton("Refresh", this);
    m_addBtn = new QPushButton("Add Source", this);
    m_editBtn = new QPushButton("Edit Source", this);
    
    toolbarLayout->addWidget(m_refreshBtn);
    toolbarLayout->addWidget(m_addBtn);
    toolbarLayout->addWidget(m_editBtn);
    toolbarLayout->addStretch();
    
    mainLayout->addLayout(toolbarLayout);

    m_table = new QTableWidget(0, 6, this);
    m_table->setHorizontalHeaderLabels({"ID", "Source Name", "Connector Type", "Topic", "Config Payload", "Active"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    
    mainLayout->addWidget(m_table);

    connect(m_refreshBtn, &QPushButton::clicked, this, &SourcesWidget::onRefresh);
    connect(m_addBtn, &QPushButton::clicked, this, &SourcesWidget::onAddSource);
    connect(m_editBtn, &QPushButton::clicked, this, &SourcesWidget::onEditSource);
}

void SourcesWidget::onRefresh() {
    spdlog::info("Refreshing Sources...");
    fetchSources();
}

void SourcesWidget::fetchSources() {
    mitm::api::ApiClient::instance().get("/admin/credentials",
        [this](const QByteArray& data, QNetworkReply*) {
            try {
                m_table->setRowCount(0);
                json j = json::parse(data.toStdString());
                if (j.is_array()) {
                    for (const auto& item : j) {
                        int row = m_table->rowCount();
                        m_table->insertRow(row);
                        
                        QString id = QString::fromStdString(item.value("id", ""));
                        QString name = QString::fromStdString(item.value("source_name", ""));
                        QString type = QString::fromStdString(item.value("connector_type", ""));
                        QString topic = QString::fromStdString(item.value("topic", ""));
                        QString config = QString::fromStdString(item.value("config_payload", ""));
                        bool isActive = item.value("is_active", false);

                        auto* idItem = new QTableWidgetItem(id);
                        idItem->setData(Qt::UserRole, config);

                        m_table->setItem(row, 0, idItem);
                        m_table->setItem(row, 1, new QTableWidgetItem(name));
                        m_table->setItem(row, 2, new QTableWidgetItem(type));
                        m_table->setItem(row, 3, new QTableWidgetItem(topic));
                        m_table->setItem(row, 4, new QTableWidgetItem(config));
                        m_table->setItem(row, 5, new QTableWidgetItem(isActive ? "Yes" : "No"));
                    }
                }
            } catch (...) {
                spdlog::error("Parse Error fetching sources");
            }
            m_table->resizeColumnsToContents();
        },
        [this](int /*statusCode*/, const QString& errorString) {
            spdlog::error("Error fetching sources: {}", errorString.toStdString());
            // Mock data for demonstration if backend not available
            m_table->setRowCount(0);
            m_table->insertRow(0);
            m_table->setItem(0, 0, new QTableWidgetItem("uuid-1234"));
            m_table->setItem(0, 1, new QTableWidgetItem("SAP_HR_PROD"));
            m_table->setItem(0, 2, new QTableWidgetItem("POSTGRESQL"));
            m_table->setItem(0, 3, new QTableWidgetItem("Employee"));
            m_table->setItem(0, 4, new QTableWidgetItem("{\"host\":\"localhost\"}"));
            m_table->setItem(0, 5, new QTableWidgetItem("Yes"));
            m_table->resizeColumnsToContents();
        }
    );
}

void SourcesWidget::onAddSource() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add Source Credential");
    auto layout = new QFormLayout(&dialog);

    auto nameEdit = new QLineEdit(&dialog);
    auto typeEdit = new QLineEdit("POSTGRESQL", &dialog);
    auto topicEdit = new QLineEdit("Employee", &dialog);
    auto configEdit = new QTextEdit(&dialog);
    configEdit->setPlaceholderText("{\n  \"host\": \"localhost\",\n  \"port\": 5432\n}");
    configEdit->setMinimumHeight(100);
    auto activeCheck = new QCheckBox("Is Active", &dialog);
    activeCheck->setChecked(true);

    layout->addRow("Source Name:", nameEdit);
    layout->addRow("Connector Type:", typeEdit);
    layout->addRow("Topic:", topicEdit);
    layout->addRow("Config Payload:", configEdit);
    layout->addRow("", activeCheck);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        json j;
        j["source_name"] = nameEdit->text().toStdString();
        j["connector_type"] = typeEdit->text().toStdString();
        j["topic"] = topicEdit->text().toStdString();
        j["config_payload"] = configEdit->toPlainText().toStdString();
        j["is_active"] = activeCheck->isChecked();

        mitm::api::ApiClient::instance().post("/admin/credentials", QString::fromStdString(j.dump()).toUtf8(),
            [this](const QByteArray&, QNetworkReply*) {
                QMessageBox::information(this, "Success", "Source added.");
                onRefresh();
            },
            [this](int /*statusCode*/, const QString& errorString) {
                QMessageBox::critical(this, "Error", "Failed to add source: " + errorString);
            }
        );
    }
}

void SourcesWidget::onEditSource() {
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Select", "Please select a source to edit.");
        return;
    }

    QString id = m_table->item(row, 0)->text();
    QString configPayload = m_table->item(row, 4)->text();
    QString name = m_table->item(row, 1)->text();
    QString type = m_table->item(row, 2)->text();
    QString topic = m_table->item(row, 3)->text();
    bool isActive = (m_table->item(row, 5)->text() == "Yes");

    QDialog dialog(this);
    dialog.setWindowTitle("Edit Source Credential");
    auto layout = new QFormLayout(&dialog);

    auto nameEdit = new QLineEdit(name, &dialog);
    nameEdit->setReadOnly(true);
    auto typeEdit = new QLineEdit(type, &dialog);
    auto topicEdit = new QLineEdit(topic, &dialog);
    auto configEdit = new QTextEdit(configPayload, &dialog);
    configEdit->setMinimumHeight(100);
    auto activeCheck = new QCheckBox("Is Active", &dialog);
    activeCheck->setChecked(isActive);

    layout->addRow("Source Name:", nameEdit);
    layout->addRow("Connector Type:", typeEdit);
    layout->addRow("Topic:", topicEdit);
    layout->addRow("Config Payload:", configEdit);
    layout->addRow("", activeCheck);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        json j;
        j["id"] = id.toStdString();
        j["source_name"] = nameEdit->text().toStdString();
        j["connector_type"] = typeEdit->text().toStdString();
        j["topic"] = topicEdit->text().toStdString();
        j["config_payload"] = configEdit->toPlainText().toStdString();
        j["is_active"] = activeCheck->isChecked();

        mitm::api::ApiClient::instance().post("/admin/credentials", QString::fromStdString(j.dump()).toUtf8(),
            [this](const QByteArray&, QNetworkReply*) {
                QMessageBox::information(this, "Success", "Source updated.");
                onRefresh();
            },
            [this](int /*statusCode*/, const QString& errorString) {
                QMessageBox::critical(this, "Error", "Failed to update source: " + errorString);
            }
        );
    }
}
