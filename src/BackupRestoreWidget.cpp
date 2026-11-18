#include "BackupRestoreWidget.h"
#include "AppPaths.h"
#include "ApiClient.h"
#include "Config.h"
#include "BackupFile.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QProcessEnvironment>
#include <QMessageBox>
#include <QCoreApplication>

BackupRestoreWidget::BackupRestoreWidget(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

BackupRestoreWidget::~BackupRestoreWidget() {}

void BackupRestoreWidget::setupUi() {
    auto layout = new QVBoxLayout(this);
    
    m_btnBackup = new QPushButton("📥 Backup Configuration", this);
    m_btnRestore = new QPushButton("📤 Restore Configuration", this);
    m_logArea = new QTextEdit(this);
    m_logArea->setReadOnly(true);

    layout->addWidget(m_btnBackup);
    layout->addWidget(m_btnRestore);
    layout->addWidget(m_logArea);

    connect(m_btnBackup, &QPushButton::clicked, this, &BackupRestoreWidget::onBackupClicked);
    connect(m_btnRestore, &QPushButton::clicked, this, &BackupRestoreWidget::onRestoreClicked);
}

void BackupRestoreWidget::logMessage(const QString& msg) {
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_logArea->append(QString("[%1] %2").arg(timeStr, msg));
}

namespace {
/// Asks for the backup passphrase (masked); with confirmation for new backups. Returns an empty string on cancel.
QString askPassphrase(QWidget* parent, bool confirm) {
    QDialog dialog(parent);
    dialog.setWindowTitle(confirm ? "Backup Passphrase" : "Restore Passphrase");
    auto* layout = new QFormLayout(&dialog);
    auto* info = new QLabel(confirm
        ? QString("The backup is encrypted with this passphrase (AES-256-GCM, Argon2id).<br/>"
                  "<b>Without the passphrase the backup cannot be restored.</b> Minimum %1 characters.").arg(mitm::backup::kMinPassphraseChars)
        : QString("Enter the passphrase of the backup."), &dialog);
    info->setWordWrap(true);
    layout->addRow(info);
    auto* pass = new QLineEdit(&dialog);
    pass->setEchoMode(QLineEdit::Password);
    layout->addRow("Passphrase:", pass);
    QLineEdit* repeat = nullptr;
    if (confirm) {
        repeat = new QLineEdit(&dialog);
        repeat->setEchoMode(QLineEdit::Password);
        layout->addRow("Repeat:", repeat);
    }
    auto* error = new QLabel(&dialog);
    error->setStyleSheet("color: red;");
    layout->addRow(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        const QString err = mitm::backup::passphraseError(pass->text(), repeat ? repeat->text() : QString(), confirm);
        if (err.isEmpty()) dialog.accept(); else error->setText(err);
    });

    QString result;
    if (dialog.exec() == QDialog::Accepted) result = pass->text();
    pass->clear();
    if (repeat) repeat->clear();
    return result;
}

/// Overwrites a QString holding a secret (best effort; Qt may hold further copies).
void wipe(QString& secret) {
    secret.fill(QChar(0));
    secret.clear();
}
} // namespace

void BackupRestoreWidget::onBackupClicked() {
    QString passphrase = askPassphrase(this, true);
    if (passphrase.isEmpty()) return;

    logMessage("Requesting encrypted backup from server...");
    QByteArray body = mitm::backup::backupBody(passphrase);
    wipe(passphrase);

    mitm::api::ApiClient::instance().post("/api/v1/system/backup", body,
        [this](const QByteArray& data, QNetworkReply*) {
        if (mitm::backup::classify(data) != mitm::backup::Kind::Encrypted) {
            logMessage("Backup failed: the server did not return an encrypted backup (mitm_http-server >= 3.6.0 required).");
            QMessageBox::warning(this, "Backup Failed", "The server did not return an encrypted backup.\nmitm_http-server 3.6.0 or newer is required.");
            return;
        }
        QString backupDir = mitm::paths::baseDir() + "/data/backup";
        QDir dir(backupDir);
        if (!dir.exists()) dir.mkpath(".");

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
        QString configName = QString::fromStdString(mitm::config::ConfigManager::GetInstance().GetConfig().name);
        QString osUser = QProcessEnvironment::systemEnvironment().value("USER", QProcessEnvironment::systemEnvironment().value("USERNAME", "unknown"));
        QString filename = mitm::backup::fileName(backupDir, timestamp, configName, osUser);

        QFile file(filename);
        if (file.open(QIODevice::WriteOnly)) {
            file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            file.write(data);
            file.close();
            logMessage("Encrypted backup saved to: " + filename);
            QMessageBox::information(this, "Backup Successful",
                "Encrypted configuration backup saved to:\n" + filename + "\n\nKeep the passphrase safe – it is required for a restore.");
        } else {
            logMessage("Failed to save backup file: " + file.errorString());
        }
    },
    [this](int, const QString& errorString) {
        logMessage("Backup failed: " + errorString);
        QMessageBox::warning(this, "Backup Failed", errorString);
    });
    body.fill('\0');
}

void BackupRestoreWidget::onRestoreClicked() {
    QString backupDir = mitm::paths::baseDir() + "/data/backup";
    QString filename = QFileDialog::getOpenFileName(this, "Select Backup File", backupDir,
        QString("MitM Backup (*.%1);;All Files (*)").arg(mitm::backup::kFileExtension));
    if (filename.isEmpty()) return;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        logMessage("Failed to open file: " + filename);
        return;
    }
    QByteArray document = file.readAll();
    file.close();

    switch (mitm::backup::classify(document)) {
    case mitm::backup::Kind::Encrypted:
        break;
    case mitm::backup::Kind::LegacyUnencrypted:
        logMessage("Restore aborted: unencrypted backup of an older version: " + filename);
        QMessageBox::warning(this, "Unsupported Backup",
            "This is an unencrypted backup of an older version (before mitm_http-server 3.6.0).\n"
            "It cannot be restored anymore. Please create a new, encrypted backup.");
        return;
    case mitm::backup::Kind::Invalid:
        logMessage("Restore aborted: not a MitM backup file: " + filename);
        QMessageBox::warning(this, "Invalid Backup", "The selected file is not a MitM configuration backup.");
        return;
    }

    auto replyBox = QMessageBox::question(this, "Confirm Restore",
        "Are you sure you want to restore the configuration?\n\n"
        "This overwrites credentials, delivery targets, mappings, users (incl. roles and security keys), "
        "RBAC rules and the scheduler configuration.\n"
        "Jobs that are not contained in the backup are deactivated (not deleted); the job history is kept.",
        QMessageBox::Yes | QMessageBox::No);
    if (replyBox != QMessageBox::Yes) return;

    QString passphrase = askPassphrase(this, false);
    if (passphrase.isEmpty()) return;

    logMessage("Sending restore request to server...");
    QByteArray body = mitm::backup::restoreBody(document, passphrase);
    wipe(passphrase);

    mitm::api::ApiClient::instance().post("/api/v1/system/restore", body,
        [this](const QByteArray& data, QNetworkReply*) {
        const QString summary = mitm::backup::restoreSummary(data);
        logMessage("Restore completed successfully. " + QString(summary).replace('\n', ' '));
        QMessageBox::information(this, "Restore Successful", "Configuration restored successfully.\n\n" + summary);
    },
    [this](int, const QString& errorString) {
        logMessage("Restore failed: " + errorString);
        QMessageBox::warning(this, "Restore Failed", errorString);
    });
    body.fill('\0');
}
