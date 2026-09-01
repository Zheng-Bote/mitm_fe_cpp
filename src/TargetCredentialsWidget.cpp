/**
 * SPDX-FileComment: TargetCredentialsWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file TargetCredentialsWidget.cpp
 * @brief Widget to manage source_credentials.
 * @version 0.2.0
 * @date 2026-06-08
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "TargetCredentialsWidget.h"
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

TargetCredentialsWidget::TargetCredentialsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onRefresh();
}

void TargetCredentialsWidget::setupUi() {
    auto mainLayout = new QVBoxLayout(this);

    auto toolbarLayout = new QHBoxLayout();
    m_refreshBtn = new QPushButton("Refresh", this);
    m_addBtn = new QPushButton("Add Target", this);
    m_editBtn = new QPushButton("Edit Target", this);
    
    toolbarLayout->addWidget(m_refreshBtn);
    toolbarLayout->addWidget(m_addBtn);
    toolbarLayout->addWidget(m_editBtn);
    toolbarLayout->addStretch();
    
    mainLayout->addLayout(toolbarLayout);

    m_table = new QTableWidget(0, 6, this);
    m_table->setHorizontalHeaderLabels({"ID", "Topic", "Adapter Type", "Endpoint URL", "Config Payload", "Active"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    
    mainLayout->addWidget(m_table);

    connect(m_refreshBtn, &QPushButton::clicked, this, &TargetCredentialsWidget::onRefresh);
    connect(m_addBtn, &QPushButton::clicked, this, &TargetCredentialsWidget::onAddTarget);
    connect(m_editBtn, &QPushButton::clicked, this, &TargetCredentialsWidget::onEditTarget);
}

void TargetCredentialsWidget::onRefresh() {
    spdlog::info("Refreshing Targets...");
    fetchTargets();
}

void TargetCredentialsWidget::fetchTargets() {
    mitm::api::ApiClient::instance().get("/admin/delivery_targets",
        [this](const QByteArray& data, QNetworkReply*) {
            try {
                m_table->setRowCount(0);
                json j = json::parse(data.toStdString());
                if (j.is_array()) {
                    for (const auto& item : j) {
                        int row = m_table->rowCount();
                        m_table->insertRow(row);
                        
                        QString id = QString::fromStdString(item.value("id", ""));
                        QString topic = QString::fromStdString(item.value("topic", ""));
                        QString type = QString::fromStdString(item.value("adapter_type", ""));
                        QString url = QString::fromStdString(item.value("endpoint_url", ""));
                        QString config = QString::fromStdString(item.value("config_payload", ""));
                        bool isActive = item.value("is_active", false);

                        auto* idItem = new QTableWidgetItem(id);
                        idItem->setData(Qt::UserRole, config);

                        m_table->setItem(row, 0, idItem);
                        m_table->setItem(row, 1, new QTableWidgetItem(topic));
                        m_table->setItem(row, 2, new QTableWidgetItem(type));
                        m_table->setItem(row, 3, new QTableWidgetItem(url));
                        m_table->setItem(row, 4, new QTableWidgetItem(config));
                        m_table->setItem(row, 5, new QTableWidgetItem(isActive ? "Yes" : "No"));
                    }
                }
            } catch (...) {
                spdlog::error("Parse Error fetching targets");
            }
            m_table->resizeColumnsToContents();
        },
        [this](int /*statusCode*/, const QString& errorString) {
            spdlog::error("Error fetching targets: {}", errorString.toStdString());
            // Mock data for demonstration if backend not available
            m_table->setRowCount(0);
            m_table->insertRow(0);
            m_table->setItem(0, 0, new QTableWidgetItem("uuid-1234"));
            m_table->setItem(0, 1, new QTableWidgetItem("Employee"));
            m_table->setItem(0, 2, new QTableWidgetItem("CORITY_SAAS"));
            m_table->setItem(0, 3, new QTableWidgetItem("https://demo.cority.com"));
            m_table->setItem(0, 4, new QTableWidgetItem("{\"host\":\"localhost\"}"));
            m_table->setItem(0, 5, new QTableWidgetItem("Yes"));
            m_table->resizeColumnsToContents();
        }
    );
}

void TargetCredentialsWidget::onAddTarget() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add Target Credential");
    auto layout = new QFormLayout(&dialog);

    auto topicEdit = new QLineEdit("Employee", &dialog);
    auto typeEdit = new QLineEdit("CORITY_SAAS", &dialog);
    auto urlEdit = new QLineEdit("https://", &dialog);
    auto configEdit = new QTextEdit(&dialog);
    configEdit->setPlaceholderText("{\n  \"login_user\": \"svc_cority_import\",\n  \"login_pass\": \"SuperSecret123!\"\n}");
    configEdit->setMinimumHeight(100);
    auto activeCheck = new QCheckBox("Is Active", &dialog);
    activeCheck->setChecked(true);

    layout->addRow("Topic:", topicEdit);
    layout->addRow("Adapter Type:", typeEdit);
    layout->addRow("Endpoint URL:", urlEdit);
    layout->addRow("Config Payload:", configEdit);
    layout->addRow("", activeCheck);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        json j;
        j["topic"] = topicEdit->text().toStdString();
        j["adapter_type"] = typeEdit->text().toStdString();
        j["endpoint_url"] = urlEdit->text().toStdString();
        j["config_payload"] = configEdit->toPlainText().toStdString();
        j["is_active"] = activeCheck->isChecked();

        mitm::api::ApiClient::instance().post("/admin/delivery_targets", QString::fromStdString(j.dump()).toUtf8(),
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

void TargetCredentialsWidget::onEditTarget() {
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Select", "Please select a source to edit.");
        return;
    }

    QString id = m_table->item(row, 0)->text();
    QString topic = m_table->item(row, 1)->text();
    QString type = m_table->item(row, 2)->text();
    QString url = m_table->item(row, 3)->text();
    QString configPayload = m_table->item(row, 4)->text();
    bool isActive = (m_table->item(row, 5)->text() == "Yes");

    QDialog dialog(this);
    dialog.setWindowTitle("Edit Target Credential");
    auto layout = new QFormLayout(&dialog);

    auto topicEdit = new QLineEdit(topic, &dialog);
    topicEdit->setReadOnly(true);
    auto typeEdit = new QLineEdit(type, &dialog);
    auto urlEdit = new QLineEdit(url, &dialog);
    auto configEdit = new QTextEdit(configPayload, &dialog);
    configEdit->setMinimumHeight(100);
    auto activeCheck = new QCheckBox("Is Active", &dialog);
    activeCheck->setChecked(isActive);

    layout->addRow("Topic:", topicEdit);
    layout->addRow("Adapter Type:", typeEdit);
    layout->addRow("Endpoint URL:", urlEdit);
    layout->addRow("Config Payload:", configEdit);
    layout->addRow("", activeCheck);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        json j;
        j["id"] = id.toStdString();
        j["topic"] = topicEdit->text().toStdString();
        j["adapter_type"] = typeEdit->text().toStdString();
        j["endpoint_url"] = urlEdit->text().toStdString();
        j["config_payload"] = configEdit->toPlainText().toStdString();
        j["is_active"] = activeCheck->isChecked();

        mitm::api::ApiClient::instance().post("/admin/delivery_targets", QString::fromStdString(j.dump()).toUtf8(),
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
