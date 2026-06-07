/**
 * SPDX-FileComment: SettingsWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file SettingsWidget.cpp
 * @brief SettingsWidget
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "SettingsWidget.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <spdlog/spdlog.h>

SettingsWidget::SettingsWidget(QWidget *parent) : QWidget(parent) {
    auto mainLayout = new QVBoxLayout(this);

    auto title = new QLabel("<b>Settings & Key Vault</b>", this);
    title->setStyleSheet("font-size: 18px; margin-bottom: 20px;");
    mainLayout->addWidget(title);

    auto formLayout = new QFormLayout();
    
    m_masterKeyInput = new QLineEdit(this);
    m_masterKeyInput->setEchoMode(QLineEdit::Password);
    m_masterKeyInput->setPlaceholderText("Enter MASTER_KEY or use Windows Hello to unlock...");
    
    formLayout->addRow("Master Key (KEK):", m_masterKeyInput);
    
    m_unlockButton = new QPushButton("Unlock Vault", this);
    formLayout->addRow("", m_unlockButton);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    connect(m_unlockButton, &QPushButton::clicked, this, &SettingsWidget::onUnlockVault);
}

void SettingsWidget::onUnlockVault() {
    if (m_masterKeyInput->text().isEmpty()) {
        QMessageBox::warning(this, "Key Vault", "Please enter the Master Key.");
        return;
    }
    
    spdlog::info("Master Key provided. Vault unlocked. (Concept)");
    QMessageBox::information(this, "Key Vault", "Vault unlocked successfully!\nIn a real implementation, this would establish the secure context.");
    m_masterKeyInput->clear();
}
