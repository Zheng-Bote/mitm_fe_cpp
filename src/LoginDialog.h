/**
 * SPDX-FileComment: WebAuthn Login & Registration Dialog
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: Antigravity
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file LoginDialog.h
 * @brief Interactive Qt dialog for FIDO2/WebAuthn Login and User Registration.
 */
#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QLabel>
#include <QComboBox>
#include <QProgressBar>

namespace mitm::ui {

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr, bool registerOnly = false, const QString& prefillUsername = QString());
    ~LoginDialog() override = default;

private slots:
    void onSignInWithPasskey();
    void onRegisterPasskey();
    void onBrowserOAuthSignIn();
    void checkBootstrapStatus();

private:
    void setupUi();
    QWidget* createSignInTab();
    QWidget* createRegisterTab();
    void setStatus(const QString& msg, bool isError = false);
    void loadUserProfile(const QString& username);
    std::optional<QString> promptForPinIfNeeded(QWidget* parentWidget, const QString& existingPin, bool isRetry = false);

    QTabWidget* tabWidget_{nullptr};
    QLabel* statusLabel_{nullptr};
    QProgressBar* progressBar_{nullptr};

    // Sign In Controls
    QLineEdit* loginIdInput_{nullptr};
    QLineEdit* pinInput_{nullptr};
    QPushButton* signInBtn_{nullptr};
    QPushButton* browserAuthBtn_{nullptr};

    // Register Controls
    QLineEdit* regLoginIdInput_{nullptr};
    QLineEdit* firstNameInput_{nullptr};
    QLineEdit* lastNameInput_{nullptr};
    QComboBox* departmentCombo_{nullptr};
    QLineEdit* bootstrapTokenInput_{nullptr};
    QWidget* bootstrapContainer_{nullptr};
    QComboBox* authenticatorTypeCombo_{nullptr};
    QLineEdit* regPinInput_{nullptr};
    QPushButton* registerBtn_{nullptr};

    bool isBootstrapMode_{false};
    bool registerOnly_{false};
    QString prefillUsername_{};
};

} // namespace mitm::ui
