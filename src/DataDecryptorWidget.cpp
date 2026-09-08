/**
 * SPDX-FileComment: Data Decryptor Widget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: Antigravity
 * SPDX-FileCopyrightText: 2026 Antigravity
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file DataDecryptorWidget.cpp
 * @brief Widget for Envelope Decryption in Frontend
 * @version 1.0.0
 * @date 2026-09-08
 *
 * @author Antigravity
 * @copyright Copyright (c) 2026 Antigravity
 * @/home/zb_bamboo/DEV/__NEW__/Go/mitm-2/LICENSE Apache-2.0
 */

#include "DataDecryptorWidget.h"
#include "ApiClient.h"
#include "Crypto.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QNetworkReply>

namespace mitm::ui {

DataDecryptorWidget::DataDecryptorWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Master Key
    QHBoxLayout* keyLayout = new QHBoxLayout();
    QLabel* keyLabel = new QLabel(tr("MASTER_KEY:"), this);
    m_masterKeyInput = new QLineEdit(this);
    m_masterKeyInput->setEchoMode(QLineEdit::Password);
    m_masterKeyInput->setPlaceholderText(tr("Enter 32-byte Key Encryption Key"));
    keyLayout->addWidget(keyLabel);
    keyLayout->addWidget(m_masterKeyInput);
    mainLayout->addLayout(keyLayout);

    // Encrypted JSON
    QLabel* encLabel = new QLabel(tr("Encrypted JSON (must contain nonce and ciphertext):"), this);
    mainLayout->addWidget(encLabel);
    m_encryptedInput = new QTextEdit(this);
    m_encryptedInput->setPlaceholderText(tr("{\"nonce\": \"...\", \"ciphertext\": \"...\"}"));
    mainLayout->addWidget(m_encryptedInput);

    // Decrypt Button
    m_decryptButton = new QPushButton(tr("Decrypt"), this);
    mainLayout->addWidget(m_decryptButton);
    connect(m_decryptButton, &QPushButton::clicked, this, &DataDecryptorWidget::onDecryptClicked);

    // Decrypted Output
    QLabel* decLabel = new QLabel(tr("Decrypted Output:"), this);
    mainLayout->addWidget(decLabel);
    m_decryptedOutput = new QTextEdit(this);
    m_decryptedOutput->setReadOnly(true);
    mainLayout->addWidget(m_decryptedOutput);
}

void DataDecryptorWidget::onDecryptClicked()
{
    m_decryptedOutput->clear();

    QString encryptedText = m_encryptedInput->toPlainText().trimmed();
    if (encryptedText.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Encrypted JSON cannot be empty."));
        return;
    }

    QString masterKey = m_masterKeyInput->text();
    if (masterKey.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("MASTER_KEY cannot be empty."));
        return;
    }

    m_decryptButton->setEnabled(false);

    // Fetch storage keys
    mitm::api::ApiClient::instance().get("/admin/storage-keys",
        [this, encryptedText, masterKey](const QByteArray& response, QNetworkReply*) {
            m_decryptButton->setEnabled(true);
            QJsonParseError parseErr;
            QJsonDocument doc = QJsonDocument::fromJson(response, &parseErr);
            if (parseErr.error != QJsonParseError::NoError || !doc.isArray()) {
                m_decryptedOutput->setPlainText(tr("API Error: Invalid response from server for storage keys."));
                return;
            }

            performDecryption(encryptedText, masterKey, doc.array());
        },
        [this](int statusCode, const QString& errorStr) {
            m_decryptButton->setEnabled(true);
            m_decryptedOutput->setPlainText(tr("API Error: Failed to fetch storage keys:\n") + errorStr);
        }
    );
}

void DataDecryptorWidget::performDecryption(const QString& encryptedJsonText, const QString& masterKeyStr, const QJsonArray& keysArray)
{
    QJsonParseError parseErr;
    QJsonDocument inputDoc = QJsonDocument::fromJson(encryptedJsonText.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !inputDoc.isObject()) {
        m_decryptedOutput->setPlainText(tr("Error: Input is not a valid JSON object."));
        return;
    }
    QJsonObject inputObj = inputDoc.object();
    if (!inputObj.contains("nonce") || !inputObj.contains("ciphertext")) {
        m_decryptedOutput->setPlainText(tr("Error: JSON payload missing expected 'nonce' or 'ciphertext' structure."));
        return;
    }

    QByteArray payloadNonce = QByteArray::fromBase64(inputObj["nonce"].toString().toUtf8());
    QByteArray payload = QByteArray::fromBase64(inputObj["ciphertext"].toString().toUtf8());
    QByteArray kek = QByteArray::fromBase64(masterKeyStr.toUtf8());

    // If master key is not base64, assume raw text? The Go code uses raw text for AES usually if passed from CLI or base64. 
    // Wait, the Go code uses MASTER_KEY as KEK directly. It usually base64 decodes it first. 
    // Let's assume KEK is passed as base64 in the UI, or just UTF8. The issue says "A password input field for the MASTER_KEY".
    // I will try raw text first, if it fails maybe it's base64. Let's provide a raw vector.
    std::vector<uint8_t> kekVec(masterKeyStr.toUtf8().begin(), masterKeyStr.toUtf8().end());
    // In Go, base64 is standard for MASTER_KEY env var, but let's just parse it as base64 if possible.
    QByteArray decodedKek = QByteArray::fromBase64(masterKeyStr.toUtf8());
    if (decodedKek.length() == 32) {
        kekVec = std::vector<uint8_t>(decodedKek.begin(), decodedKek.end());
    }

    std::vector<uint8_t> pNonceVec(payloadNonce.begin(), payloadNonce.end());
    std::vector<uint8_t> pVec(payload.begin(), payload.end());

    for (const QJsonValue& val : keysArray) {
        QString wrappedKeyStr = val.toString();
        QByteArray wrappedKey = QByteArray::fromBase64(wrappedKeyStr.toUtf8());
        std::vector<uint8_t> wKeyVec(wrappedKey.begin(), wrappedKey.end());

        try {
            std::vector<uint8_t> plaintext = mitm::crypto::EnvelopeDecrypt(kekVec, wKeyVec, pNonceVec, pVec);
            if (plaintext.empty()) {
                continue;
            }
            
            // Decryption succeeded
            QByteArray plainBytes(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());
            QJsonDocument outDoc = QJsonDocument::fromJson(plainBytes);
            if (!outDoc.isNull()) {
                m_decryptedOutput->setPlainText(outDoc.toJson(QJsonDocument::Indented));
            } else {
                m_decryptedOutput->setPlainText(QString::fromUtf8(plainBytes));
            }
            return;
        } catch (...) {
            // Failed with this key, try next
            continue;
        }
    }

    m_decryptedOutput->setPlainText(tr("Decryption failed: Invalid MASTER_KEY or wrong data."));
}

} // namespace mitm::ui
