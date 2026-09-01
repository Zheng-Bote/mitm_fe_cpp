#include "UploadWidget.h"
#include "ApiClient.h"
#include "Config.h"
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QMessageBox>
#include <QNetworkReply>
#include <QVBoxLayout>

UploadWidget::UploadWidget(QWidget *parent)
    : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);

  auto *title = new QLabel("<h2>📂 Manual File Upload</h2>", this);
  layout->addWidget(title);

  auto *info = new QLabel("Upload CSV/XLSX files directly to the MitM "
                          "Scheduler for immediate ingestion.\n"
                          "You must have the 'UPLOADER' role.",
                          this);
  layout->addWidget(info);

  // Topic row
  auto *topicLayout = new QHBoxLayout();
  topicLayout->addWidget(new QLabel("Topic (Source Name):"));
  m_topicEdit = new QLineEdit(this);
  m_topicEdit->setPlaceholderText("e.g. Employee");
  topicLayout->addWidget(m_topicEdit);
  layout->addLayout(topicLayout);

  // File row
  auto *fileLayout = new QHBoxLayout();
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
  m_statusLabel->setText("");

  QString fileName = QFileDialog::getOpenFileName(
      this, "Select File to Upload", "", "Data Files (*.csv *.xlsx)");
  if (!fileName.isEmpty()) {
    m_fileEdit->setText(fileName);
  }
}

void UploadWidget::uploadFile() {
  if (!mitm::config::ConfigManager::GetInstance().HasRole("UPLOADER") && !mitm::config::ConfigManager::GetInstance().HasRole("ADMIN")) {
      QMessageBox::warning(this, "Permission Denied", "Only users with the 'UPLOADER' or 'ADMIN' role can upload files.");
      return;
  }

  QString topic = m_topicEdit->text().trimmed();
  QString filePath = m_fileEdit->text().trimmed();

  if (topic.isEmpty() || filePath.isEmpty()) {
    QMessageBox::warning(this, "Error",
                         "Please provide a topic and select a file.");
    return;
  }

  QFile *file = new QFile(filePath);
  if (!file->open(QIODevice::ReadOnly)) {
    QMessageBox::critical(this, "Error", "Could not open the selected file.");
    delete file;
    return;
  }

  m_uploadBtn->setEnabled(false);
  m_statusLabel->setText("Uploading...");

  QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

  QHttpPart topicPart;
  topicPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                      QVariant("form-data; name=\"topic\""));
  topicPart.setBody(topic.toUtf8());
  multiPart->append(topicPart);

  QHttpPart filePart;
  QFileInfo fileInfo(filePath);
  filePart.setHeader(
      QNetworkRequest::ContentDispositionHeader,
      QVariant(QString("form-data; name=\"file\"; filename=\"%1\"")
                   .arg(fileInfo.fileName())));
  filePart.setBodyDevice(file);
  file->setParent(multiPart); // File deleted when multiPart is deleted
  multiPart->append(filePart);

  mitm::api::ApiClient::instance().post("/admin/upload/source_file", multiPart,
  [this](const QByteArray& data, QNetworkReply* reply) {
      m_uploadBtn->setEnabled(true);
      m_statusLabel->setText("<font color='green'>Upload successful! The collector has been triggered.</font>");
      m_fileEdit->clear();
  },
  [this](int statusCode, const QString& errorString) {
      m_uploadBtn->setEnabled(true);
      m_statusLabel->setText(QString("<font color='red'>Upload failed: %1 - %2</font>").arg(QString::number(statusCode), errorString));
  });
}
