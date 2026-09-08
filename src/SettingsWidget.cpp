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
    
    m_statusLabel = new QLabel("Vault Status: <b>Locked</b>", this);
    mainLayout->addWidget(m_statusLabel);

    auto formLayout = new QFormLayout();
    
    m_masterKeyInput = new QLineEdit(this);
    m_masterKeyInput->setEchoMode(QLineEdit::Password);
    m_masterKeyInput->setPlaceholderText("Enter MASTER_KEY or use Windows Hello to unlock...");
    
    formLayout->addRow("Master Key (KEK):", m_masterKeyInput);
    
    auto buttonLayout = new QHBoxLayout();
    m_unlockButton = new QPushButton("Unlock Vault", this);
    m_lockButton = new QPushButton("Lock the vault", this);
    buttonLayout->addWidget(m_unlockButton);
    buttonLayout->addWidget(m_lockButton);
    buttonLayout->addStretch();
    
    formLayout->addRow("", buttonLayout);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    connect(m_unlockButton, &QPushButton::clicked, this, &SettingsWidget::onUnlockVault);
    connect(m_lockButton, &QPushButton::clicked, this, &SettingsWidget::onLockVault);
    
    updateUI();
}

void SettingsWidget::updateUI() {
    bool isUnlocked = !m_vaultKey.empty();
    
    m_masterKeyInput->setEnabled(!isUnlocked);
    m_unlockButton->setEnabled(!isUnlocked);
    m_lockButton->setEnabled(isUnlocked);
    
    if (isUnlocked) {
        m_statusLabel->setText("Vault Status: <b style='color: green;'>Unlocked</b>");
        m_masterKeyInput->clear();
        m_masterKeyInput->setPlaceholderText("Vault is unlocked.");
    } else {
        m_statusLabel->setText("Vault Status: <b style='color: red;'>Locked</b>");
        m_masterKeyInput->clear();
        m_masterKeyInput->setPlaceholderText("Enter MASTER_KEY or use Windows Hello to unlock...");
    }
}

void SettingsWidget::onUnlockVault() {
    if (m_masterKeyInput->text().isEmpty()) {
        QMessageBox::warning(this, "Key Vault", "Please enter the Master Key.");
        return;
    }
    
    // In a real implementation, we would validate the key here against a known test payload
    // before accepting it into the vault. For now, we simply store it.
    std::string keyStr = m_masterKeyInput->text().toStdString();
    m_vaultKey = mitm::crypto::SecureString(keyStr.begin(), keyStr.end());
    
    // Clear the string from regular memory
    // Note: QString/std::string might leave traces, but we do our best here
    m_masterKeyInput->clear();
    
    spdlog::info("Master Key provided. Vault unlocked in memory.");
    updateUI();
}

void SettingsWidget::onLockVault() {
    // Securely clear the vault key
    m_vaultKey.clear();
    
    spdlog::info("Vault locked. Master Key securely wiped from memory.");
    updateUI();
}
