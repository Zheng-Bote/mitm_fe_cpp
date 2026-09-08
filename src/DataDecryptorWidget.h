/**
 * SPDX-FileComment: Data Decryptor Widget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: Antigravity
 * SPDX-FileCopyrightText: 2026 Antigravity
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file DataDecryptorWidget.h
 * @brief Widget for Envelope Decryption in Frontend
 * @version 1.0.0
 * @date 2026-09-08
 *
 * @author Antigravity
 * @copyright Copyright (c) 2026 Antigravity
 * @/home/zb_bamboo/DEV/__NEW__/Go/mitm-2/LICENSE Apache-2.0
 */

#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>

namespace mitm::ui {

class DataDecryptorWidget : public QWidget {
    Q_OBJECT
public:
    explicit DataDecryptorWidget(QWidget* parent = nullptr);

private slots:
    void onDecryptClicked();

private:
    void performDecryption(const QString& encryptedJsonText, const QString& masterKeyStr, const QJsonArray& keysArray);

    QLineEdit* m_masterKeyInput;
    QTextEdit* m_encryptedInput;
    QTextEdit* m_decryptedOutput;
    QPushButton* m_decryptButton;
};

} // namespace mitm::ui
