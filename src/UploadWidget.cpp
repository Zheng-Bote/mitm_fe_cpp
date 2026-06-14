#include "UploadWidget.h"
#include "Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>

UploadWidget::UploadWidget(QWidget *parent) : QWidget(parent), m_networkManager(new QNetworkAccessManager(this)) {
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel("<h2>📂 Manual File Upload</h2>", this);
    layout->addWidget(title);

    auto* info = new QLabel("Upload CSV/XLSX files directly to the MitM Scheduler for immediate ingestion.\n"
                            "You must have the 'UPLOADER' role.", this);
    layout->addWidget(info);

    // Topic row
    auto* topicLayout = new QHBoxLayout();
    topicLayout->addWidget(new QLabel("Topic (Source Name):"));
    m_topicEdit = new QLineEdit(this);
    m_topicEdit->setPlaceholderText("e.g. PG_EMPLOYEE");
    topicLayout->addWidget(m_topicEdit);
    layout->addLayout(topicLayout);

    // File row
    auto* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(new QLabel("File:"));
    m_fileEdit = new QLineEdit(this);
    m_fileEdit->setReadOnly(true);
    fileLayout->addWidget(m_fileEdit);
    m_browseBtn = new QPushButton("Browse...", this);
    fileLayout->addWidget(m_browseBtn);
    layout->addLayout(fileLayout);

    m_uploadBtn = new QPushButton("🚀 Upload & Trigger Collector", this);
    layout->addWidget(m_uploadBtn);

    m_statusLabel = new QLabel("", this);
    layout->addWidget(m_statusLabel);

    layout->addStretch();

    connect(m_browseBtn, &QPushButton::clicked, this, &UploadWidget::browseFile);
    connect(m_uploadBtn, &QPushButton::clicked, this, &UploadWidget::uploadFile);
}

void UploadWidget::browseFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select File to Upload", "", "Data Files (*.csv *.xlsx)");
    if (!fileName.isEmpty()) {
        m_fileEdit->setText(fileName);
    }
}

void UploadWidget::uploadFile() {
    QString topic = m_topicEdit->text().trimmed();
    QString filePath = m_fileEdit->text().trimmed();

    if (topic.isEmpty() || filePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please provide a topic and select a file.");
        return;
    }

    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Could not open the selected file.");
        delete file;
        return;
    }

    m_uploadBtn->setEnabled(false);
    m_statusLabel->setText("Uploading...");

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart topicPart;
    topicPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"topic\""));
    topicPart.setBody(topic.toUtf8());
    multiPart->append(topicPart);

    QHttpPart filePart;
    QFileInfo fileInfo(filePath);
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(fileInfo.fileName())));
    filePart.setBodyDevice(file);
    file->setParent(multiPart); // File deleted when multiPart is deleted
    multiPart->append(filePart);

    QString url = mitm::config::ConfigManager::GetInstance().GetHostUrl() + "/admin/upload/source_file";
    QNetworkRequest request((QUrl(url)));
    QString authHeader = mitm::config::ConfigManager::GetInstance().GetAuthHeader();
    request.setRawHeader("Authorization", authHeader.toUtf8());

    QNetworkAccessManager* networkManager = new QNetworkAccessManager(this);
    QNetworkReply* reply = networkManager->post(request, multiPart);
    multiPart->setParent(reply); // Delete multipart with the reply

    connect(reply, &QNetworkReply::finished, this, [this, reply, networkManager]() {
        onUploadFinished(reply);
        networkManager->deleteLater();
    });
}

void UploadWidget::onUploadFinished(QNetworkReply* reply) {
    reply->deleteLater();
    m_uploadBtn->setEnabled(true);

    if (reply->error() == QNetworkReply::NoError) {
        m_statusLabel->setText("<font color='green'>Upload successful! The collector has been triggered.</font>");
        m_fileEdit->clear();
    } else {
        QString errStr = reply->readAll();
        m_statusLabel->setText(QString("<font color='red'>Upload failed: %1 - %2</font>").arg(reply->errorString(), errStr));
    }
}
