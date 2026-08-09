#include "BackupRestoreWidget.h"
#include "Config.h"
#include <QFileDialog>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QProcessEnvironment>
#include <QMessageBox>
#include <QCoreApplication>

BackupRestoreWidget::BackupRestoreWidget(QWidget *parent)
    : QWidget(parent), m_networkManager(new QNetworkAccessManager(this)) {
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

void BackupRestoreWidget::onBackupClicked() {
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/backup");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", mitm::config::ConfigManager::GetInstance().GetAuthHeader().toLocal8Bit());
    
    logMessage("Requesting backup from server...");
    
    auto reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            logMessage("Backup failed: " + reply->errorString());
            return;
        }
        
        QByteArray data = reply->readAll();
        
        QString binFolder = QCoreApplication::applicationDirPath();
        QString backupDir = binFolder + "/data/backup";
        QDir dir(backupDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
        QString configName = QString::fromStdString(mitm::config::ConfigManager::GetInstance().GetConfig().name);
        QString osUser = QProcessEnvironment::systemEnvironment().value("USER", QProcessEnvironment::systemEnvironment().value("USERNAME", "unknown"));
        
        QString filename = QString("%1/%2_backup-%3_%4.json").arg(backupDir, timestamp, configName, osUser);
        
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();
            logMessage("Backup saved successfully to: " + filename);
            QMessageBox::information(this, "Backup Successful", "Configuration backup saved to:\n" + filename);
        } else {
            logMessage("Failed to save backup file: " + file.errorString());
        }
    });
}

void BackupRestoreWidget::onRestoreClicked() {
    QString binFolder = QCoreApplication::applicationDirPath();
    QString backupDir = binFolder + "/data/backup";
    
    QString filename = QFileDialog::getOpenFileName(this, "Select Backup File", backupDir, "JSON Files (*.json)");
    if (filename.isEmpty()) return;
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        logMessage("Failed to open file: " + filename);
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    auto replyBox = QMessageBox::question(this, "Confirm Restore", 
        "Are you sure you want to restore the configuration? This will overwrite the current configuration.",
        QMessageBox::Yes | QMessageBox::No);
        
    if (replyBox != QMessageBox::Yes) return;
    
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QUrl url(host + "/admin/restore");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", mitm::config::ConfigManager::GetInstance().GetAuthHeader().toLocal8Bit());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    logMessage("Sending restore request to server...");
    
    auto reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            logMessage("Restore failed: " + reply->errorString());
            QByteArray errBody = reply->readAll();
            if (!errBody.isEmpty()) logMessage("Server message: " + QString(errBody));
            return;
        }
        
        logMessage("Restore completed successfully.");
        QMessageBox::information(this, "Restore Successful", "Configuration restored successfully. Scheduler has been reloaded.");
    });
}
