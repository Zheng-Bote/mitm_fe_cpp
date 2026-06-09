/**
 * SPDX-FileComment: RuleEditorDialog
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <nlohmann/json.hpp>

class RuleEditorDialog : public QDialog {
    Q_OBJECT
public:
    RuleEditorDialog(QTableWidget* sourcesTable, QTableWidget* targetsTable,
                     const QString& id, const QString& srcId, const QString& tgtId,
                     const QString& srcFld, const QString& prio,
                     const QString& tCh, const QString& vCh,
                     QWidget *parent = nullptr);

    nlohmann::json getRuleJson() const;

private slots:
    void onBuildTransformChain();
    void onBuildValidationChain();
    void onTestRule();

private:
    QString m_id;
    QComboBox* m_sourceCombo;
    QComboBox* m_targetCombo;
    QLineEdit* m_sourceFieldEdit;
    QLineEdit* m_priorityEdit;
    QTextEdit* m_transformEdit;
    QTextEdit* m_validationEdit;

    // Helper map to store UUIDs mapped to combo box indices
    QStringList m_sourceIds;
    QStringList m_targetIds;
};
