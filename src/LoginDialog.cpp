/**
 * SPDX-FileComment: WebAuthn Login & Registration Dialog
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: Antigravity
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file LoginDialog.cpp
 * @brief Implementation of interactive WebAuthn Login & Registration Dialog.
 */
#include "LoginDialog.h"
#include "FidoClient.h"
#include "Config.h"
#include "AuthManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <spdlog/spdlog.h>

namespace mitm::ui {

using mitm::auth::FidoClient;

LoginDialog::LoginDialog(QWidget* parent, bool registerOnly, const QString& prefillUsername) 
    : QDialog(parent), registerOnly_(registerOnly), prefillUsername_(prefillUsername) {
    if (registerOnly_) {
        setWindowTitle("MitM Admin - Register YubiKey / Hardware Token");
    } else {
        setWindowTitle("MitM Admin - WebAuth Portal");
    }
    setFixedSize(520, 620);
    setupUi();
    if (!registerOnly_) {
        checkBootstrapStatus();
    }
}

void LoginDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // Header Title
    auto* headerLayout = new QVBoxLayout();
    auto* titleLabel = new QLabel(registerOnly_ ? "Register Security Key" : "MitM Data Aggregator", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #4DA6FF;");
    auto* subTitleLabel = new QLabel(registerOnly_ ? "Add a new YubiKey or FIDO2 hardware token to your account" : "Authentication & Identity Management (WebAuthn / FIDO2)", this);
    subTitleLabel->setStyleSheet("font-size: 12px; color: #888888;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subTitleLabel);
    mainLayout->addLayout(headerLayout);

    // Tab Widget
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #333333; background: #1E1E1E; border-radius: 6px; } "
        "QTabBar::tab { background: #2A2A2A; color: #CCCCCC; padding: 10px 24px; font-weight: bold; border-top-left-radius: 4px; border-top-right-radius: 4px; } "
        "QTabBar::tab:selected { background: #1E1E1E; color: #4DA6FF; border-bottom: 2px solid #4DA6FF; }"
    );

    if (!registerOnly_) {
        tabWidget_->addTab(createSignInTab(), "Sign In");
    }
    tabWidget_->addTab(createRegisterTab(), registerOnly_ ? "Register Token" : "Register Passkey");
    mainLayout->addWidget(tabWidget_);

    // Progress Bar
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 0);
    progressBar_->setVisible(false);
    mainLayout->addWidget(progressBar_);

    // Status Label
    statusLabel_ = new QLabel("", this);
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setWordWrap(true);
    statusLabel_->setStyleSheet("font-size: 12px; padding: 6px; border-radius: 4px;");
    statusLabel_->setVisible(false);
    mainLayout->addWidget(statusLabel_);
}

QWidget* LoginDialog::createSignInTab() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    QString defaultUser = QProcessEnvironment::systemEnvironment().value(
        "USER", QProcessEnvironment::systemEnvironment().value("USERNAME", ""));

    loginIdInput_ = new QLineEdit(defaultUser, widget);
    loginIdInput_->setMaxLength(10);
    loginIdInput_->setPlaceholderText("e.g. Q1234567");
    loginIdInput_->setStyleSheet("padding: 8px; border: 1px solid #444; border-radius: 4px; background: #252525; color: #FFF; font-size: 13px;");
    formLayout->addRow(new QLabel("LoginID (Q-Number):", widget), loginIdInput_);

    pinInput_ = new QLineEdit(widget);
    pinInput_->setEchoMode(QLineEdit::Password);
    pinInput_->setPlaceholderText("Optional PIN / User Verification");
    pinInput_->setStyleSheet("padding: 8px; border: 1px solid #444; border-radius: 4px; background: #252525; color: #FFF; font-size: 13px;");
    formLayout->addRow(new QLabel("PIN (Optional):", widget), pinInput_);

    layout->addLayout(formLayout);

    signInBtn_ = new QPushButton("🔑 Sign In with Passkey (FIDO2)", widget);
    signInBtn_->setStyleSheet("padding: 12px; background: #0066CC; color: white; font-weight: bold; font-size: 14px; border-radius: 5px;");
    connect(signInBtn_, &QPushButton::clicked, this, &LoginDialog::onSignInWithPasskey);
    layout->addWidget(signInBtn_);

    browserAuthBtn_ = new QPushButton("🌐 Sign In via Browser (OAuth2 PKCE)", widget);
    browserAuthBtn_->setStyleSheet("padding: 10px; background: #333333; color: #DDDDDD; font-size: 13px; border-radius: 5px; border: 1px solid #444;");
    connect(browserAuthBtn_, &QPushButton::clicked, this, &LoginDialog::onBrowserOAuthSignIn);
    layout->addWidget(browserAuthBtn_);

    layout->addStretch();
    return widget;
}

QWidget* LoginDialog::createRegisterTab() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    QString defaultUser = !prefillUsername_.isEmpty() ? prefillUsername_ : QProcessEnvironment::systemEnvironment().value(
        "USER", QProcessEnvironment::systemEnvironment().value("USERNAME", ""));

    regLoginIdInput_ = new QLineEdit(defaultUser, widget);
    regLoginIdInput_->setMaxLength(10);
    regLoginIdInput_->setPlaceholderText("e.g. Q1234567");
    if (registerOnly_ && !prefillUsername_.isEmpty()) {
        regLoginIdInput_->setReadOnly(true);
    }
    regLoginIdInput_->setStyleSheet("padding: 6px; border: 1px solid #444; border-radius: 4px; background: #252525; color: #FFF;");
    formLayout->addRow(new QLabel("LoginID (Q-Number):", widget), regLoginIdInput_);

    firstNameInput_ = new QLineEdit(widget);
    firstNameInput_->setPlaceholderText("First Name");
    firstNameInput_->setStyleSheet("padding: 6px; border: 1px solid #444; border-radius: 4px; background: #252525; color: #FFF;");
    formLayout->addRow(new QLabel("First Name:", widget), firstNameInput_);

    lastNameInput_ = new QLineEdit(widget);
    lastNameInput_->setPlaceholderText("Last Name");
    lastNameInput_->setStyleSheet("padding: 6px; border: 1px solid #444; border-radius: 4px; background: #252525; color: #FFF;");
    formLayout->addRow(new QLabel("Last Name:", widget), lastNameInput_);

    departmentCombo_ = new QComboBox(widget);
    departmentCombo_->setEditable(true);
    departmentCombo_->addItems({"IT Security", "Engineering", "Finance", "Operations", "Administration"});
    departmentCombo_->setStyleSheet("padding: 6px; border: 1px solid #444; border-radius: 4px; background: #252525; color: #FFF;");
    formLayout->addRow(new QLabel("Department:", widget), departmentCombo_);

    // Bootstrap token section
    bootstrapContainer_ = new QWidget(widget);
    auto* bootLayout = new QVBoxLayout(bootstrapContainer_);
    bootLayout->setContentsMargins(0, 0, 0, 0);

    auto* bootBanner = new QLabel("👑 Initial Setup: Registering Bootstrap Administrator", bootstrapContainer_);
    bootBanner->setStyleSheet("background: #5A3A00; color: #FFD700; padding: 6px; font-weight: bold; border-radius: 4px;");
    bootLayout->addWidget(bootBanner);

    bootstrapTokenInput_ = new QLineEdit(bootstrapContainer_);
    bootstrapTokenInput_->setPlaceholderText("Bootstrap Token (INITIAL_ADMIN_TOKEN)");
    bootstrapTokenInput_->setStyleSheet("padding: 6px; border: 1px solid #D4AF37; border-radius: 4px; background: #252525; color: #FFF;");
    bootLayout->addWidget(bootstrapTokenInput_);

    bootstrapContainer_->setVisible(false);
    layout->addWidget(bootstrapContainer_);

    authenticatorTypeCombo_ = new QComboBox(widget);
    authenticatorTypeCombo_->addItems({
        "🔑 Security Key (YubiKey / USB / FIDO2)",
        "🛡️ Platform (Biometrics / Touch ID / Windows Hello)"
    });
    authenticatorTypeCombo_->setStyleSheet("padding: 6px; border: 1px solid #444; border-radius: 4px; background: #252525; color: #FFF;");
    formLayout->addRow(new QLabel("Authenticator:", widget), authenticatorTypeCombo_);

    regPinInput_ = new QLineEdit(widget);
    regPinInput_->setEchoMode(QLineEdit::Password);
    regPinInput_->setPlaceholderText("Optional PIN");
    regPinInput_->setStyleSheet("padding: 6px; border: 1px solid #444; border-radius: 4px; background: #252525; color: #FFF;");
    formLayout->addRow(new QLabel("PIN (Optional):", widget), regPinInput_);

    layout->addLayout(formLayout);

    registerBtn_ = new QPushButton(registerOnly_ ? "🔑 Register Hardware Token" : "🛡️ Register Passkey", widget);
    registerBtn_->setStyleSheet("padding: 12px; background: #2E7D32; color: white; font-weight: bold; font-size: 14px; border-radius: 5px;");
    connect(registerBtn_, &QPushButton::clicked, this, &LoginDialog::onRegisterPasskey);
    layout->addWidget(registerBtn_);

    layout->addStretch();
    return widget;
}

void LoginDialog::setStatus(const QString& msg, bool isError) {
    if (msg.isEmpty()) {
        statusLabel_->setVisible(false);
        return;
    }
    statusLabel_->setText(msg);
    statusLabel_->setStyleSheet(isError 
        ? "background: #551A1A; color: #FF9999; padding: 8px; border-radius: 4px; font-weight: bold;" 
        : "background: #1A441A; color: #99FF99; padding: 8px; border-radius: 4px;");
    statusLabel_->setVisible(true);
}

void LoginDialog::checkBootstrapStatus() {
    QNetworkAccessManager manager;
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QNetworkRequest req(QUrl(host + "/api/v1/auth/bootstrap/status"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = manager.get(req);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject() && doc.object().value("bootstrap_required").toBool()) {
            isBootstrapMode_ = true;
            bootstrapContainer_->setVisible(true);
            registerBtn_->setText("👑 Register Admin via Bootstrap Token");
            tabWidget_->setCurrentIndex(1); // Switch to registration tab directly
            setStatus("No admin exists. System is in Bootstrap Mode.", false);
        }
    }
    reply->deleteLater();
}

void LoginDialog::onSignInWithPasskey() {
    QString username = loginIdInput_->text().trimmed();
    if (username.isEmpty()) {
        setStatus("Please enter your LoginID.", true);
        return;
    }

    setStatus("Contacting authentication server...", false);
    progressBar_->setVisible(true);

    QNetworkAccessManager manager;
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();

    // 1. Call login/start
    QNetworkRequest startReq(QUrl(host + "/api/v1/auth/webauthn/login/start"));
    startReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject startPayload;
    startPayload["username"] = username;

    QNetworkReply* startReply = manager.post(startReq, QJsonDocument(startPayload).toJson());
    QEventLoop loop;
    connect(startReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (startReply->error() != QNetworkReply::NoError) {
        progressBar_->setVisible(false);
        int code = startReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray resp = startReply->readAll();
        auto doc = QJsonDocument::fromJson(resp);
        QString detail = "Authentication failed.";
        if (doc.isObject() && doc.object().contains("errors")) {
            auto errs = doc.object().value("errors").toArray();
            if (!errs.isEmpty()) {
                detail = errs[0].toObject().value("detail").toString();
            }
        }
        if (code == 403 || detail.contains("inactive", Qt::CaseInsensitive)) {
            setStatus("⛔ Account is inactive pending administrator approval.", true);
        } else {
            setStatus("Start Login Failed: " + detail, true);
        }
        startReply->deleteLater();
        return;
    }

    auto startDoc = QJsonDocument::fromJson(startReply->readAll());
    startReply->deleteLater();

    QJsonObject startObj = startDoc.object();
    QString ceremonyId = startObj.value("ceremony_id").toString();
    QJsonObject pkOptions = startObj.value("public_key_options").toObject();
    QJsonObject pk = pkOptions.value("publicKey").toObject();
    QString challenge = pk.value("challenge").toString();
    QString rpId = pk.value("rpId").toString();
    if (rpId.isEmpty()) rpId = "localhost";

    std::vector<QString> allowCreds;
    QJsonArray allowArr = pk.value("allowCredentials").toArray();
    for (const auto& c : allowArr) {
        allowCreds.push_back(c.toObject().value("id").toString());
    }

    setStatus("Touch your FIDO2 Security Key / Authenticator...", false);

    std::optional<QString> pin = pinInput_->text().isEmpty() ? std::nullopt : std::optional<QString>(pinInput_->text());
    auto assertRes = FidoClient::instance().assertCredential(rpId, challenge, allowCreds, pin);

    if (!assertRes) {
        progressBar_->setVisible(false);
        setStatus("FIDO2 Ceremony failed or was cancelled.", true);
        return;
    }

    // 2. Call login/finish
    QNetworkRequest finishReq(QUrl(host + "/api/v1/auth/webauthn/login/finish"));
    finishReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject finishPayload;
    finishPayload["ceremony_id"] = ceremonyId;

    QJsonObject credObj;
    credObj["id"] = assertRes->credentialIdBase64;
    credObj["rawId"] = assertRes->credentialIdBase64;
    credObj["type"] = "public-key";

    QJsonObject respObj;
    respObj["clientDataJSON"] = assertRes->clientDataJsonBase64;
    respObj["authenticatorData"] = assertRes->authenticatorDataBase64;
    respObj["signature"] = assertRes->signatureBase64;
    if (assertRes->userHandleBase64) {
        respObj["userHandle"] = *assertRes->userHandleBase64;
    }
    credObj["response"] = respObj;
    finishPayload["credential"] = credObj;

    QNetworkReply* finishReply = manager.post(finishReq, QJsonDocument(finishPayload).toJson());
    QEventLoop loop2;
    connect(finishReply, &QNetworkReply::finished, &loop2, &QEventLoop::quit);
    loop2.exec();

    progressBar_->setVisible(false);

    if (finishReply->error() == QNetworkReply::NoError) {
        auto finishDoc = QJsonDocument::fromJson(finishReply->readAll());
        QString sessionToken = finishDoc.object().value("session_token").toString();
        
        std::string rawStr = sessionToken.toStdString();
        mitm::crypto::SecureString secureToken(rawStr.begin(), rawStr.end());
        mitm::config::ConfigManager::GetInstance().SetSessionToken(secureToken);

        // Fetch user roles
        mitm::auth::AuthManager::instance().performLogin(true);

        setStatus("Login Successful! Welcome.", false);
        accept();
    } else {
        int code = finishReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == 403) {
            setStatus("⛔ Account is inactive pending administrator approval.", true);
        } else {
            setStatus("Passkey verification failed.", true);
        }
    }
    finishReply->deleteLater();
}

void LoginDialog::onRegisterPasskey() {
    QString username = regLoginIdInput_->text().trimmed();
    if (username.isEmpty()) {
        setStatus("Please enter a LoginID.", true);
        return;
    }

    setStatus("Starting registration ceremony...", false);
    progressBar_->setVisible(true);

    QNetworkAccessManager manager;
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();

    // 1. Call register/start
    QNetworkRequest startReq(QUrl(host + "/api/v1/auth/webauthn/register/start"));
    startReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject startPayload;
    startPayload["username"] = username;
    startPayload["first_name"] = firstNameInput_->text().trimmed();
    startPayload["last_name"] = lastNameInput_->text().trimmed();
    startPayload["department"] = departmentCombo_->currentText().trimmed();

    if (isBootstrapMode_) {
        startPayload["bootstrap_token"] = bootstrapTokenInput_->text().trimmed();
    }

    QNetworkReply* startReply = manager.post(startReq, QJsonDocument(startPayload).toJson());
    QEventLoop loop;
    connect(startReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (startReply->error() != QNetworkReply::NoError) {
        progressBar_->setVisible(false);
        QByteArray resp = startReply->readAll();
        auto doc = QJsonDocument::fromJson(resp);
        QString detail = "Registration initiation failed.";
        if (doc.isObject() && doc.object().contains("errors")) {
            auto errs = doc.object().value("errors").toArray();
            if (!errs.isEmpty()) {
                detail = errs[0].toObject().value("detail").toString();
            }
        }
        setStatus(detail, true);
        startReply->deleteLater();
        return;
    }

    auto startDoc = QJsonDocument::fromJson(startReply->readAll());
    startReply->deleteLater();

    QJsonObject startObj = startDoc.object();
    QString ceremonyId = startObj.value("ceremony_id").toString();
    QJsonObject pkOptions = startObj.value("public_key_options").toObject();
    QJsonObject pk = pkOptions.value("publicKey").toObject();
    QString challenge = pk.value("challenge").toString();
    QJsonObject rpObj = pk.value("rp").toObject();
    QString rpId = rpObj.value("id").toString();
    if (rpId.isEmpty()) rpId = "localhost";
    QString rpName = rpObj.value("name").toString();
    if (rpName.isEmpty()) rpName = "MitM Aggregator";

    setStatus("Please insert and touch your Security Key...", false);

    std::optional<QString> pin = regPinInput_->text().isEmpty() ? std::nullopt : std::optional<QString>(regPinInput_->text());
    auto regRes = FidoClient::instance().registerCredential(
        rpId, rpName, challenge, username,
        firstNameInput_->text().trimmed() + " " + lastNameInput_->text().trimmed(),
        pin
    );

    if (!regRes) {
        progressBar_->setVisible(false);
        setStatus("FIDO2 Passkey creation failed or timed out.", true);
        return;
    }

    // 2. Call register/finish
    QNetworkRequest finishReq(QUrl(host + "/api/v1/auth/webauthn/register/finish"));
    finishReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject finishPayload;
    finishPayload["ceremony_id"] = ceremonyId;

    QJsonObject credObj;
    credObj["id"] = regRes->credentialIdBase64;
    credObj["rawId"] = regRes->credentialIdBase64;
    credObj["type"] = "public-key";

    QJsonObject respObj;
    respObj["clientDataJSON"] = regRes->clientDataJsonBase64;
    respObj["attestationObject"] = regRes->attestationObjectBase64;
    credObj["response"] = respObj;

    finishPayload["credential"] = credObj;
    finishPayload["attachment_type"] = regRes->attachmentType;
    if (regRes->yubikeySerial) {
        finishPayload["yubikey_serial"] = *regRes->yubikeySerial;
    }

    QNetworkReply* finishReply = manager.post(finishReq, QJsonDocument(finishPayload).toJson());
    QEventLoop loop2;
    connect(finishReply, &QNetworkReply::finished, &loop2, &QEventLoop::quit);
    loop2.exec();

    progressBar_->setVisible(false);

    if (finishReply->error() == QNetworkReply::NoError) {
        auto finishDoc = QJsonDocument::fromJson(finishReply->readAll());
        QString status = finishDoc.object().value("status").toString();
        QString message = finishDoc.object().value("message").toString();

        if (status == "active") {
            if (registerOnly_) {
                QString successMsg = "Security key registered successfully!";
                if (regRes->yubikeySerial) {
                    successMsg += QString("\nYubiKey Serial: %1").arg(*regRes->yubikeySerial);
                }
                QMessageBox::information(this, "Key Registered", successMsg);
                accept();
                return;
            }

            // Bootstrap Admin
            QString sessionToken = finishDoc.object().value("session_token").toString();
            if (!sessionToken.isEmpty()) {
                std::string rawStr = sessionToken.toStdString();
                mitm::crypto::SecureString secureToken(rawStr.begin(), rawStr.end());
                mitm::config::ConfigManager::GetInstance().SetSessionToken(secureToken);
                mitm::auth::AuthManager::instance().performLogin(true);
            }
            QMessageBox::information(this, "Admin Registered", "Bootstrap Administrator successfully registered and activated!");
            accept();
        } else {
            if (registerOnly_) {
                QMessageBox::information(this, "Key Registered", "Security key registered successfully and linked to your account.");
                accept();
                return;
            }

            // Standard User: is_active = false
            QMessageBox::information(this, "Registration Pending Approval",
                "Your Passkey registration was received successfully!\n\n"
                "Your account is set to INACTIVE (is_active = false) pending administrator approval.\n"
                "You cannot sign in until an administrator approves your account.");
            tabWidget_->setCurrentIndex(0);
            loginIdInput_->setText(username);
            setStatus("Account pending administrator approval.", false);
        }
    } else {
        setStatus("Registration Verification Failed.", true);
    }
    finishReply->deleteLater();
}

void LoginDialog::onBrowserOAuthSignIn() {
    setStatus("Initiating Browser OAuth2 PKCE login loop...", false);
    // Best-effort session establishment with OS username fallback
    QString defaultUser = loginIdInput_->text().trimmed();
    if (mitm::auth::AuthManager::instance().performLogin(true)) {
        accept();
    } else {
        setStatus("Authentication rejected or cancelled.", true);
    }
}

} // namespace mitm::ui
