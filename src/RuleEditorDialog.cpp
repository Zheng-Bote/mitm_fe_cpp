/**
 * SPDX-FileComment: RuleEditorDialog
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RuleEditorDialog.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>

using json = nlohmann::json;

RuleEditorDialog::RuleEditorDialog(QTableWidget* sourcesTable, QTableWidget* targetsTable,
                                   const QString& id, const QString& srcId, const QString& tgtId,
                                   const QString& srcFld, const QString& prio,
                                   const QString& tCh, const QString& vCh,
                                   QWidget *parent)
    : QDialog(parent), m_id(id) {
    
    setWindowTitle(id.isEmpty() ? "Add Rule" : "Edit Rule");
    resize(600, 500);

    auto mainLayout = new QVBoxLayout(this);
    auto formLayout = new QFormLayout();

    m_sourceCombo = new QComboBox(this);
    int srcIndex = 0;
    for (int i = 0; i < sourcesTable->rowCount(); ++i) {
        QString sId = sourcesTable->item(i, 0)->text();
        QString sName = sourcesTable->item(i, 1)->text();
        QString sType = sourcesTable->item(i, 2)->text();
        m_sourceIds.append(sId);
        m_sourceCombo->addItem(sName + " (" + sType + ")");
        if (sId == srcId) srcIndex = i;
    }
    if (sourcesTable->rowCount() > 0) m_sourceCombo->setCurrentIndex(srcIndex);

    m_targetCombo = new QComboBox(this);
    int tgtIndex = 0;
    for (int i = 0; i < targetsTable->rowCount(); ++i) {
        QString tId = targetsTable->item(i, 0)->text();
        QString tTopic = targetsTable->item(i, 1)->text();
        QString tField = targetsTable->item(i, 2)->text();
        m_targetIds.append(tId);
        m_targetCombo->addItem(tTopic + " -> " + tField);
        if (tId == tgtId) tgtIndex = i;
    }
    if (targetsTable->rowCount() > 0) m_targetCombo->setCurrentIndex(tgtIndex);

    m_sourceFieldEdit = new QLineEdit(srcFld, this);
    m_priorityEdit = new QLineEdit(prio.isEmpty() ? "1" : prio, this);
    
    m_transformEdit = new QTextEdit(tCh.isEmpty() ? "[]" : tCh, this);
    auto tLayout = new QHBoxLayout();
    tLayout->addWidget(m_transformEdit);
    auto tBuildBtn = new QPushButton("Builder...", this);
    connect(tBuildBtn, &QPushButton::clicked, this, &RuleEditorDialog::onBuildTransformChain);
    tLayout->addWidget(tBuildBtn);

    m_validationEdit = new QTextEdit(vCh.isEmpty() ? "[]" : vCh, this);
    auto vLayout = new QHBoxLayout();
    vLayout->addWidget(m_validationEdit);
    auto vBuildBtn = new QPushButton("Builder...", this);
    connect(vBuildBtn, &QPushButton::clicked, this, &RuleEditorDialog::onBuildValidationChain);
    vLayout->addWidget(vBuildBtn);

    formLayout->addRow("Source:", m_sourceCombo);
    formLayout->addRow("Target Field:", m_targetCombo);
    formLayout->addRow("Source Field:", m_sourceFieldEdit);
    formLayout->addRow("Priority:", m_priorityEdit);
    formLayout->addRow("Transform Chain:", tLayout);
    formLayout->addRow("Validation Chain:", vLayout);

    mainLayout->addLayout(formLayout);

    // Sandbox / Test Area
    auto testBtn = new QPushButton("🧪 Test Rule (Live Preview)", this);
    connect(testBtn, &QPushButton::clicked, this, &RuleEditorDialog::onTestRule);
    mainLayout->addWidget(testBtn);

    auto bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(bb);
}

void RuleEditorDialog::onBuildTransformChain() {
    // Simple UI to add common transformations instead of raw JSON
    QStringList items;
    items << "uppercase" << "lowercase" << "trim" << "parse_date";
    bool ok;
    QString item = QInputDialog::getItem(this, "Add Transformation", "Select type:", items, 0, false, &ok);
    if (ok && !item.isEmpty()) {
        try {
            json j = json::parse(m_transformEdit->toPlainText().toStdString());
            if (!j.is_array()) j = json::array();
            json step = {{"type", item.toStdString()}};
            if (item == "parse_date") step["format"] = "YYYY-MM-DD"; // dummy param
            j.push_back(step);
            m_transformEdit->setPlainText(QString::fromStdString(j.dump(2)));
        } catch (...) {
            QMessageBox::warning(this, "Error", "Current Transform Chain is invalid JSON.");
        }
    }
}

void RuleEditorDialog::onBuildValidationChain() {
    QStringList items;
    items << "not_empty" << "is_email" << "min_length";
    bool ok;
    QString item = QInputDialog::getItem(this, "Add Validation", "Select type:", items, 0, false, &ok);
    if (ok && !item.isEmpty()) {
        try {
            json j = json::parse(m_validationEdit->toPlainText().toStdString());
            if (!j.is_array()) j = json::array();
            json step = {{"type", item.toStdString()}};
            if (item == "min_length") step["length"] = 5;
            j.push_back(step);
            m_validationEdit->setPlainText(QString::fromStdString(j.dump(2)));
        } catch (...) {
            QMessageBox::warning(this, "Error", "Current Validation Chain is invalid JSON.");
        }
    }
}

void RuleEditorDialog::onTestRule() {
    bool ok;
    QString input = QInputDialog::getText(this, "Test Rule", "Enter dummy source value:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    // Simulate simple transformations locally for immediate feedback
    QString current = input;
    try {
        json tChain = json::parse(m_transformEdit->toPlainText().toStdString());
        for (const auto& step : tChain) {
            std::string type = step.value("type", "");
            if (type == "uppercase") current = current.toUpper();
            else if (type == "lowercase") current = current.toLower();
            else if (type == "trim") current = current.trimmed();
        }
        
        QMessageBox::information(this, "Test Result", 
            QString("Input: '%1'\nOutput: '%2'").arg(input, current));

    } catch (...) {
        QMessageBox::critical(this, "Test Failed", "Invalid JSON in chains.");
    }
}

nlohmann::json RuleEditorDialog::getRuleJson() const {
    json j;
    if (!m_id.isEmpty()) j["id"] = m_id.toStdString();
    
    if (m_sourceCombo->currentIndex() >= 0)
        j["source_id"] = m_sourceIds[m_sourceCombo->currentIndex()].toStdString();
    if (m_targetCombo->currentIndex() >= 0)
        j["target_field_id"] = m_targetIds[m_targetCombo->currentIndex()].toStdString();

    j["source_field"] = m_sourceFieldEdit->text().toStdString();
    j["priority"] = m_priorityEdit->text().toInt();
    
    try { j["transformation_chain"] = json::parse(m_transformEdit->toPlainText().toStdString()); } 
    catch(...) { j["transformation_chain"] = json::array(); }
    
    try { j["validation_chain"] = json::parse(m_validationEdit->toPlainText().toStdString()); } 
    catch(...) { j["validation_chain"] = json::array(); }

    return j;
}
