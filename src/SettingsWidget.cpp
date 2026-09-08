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
#include <QJsonDocument>
#include <QJsonObject>
#include <spdlog/spdlog.h>
#include <sodium.h>
#include "Crypto.h"
#include "ApiClient.h"

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

    // --- Key Rotation UI ---
    auto rotationTitle = new QLabel("<b>Master-Key Rotation</b>", this);
    rotationTitle->setStyleSheet("font-size: 14px; margin-top: 20px; margin-bottom: 10px;");
    formLayout->addRow(rotationTitle);

    m_newKeyInput = new QLineEdit(this);
    m_newKeyInput->setPlaceholderText("Generated 32-byte Base64 new Master-Key...");
    m_newKeyInput->setReadOnly(true);
    
    auto rotationButtonLayout = new QHBoxLayout();
    m_createKeyButton = new QPushButton("Create Master-Key", this);
    m_changeKeyButton = new QPushButton("Change Master-Key", this);
    rotationButtonLayout->addWidget(m_createKeyButton);
    rotationButtonLayout->addWidget(m_changeKeyButton);
    rotationButtonLayout->addStretch();
    
    formLayout->addRow("New Master-Key:", m_newKeyInput);
    formLayout->addRow("", rotationButtonLayout);
    // -----------------------

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    connect(m_unlockButton, &QPushButton::clicked, this, &SettingsWidget::onUnlockVault);
    connect(m_lockButton, &QPushButton::clicked, this, &SettingsWidget::onLockVault);
    connect(m_createKeyButton, &QPushButton::clicked, this, &SettingsWidget::onCreateMasterKey);
    connect(m_changeKeyButton, &QPushButton::clicked, this, &SettingsWidget::onChangeMasterKey);
    
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
void SettingsWidget::onCreateMasterKey() {
    std::vector<uint8_t> key(32);
    randombytes_buf(key.data(), key.size());
    QByteArray qKey(reinterpret_cast<const char*>(key.data()), key.size());
    m_newKeyInput->setText(QString(qKey.toBase64()));
    spdlog::info("Generated a new 32-byte Master Key.");
}

void SettingsWidget::onChangeMasterKey() {
    if (m_vaultKey.empty()) {
        QMessageBox::warning(this, "Error", "Vault is locked. You must unlock it first.");
        return;
    }
    if (m_newKeyInput->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Generate a new Master-Key first.");
        return;
    }

    auto reply = QMessageBox::question(this, "Warning", 
        "Are you sure you want to change the Master-Key? This will trigger a re-encryption of all keys in the database.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) return;

    QByteArray newKeyBytes = QByteArray::fromBase64(m_newKeyInput->text().toUtf8());
    if (newKeyBytes.size() != 32) {
        QMessageBox::critical(this, "Error", "New Master-Key must be 32 bytes.");
        return;
    }

    std::vector<uint8_t> currentKey(m_vaultKey.begin(), m_vaultKey.end());
    if (currentKey.size() != 32) {
        QMessageBox::critical(this, "Error", "Current Master-Key is not 32 bytes.");
        return;
    }

    if (crypto_aead_aes256gcm_is_available() == 0) {
        QMessageBox::critical(this, "Error", "Hardware AES-GCM is not available on this platform");
        return;
    }

    std::vector<uint8_t> nonce(12);
    randombytes_buf(nonce.data(), nonce.size());

    std::vector<uint8_t> ciphertext(newKeyBytes.size() + 16);
    unsigned long long ciphertext_len = 0;

    if (crypto_aead_aes256gcm_encrypt(ciphertext.data(), &ciphertext_len,
                                      reinterpret_cast<const uint8_t*>(newKeyBytes.constData()), newKeyBytes.size(),
                                      nullptr, 0, nullptr,
                                      nonce.data(), currentKey.data()) != 0) {
        QMessageBox::critical(this, "Error", "Encryption failed.");
        return;
    }
    ciphertext.resize(ciphertext_len);

    QJsonObject payload;
    payload["nonce"] = QString(QByteArray(reinterpret_cast<const char*>(nonce.data()), nonce.size()).toBase64());
    payload["ciphertext"] = QString(QByteArray(reinterpret_cast<const char*>(ciphertext.data()), ciphertext.size()).toBase64());

    QJsonDocument doc(payload);
    QByteArray data = doc.toJson();

    m_changeKeyButton->setEnabled(false);

    mitm::api::ApiClient::instance().post("/admin/key-rotation", data,
        [this](const QByteArray& response, QNetworkReply*) {
            spdlog::info("Master-Key rotated successfully.");
            QMessageBox::information(this, "Success", "Master-Key rotated successfully.");
            m_changeKeyButton->setEnabled(true);
            
            // On success, automatically lock the Key Vault
            onLockVault();
        },
        [this](int statusCode, const QString& errorString) {
            spdlog::error("Key rotation failed. HTTP {}: {}", statusCode, errorString.toStdString());
            QMessageBox::critical(this, "Error", QString("Key rotation failed: %1").arg(errorString));
            m_changeKeyButton->setEnabled(true);
        }
    );
}
