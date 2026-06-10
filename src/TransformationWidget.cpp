/**
 * SPDX-FileComment: TransformationWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TransformationWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QNetworkRequest>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "Config.h"
#include "RuleEditorDialog.h"
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QInputDialog>


using json = nlohmann::json;

TransformationWidget::TransformationWidget(QWidget *parent)
    : QWidget(parent), m_networkManager(new QNetworkAccessManager(this)) {
    
    auto mainLayout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    auto sourcesTab = new QWidget();
    setupSourcesTab(sourcesTab);
    m_tabWidget->addTab(sourcesTab, "Sources");

    auto targetsTab = new QWidget();
    setupTargetsTab(targetsTab);
    m_tabWidget->addTab(targetsTab, "Target Fields");

    auto rulesTab = new QWidget();
    setupRulesTab(rulesTab);
    m_tabWidget->addTab(rulesTab, "Rules");

    auto transformsTab = new QWidget();
    setupTransformationsTab(transformsTab);
    m_tabWidget->addTab(transformsTab, "Transformations");

    auto validationsTab = new QWidget();
    setupValidationsTab(validationsTab);
    m_tabWidget->addTab(validationsTab, "Validations");

    onRefreshSources();
    onRefreshTargets();
    onRefreshRules();
    onRefreshTransformations();
    onRefreshValidations();
}

QString TransformationWidget::getAuthHeader() {
    return mitm::config::ConfigManager::GetInstance().GetAuthHeader();
}

void TransformationWidget::postEntity(const QString& endpoint, const nlohmann::json& payload, const std::function<void()>& onSuccess) {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + endpoint);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->post(request, QString::fromStdString(payload.dump()).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess]() {
        if (reply->error() == QNetworkReply::NoError) {
            onSuccess();
        } else {
            QString errBody = reply->readAll();
            if (errBody.isEmpty()) errBody = reply->errorString();
            QMessageBox::critical(this, "Error", "Failed to save: " + errBody);
        }
        reply->deleteLater();
    });
}

void TransformationWidget::deleteEntity(const QString& endpoint, const QString& id, const std::function<void()>& onSuccess) {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + endpoint + "?id=" + id);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());

    QNetworkReply* reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess]() {
        if (reply->error() == QNetworkReply::NoError) {
            onSuccess();
        } else {
            QString errBody = reply->readAll();
            if (errBody.isEmpty()) errBody = reply->errorString();
            QMessageBox::critical(this, "Error", "Failed to delete: " + errBody);
        }
        reply->deleteLater();
    });
}


void TransformationWidget::setupSourcesTab(QWidget* tab) {
    auto layout = new QVBoxLayout(tab);
    auto headerLayout = new QHBoxLayout();
    m_refreshSourcesBtn = new QPushButton("Refresh Sources", this);
    m_addSourceBtn = new QPushButton("Add", this);
    m_editSourceBtn = new QPushButton("Edit", this);
    m_deleteSourceBtn = new QPushButton("Delete", this);
    headerLayout->addWidget(m_refreshSourcesBtn);
    headerLayout->addWidget(m_addSourceBtn);
    headerLayout->addWidget(m_editSourceBtn);
    headerLayout->addWidget(m_deleteSourceBtn);
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    m_sourcesTable = new QTableWidget(0, 6, this);
    m_sourcesTable->setHorizontalHeaderLabels({"ID", "Name", "Type", "Topic", "Version", "Created At"});
    m_sourcesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_sourcesTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_sourcesTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_sourcesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_sourcesTable);

    connect(m_addSourceBtn, &QPushButton::clicked, this, &TransformationWidget::onAddSource);
    connect(m_editSourceBtn, &QPushButton::clicked, this, &TransformationWidget::onEditSource);
    connect(m_deleteSourceBtn, &QPushButton::clicked, this, &TransformationWidget::onDeleteSource);
    connect(m_refreshSourcesBtn, &QPushButton::clicked, this, &TransformationWidget::onRefreshSources);
}

void TransformationWidget::setupTargetsTab(QWidget* tab) {
    auto layout = new QVBoxLayout(tab);
    auto headerLayout = new QHBoxLayout();
    m_refreshTargetsBtn = new QPushButton("Refresh Targets", this);
    m_addTargetBtn = new QPushButton("Add", this);
    m_editTargetBtn = new QPushButton("Edit", this);
    m_deleteTargetBtn = new QPushButton("Delete", this);
    headerLayout->addWidget(m_refreshTargetsBtn);
    headerLayout->addWidget(m_addTargetBtn);
    headerLayout->addWidget(m_editTargetBtn);
    headerLayout->addWidget(m_deleteTargetBtn);
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    m_targetsTable = new QTableWidget(0, 7, this);
    m_targetsTable->setHorizontalHeaderLabels({"ID", "Topic", "Field Name", "Data Type", "Required", "Encrypted", "Version"});
    m_targetsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_targetsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_targetsTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_targetsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_targetsTable);

    connect(m_addTargetBtn, &QPushButton::clicked, this, &TransformationWidget::onAddTarget);
    connect(m_editTargetBtn, &QPushButton::clicked, this, &TransformationWidget::onEditTarget);
    connect(m_deleteTargetBtn, &QPushButton::clicked, this, &TransformationWidget::onDeleteTarget);
    connect(m_refreshTargetsBtn, &QPushButton::clicked, this, &TransformationWidget::onRefreshTargets);
}

void TransformationWidget::setupRulesTab(QWidget* tab) {
    auto layout = new QVBoxLayout(tab);
    auto headerLayout = new QHBoxLayout();
    m_refreshRulesBtn = new QPushButton("Refresh Rules", this);
    m_addRuleBtn = new QPushButton("Add", this);
    m_editRuleBtn = new QPushButton("Edit", this);
    m_deleteRuleBtn = new QPushButton("Delete", this);
    m_autoMapRulesBtn = new QPushButton("✨ Auto-Map (Smart Suggest)", this);
    
    headerLayout->addWidget(m_refreshRulesBtn);
    headerLayout->addWidget(m_addRuleBtn);
    headerLayout->addWidget(m_editRuleBtn);
    headerLayout->addWidget(m_deleteRuleBtn);
    headerLayout->addWidget(m_autoMapRulesBtn);
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    m_rulesTable = new QTableWidget(0, 9, this);
    m_rulesTable->setHorizontalHeaderLabels({"ID", "Source ID", "Target Field ID", "Source Field", "Target Field", "Priority", "Transform Chain", "Validation Chain", "Version"});
    m_rulesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_rulesTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_rulesTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_rulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_rulesTable);

    connect(m_addRuleBtn, &QPushButton::clicked, this, &TransformationWidget::onAddRule);
    connect(m_editRuleBtn, &QPushButton::clicked, this, &TransformationWidget::onEditRule);
    connect(m_deleteRuleBtn, &QPushButton::clicked, this, &TransformationWidget::onDeleteRule);
    connect(m_refreshRulesBtn, &QPushButton::clicked, this, &TransformationWidget::onRefreshRules);
    
    // Minimal stub for Auto-Map
    connect(m_autoMapRulesBtn, &QPushButton::clicked, this, [this](){
        auto items = m_sourcesTable->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::warning(this, "Auto-Map", "Please select a Mapping Source from the Sources tab first.");
            return;
        }
        QString sourceId = m_sourcesTable->item(items.first()->row(), 0)->text();

        bool ok;
        QString fieldsStr = QInputDialog::getText(this, "Auto-Map (Smart Suggest)",
            "Enter source fields to map (comma separated):", QLineEdit::Normal,
            "first_name, last_name, dob, email_address", &ok);
            
        if (!ok || fieldsStr.trimmed().isEmpty()) return;
        
        QStringList fieldsList = fieldsStr.split(",", Qt::SkipEmptyParts);
        json j;
        j["source_id"] = sourceId.toStdString();
        j["source_fields"] = json::array();
        for (const auto& f : fieldsList) {
            j["source_fields"].push_back(f.trimmed().toStdString());
        }

        postEntity("/admin/transformation/auto-map", j, [this](){
            QMessageBox::information(this, "Auto-Map", "Auto-Map completed successfully!");
            onRefreshRules();
        });
    });
}

void TransformationWidget::setupTransformationsTab(QWidget* tab) {
    auto layout = new QVBoxLayout(tab);
    auto headerLayout = new QHBoxLayout();
    m_refreshTransformBtn = new QPushButton("Refresh Transformations", this);
    m_addTransformBtn = new QPushButton("Add", this);
    m_editTransformBtn = new QPushButton("Edit", this);
    m_deleteTransformBtn = new QPushButton("Delete", this);
    headerLayout->addWidget(m_addTransformBtn);
    headerLayout->addWidget(m_editTransformBtn);
    headerLayout->addWidget(m_deleteTransformBtn);

    headerLayout->addWidget(m_refreshTransformBtn);
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    m_transformTable = new QTableWidget(0, 5, this);
    m_transformTable->setHorizontalHeaderLabels({"ID", "Name", "Description", "Parameters", "Version"});
    m_transformTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_transformTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_transformTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_transformTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_transformTable);


    connect(m_addTransformBtn, &QPushButton::clicked, this, &TransformationWidget::onAddTransformation);
    connect(m_editTransformBtn, &QPushButton::clicked, this, &TransformationWidget::onEditTransformation);
    connect(m_deleteTransformBtn, &QPushButton::clicked, this, &TransformationWidget::onDeleteTransformation);
    connect(m_refreshTransformBtn, &QPushButton::clicked, this, &TransformationWidget::onRefreshTransformations);
}

void TransformationWidget::setupValidationsTab(QWidget* tab) {
    auto layout = new QVBoxLayout(tab);
    auto headerLayout = new QHBoxLayout();
    m_refreshValidBtn = new QPushButton("Refresh Validations", this);
    m_addValidBtn = new QPushButton("Add", this);
    m_editValidBtn = new QPushButton("Edit", this);
    m_deleteValidBtn = new QPushButton("Delete", this);
    headerLayout->addWidget(m_addValidBtn);
    headerLayout->addWidget(m_editValidBtn);
    headerLayout->addWidget(m_deleteValidBtn);

    headerLayout->addWidget(m_refreshValidBtn);
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    m_validTable = new QTableWidget(0, 5, this);
    m_validTable->setHorizontalHeaderLabels({"ID", "Name", "Description", "Parameters", "Version"});
    m_validTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_validTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_validTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_validTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_validTable);


    connect(m_addValidBtn, &QPushButton::clicked, this, &TransformationWidget::onAddValidation);
    connect(m_editValidBtn, &QPushButton::clicked, this, &TransformationWidget::onEditValidation);
    connect(m_deleteValidBtn, &QPushButton::clicked, this, &TransformationWidget::onDeleteValidation);
    connect(m_refreshValidBtn, &QPushButton::clicked, this, &TransformationWidget::onRefreshValidations);
}

// ---------------------------------------------------------
// Refresh Slots
// ---------------------------------------------------------

void TransformationWidget::onRefreshSources() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/transformation/sources");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    auto reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            m_sourcesTable->setRowCount(0);
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    for (const auto& item : data) {
                        int row = m_sourcesTable->rowCount();
                        m_sourcesTable->insertRow(row);
                        m_sourcesTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(item.value("id", ""))));
                        m_sourcesTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(item.value("name", ""))));
                        m_sourcesTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(item.value("type", ""))));
                        m_sourcesTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(item.value("topic", ""))));
                        m_sourcesTable->setItem(row, 4, new QTableWidgetItem(QString::number(item.value("version", 0))));
                        m_sourcesTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(item.value("created_at", ""))));
                    }
                }
            } catch (...) {
                spdlog::error("JSON parse error on sources");
            }
        } else {
            spdlog::error("Error fetching sources: {}", reply->errorString().toStdString());
        }
        m_sourcesTable->resizeColumnsToContents();
        reply->deleteLater();
    });
}

void TransformationWidget::onRefreshTargets() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/transformation/targets");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    auto reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            m_targetsTable->setRowCount(0);
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    for (const auto& item : data) {
                        int row = m_targetsTable->rowCount();
                        m_targetsTable->insertRow(row);
                        m_targetsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(item.value("id", ""))));
                        m_targetsTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(item.value("topic", ""))));
                        m_targetsTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(item.value("field_name", ""))));
                        m_targetsTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(item.value("data_type", ""))));
                        m_targetsTable->setItem(row, 4, new QTableWidgetItem(item.value("is_required", false) ? "Yes" : "No"));
                        m_targetsTable->setItem(row, 5, new QTableWidgetItem(item.value("encrypted", false) ? "Yes" : "No"));
                        m_targetsTable->setItem(row, 6, new QTableWidgetItem(QString::number(item.value("version", 0))));
                    }
                }
            } catch (...) {
                spdlog::error("JSON parse error on targets");
            }
        }
        m_targetsTable->resizeColumnsToContents();
        reply->deleteLater();
    });
}

void TransformationWidget::onRefreshRules() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/transformation/rules");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    auto reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            m_rulesTable->setRowCount(0);
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    for (const auto& item : data) {
                        int row = m_rulesTable->rowCount();
                        m_rulesTable->insertRow(row);
                        m_rulesTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(item.value("id", ""))));
                        m_rulesTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(item.value("source_id", ""))));
                        QString tgtId = QString::fromStdString(item.value("target_field_id", ""));
                        m_rulesTable->setItem(row, 2, new QTableWidgetItem(tgtId));
                        m_rulesTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(item.value("source_field", ""))));
                        
                        QString targetFieldStr = tgtId;
                        for (int i = 0; i < m_targetsTable->rowCount(); ++i) {
                            if (m_targetsTable->item(i, 0) && m_targetsTable->item(i, 0)->text() == tgtId) {
                                QString topic = m_targetsTable->item(i, 1) ? m_targetsTable->item(i, 1)->text() : "";
                                QString fld = m_targetsTable->item(i, 2) ? m_targetsTable->item(i, 2)->text() : "";
                                targetFieldStr = topic + "->" + fld;
                                break;
                            }
                        }
                        m_rulesTable->setItem(row, 4, new QTableWidgetItem(targetFieldStr));
                        
                        m_rulesTable->setItem(row, 5, new QTableWidgetItem(QString::number(item.value("priority", 0))));
                        
                        QString transformChain = "";
                        if (item.contains("transformation_chain") && !item["transformation_chain"].is_null()) {
                            transformChain = QString::fromStdString(item["transformation_chain"].dump());
                        }
                        m_rulesTable->setItem(row, 6, new QTableWidgetItem(transformChain));
                        
                        QString validationChain = "";
                        if (item.contains("validation_chain") && !item["validation_chain"].is_null()) {
                            validationChain = QString::fromStdString(item["validation_chain"].dump());
                        }
                        m_rulesTable->setItem(row, 7, new QTableWidgetItem(validationChain));
                        
                        m_rulesTable->setItem(row, 8, new QTableWidgetItem(QString::number(item.value("version", 0))));
                    }
                }
            } catch (...) {
                spdlog::error("JSON parse error on rules");
            }
        }
        m_rulesTable->resizeColumnsToContents();
        reply->deleteLater();
    });
}

void TransformationWidget::onRefreshTransformations() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/transformation/transformations");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    auto reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            m_transformTable->setRowCount(0);
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    for (const auto& item : data) {
                        int row = m_transformTable->rowCount();
                        m_transformTable->insertRow(row);
                        m_transformTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(item.value("id", ""))));
                        m_transformTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(item.value("name", ""))));
                        m_transformTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(item.value("description", ""))));
                        
                        QString params = "";
                        if (item.contains("parameters") && !item["parameters"].is_null()) {
                            params = QString::fromStdString(item["parameters"].dump());
                        }
                        m_transformTable->setItem(row, 3, new QTableWidgetItem(params));
                        m_transformTable->setItem(row, 4, new QTableWidgetItem(QString::number(item.value("version", 0))));
                    }
                }
            } catch (...) {
                spdlog::error("JSON parse error on transformations");
            }
        }
        m_transformTable->resizeColumnsToContents();
        reply->deleteLater();
    });
}

void TransformationWidget::onRefreshValidations() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/transformation/validations");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", getAuthHeader().toLocal8Bit());
    
    auto reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            m_validTable->setRowCount(0);
            try {
                json data = json::parse(reply->readAll().toStdString());
                if (data.is_array()) {
                    for (const auto& item : data) {
                        int row = m_validTable->rowCount();
                        m_validTable->insertRow(row);
                        m_validTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(item.value("id", ""))));
                        m_validTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(item.value("name", ""))));
                        m_validTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(item.value("description", ""))));
                        
                        QString params = "";
                        if (item.contains("parameters") && !item["parameters"].is_null()) {
                            params = QString::fromStdString(item["parameters"].dump());
                        }
                        m_validTable->setItem(row, 3, new QTableWidgetItem(params));
                        m_validTable->setItem(row, 4, new QTableWidgetItem(QString::number(item.value("version", 0))));
                    }
                }
            } catch (...) {
                spdlog::error("JSON parse error on validations");
            }
        }
        m_validTable->resizeColumnsToContents();
        reply->deleteLater();
    });
}

// --- CRUD Impl ---

QString getSelectedId(QTableWidget* table) {
    auto items = table->selectedItems();
    if (items.isEmpty()) return "";
    return table->item(items.first()->row(), 0)->text();
}

// Sources
void TransformationWidget::onAddSource() { onEditSource(); }
void TransformationWidget::onEditSource() {
    QString id = "";
    QString name = "", type = "", topic = "";
    if (sender() == m_editSourceBtn) {
        id = getSelectedId(m_sourcesTable);
        if (id.isEmpty()) return;
        int r = m_sourcesTable->selectedItems().first()->row();
        name = m_sourcesTable->item(r, 1)->text();
        type = m_sourcesTable->item(r, 2)->text();
        topic = m_sourcesTable->item(r, 3)->text();
    }
    
    QDialog dlg(this); dlg.setWindowTitle(id.isEmpty() ? "Add Source" : "Edit Source");
    auto l = new QFormLayout(&dlg);
    auto eName = new QLineEdit(name); l->addRow("Name:", eName);
    auto eType = new QLineEdit(type); l->addRow("Type (oracle/csv/api):", eType);
    auto eTopic = new QLineEdit(topic); l->addRow("Topic:", eTopic);
    auto bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    l->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    
    if (dlg.exec() == QDialog::Accepted) {
        json j; if(!id.isEmpty()) j["id"] = id.toStdString();
        j["name"] = eName->text().toStdString();
        j["type"] = eType->text().toStdString();
        j["topic"] = eTopic->text().toStdString();
        postEntity("/admin/transformation/sources", j, [this](){ onRefreshSources(); });
    }
}
void TransformationWidget::onDeleteSource() {
    QString id = getSelectedId(m_sourcesTable);
    if (!id.isEmpty() && QMessageBox::question(this, "Confirm", "Delete this source?") == QMessageBox::Yes)
        deleteEntity("/admin/transformation/sources", id, [this](){ onRefreshSources(); });
}

// Targets
void TransformationWidget::onAddTarget() { onEditTarget(); }
void TransformationWidget::onEditTarget() {
    QString id = "", topic = "", field = "", dtype = "";
    bool req = false, enc = false;
    if (sender() == m_editTargetBtn) {
        id = getSelectedId(m_targetsTable);
        if (id.isEmpty()) return;
        int r = m_targetsTable->selectedItems().first()->row();
        topic = m_targetsTable->item(r, 1)->text();
        field = m_targetsTable->item(r, 2)->text();
        dtype = m_targetsTable->item(r, 3)->text();
        req = m_targetsTable->item(r, 4)->text() == "Yes";
        enc = m_targetsTable->item(r, 5)->text() == "Yes";
    }
    
    QDialog dlg(this); dlg.setWindowTitle(id.isEmpty() ? "Add Target" : "Edit Target");
    auto l = new QFormLayout(&dlg);
    auto eTop = new QLineEdit(topic); l->addRow("Topic:", eTop);
    auto eFld = new QLineEdit(field); l->addRow("Field Name:", eFld);
    auto eTyp = new QLineEdit(dtype); l->addRow("Data Type:", eTyp);
    auto eReq = new QCheckBox(); eReq->setChecked(req); l->addRow("Required:", eReq);
    auto eEnc = new QCheckBox(); eEnc->setChecked(enc); l->addRow("Encrypted:", eEnc);
    auto bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    l->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    
    if (dlg.exec() == QDialog::Accepted) {
        json j; if(!id.isEmpty()) j["id"] = id.toStdString();
        j["topic"] = eTop->text().toStdString();
        j["field_name"] = eFld->text().toStdString();
        j["data_type"] = eTyp->text().toStdString();
        j["is_required"] = eReq->isChecked();
        j["encrypted"] = eEnc->isChecked();
        postEntity("/admin/transformation/targets", j, [this](){ onRefreshTargets(); });
    }
}
void TransformationWidget::onDeleteTarget() {
    QString id = getSelectedId(m_targetsTable);
    if (!id.isEmpty() && QMessageBox::question(this, "Confirm", "Delete this target?") == QMessageBox::Yes)
        deleteEntity("/admin/transformation/targets", id, [this](){ onRefreshTargets(); });
}

// Rules
void TransformationWidget::onAddRule() { onEditRule(); }
void TransformationWidget::onEditRule() {
    QString id = "", srcId = "", tgtId = "", srcFld = "", prio = "1", tCh = "[]", vCh = "[]";
    if (sender() == m_editRuleBtn) {
        id = getSelectedId(m_rulesTable);
        if (id.isEmpty()) return;
        int r = m_rulesTable->selectedItems().first()->row();
        srcId = m_rulesTable->item(r, 1)->text();
        tgtId = m_rulesTable->item(r, 2)->text();
        srcFld = m_rulesTable->item(r, 3)->text();
        prio = m_rulesTable->item(r, 5)->text();
        tCh = m_rulesTable->item(r, 6)->text();
        vCh = m_rulesTable->item(r, 7)->text();
    }
    
    RuleEditorDialog dlg(m_sourcesTable, m_targetsTable, id, srcId, tgtId, srcFld, prio, tCh, vCh, this);
    if (dlg.exec() == QDialog::Accepted) {
        postEntity("/admin/transformation/rules", dlg.getRuleJson(), [this](){ onRefreshRules(); });
    }
}
void TransformationWidget::onDeleteRule() {
    QString id = getSelectedId(m_rulesTable);
    if (!id.isEmpty() && QMessageBox::question(this, "Confirm", "Delete this rule?") == QMessageBox::Yes)
        deleteEntity("/admin/transformation/rules", id, [this](){ onRefreshRules(); });
}

// Transformations
void TransformationWidget::onAddTransformation() { onEditTransformation(); }
void TransformationWidget::onEditTransformation() {
    QString id = "", name = "", desc = "", param = "{}";
    if (sender() == m_editTransformBtn) {
        id = getSelectedId(m_transformTable);
        if (id.isEmpty()) return;
        int r = m_transformTable->selectedItems().first()->row();
        name = m_transformTable->item(r, 1)->text();
        desc = m_transformTable->item(r, 2)->text();
        param = m_transformTable->item(r, 3)->text();
    }
    
    QDialog dlg(this); dlg.setWindowTitle(id.isEmpty() ? "Add Transformation" : "Edit Transformation");
    auto l = new QFormLayout(&dlg);
    auto eName = new QLineEdit(name); l->addRow("Name:", eName);
    auto eDesc = new QLineEdit(desc); l->addRow("Description:", eDesc);
    auto ePar = new QTextEdit(param); l->addRow("Parameters (JSON):", ePar);
    auto bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    l->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    
    if (dlg.exec() == QDialog::Accepted) {
        json j; if(!id.isEmpty()) j["id"] = id.toStdString();
        j["name"] = eName->text().toStdString();
        j["description"] = eDesc->text().toStdString();
        try { j["parameters"] = json::parse(ePar->toPlainText().toStdString()); } catch(...) { j["parameters"] = json::object(); }
        postEntity("/admin/transformation/transformations", j, [this](){ onRefreshTransformations(); });
    }
}
void TransformationWidget::onDeleteTransformation() {
    QString id = getSelectedId(m_transformTable);
    if (!id.isEmpty() && QMessageBox::question(this, "Confirm", "Delete this transformation?") == QMessageBox::Yes)
        deleteEntity("/admin/transformation/transformations", id, [this](){ onRefreshTransformations(); });
}

// Validations
void TransformationWidget::onAddValidation() { onEditValidation(); }
void TransformationWidget::onEditValidation() {
    QString id = "", name = "", desc = "", param = "{}";
    if (sender() == m_editValidBtn) {
        id = getSelectedId(m_validTable);
        if (id.isEmpty()) return;
        int r = m_validTable->selectedItems().first()->row();
        name = m_validTable->item(r, 1)->text();
        desc = m_validTable->item(r, 2)->text();
        param = m_validTable->item(r, 3)->text();
    }
    
    QDialog dlg(this); dlg.setWindowTitle(id.isEmpty() ? "Add Validation" : "Edit Validation");
    auto l = new QFormLayout(&dlg);
    auto eName = new QLineEdit(name); l->addRow("Name:", eName);
    auto eDesc = new QLineEdit(desc); l->addRow("Description:", eDesc);
    auto ePar = new QTextEdit(param); l->addRow("Parameters (JSON):", ePar);
    auto bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    l->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    
    if (dlg.exec() == QDialog::Accepted) {
        json j; if(!id.isEmpty()) j["id"] = id.toStdString();
        j["name"] = eName->text().toStdString();
        j["description"] = eDesc->text().toStdString();
        try { j["parameters"] = json::parse(ePar->toPlainText().toStdString()); } catch(...) { j["parameters"] = json::object(); }
        postEntity("/admin/transformation/validations", j, [this](){ onRefreshValidations(); });
    }
}
void TransformationWidget::onDeleteValidation() {
    QString id = getSelectedId(m_validTable);
    if (!id.isEmpty() && QMessageBox::question(this, "Confirm", "Delete this validation?") == QMessageBox::Yes)
        deleteEntity("/admin/transformation/validations", id, [this](){ onRefreshValidations(); });
}
