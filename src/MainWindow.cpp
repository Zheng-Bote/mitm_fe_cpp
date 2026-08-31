#include <QPointer>
#include "ApiClient.h"
/**
 * SPDX-FileComment: MainWindow
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file MainWindow.cpp
 * @brief MainWindow
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "MainWindow.h"
#include "AuditLogsWidget.h"
#include "Config.h"
#include "DashboardWidget.h"
#include "DlqWidget.h"
#include "TransformationWidget.h"
#include "SchedulerWidget.h"
#include "SettingsWidget.h"
#include "SourcesWidget.h"
#include "TargetCredentialsWidget.h"
#include "SystemLogsWidget.h"
#include "AdminLogsWidget.h"
#include "RbacWidget.h"
#include "UploadWidget.h"
#include "TransformationErrorsWidget.h"
#include "BackupRestoreWidget.h"
#include "rz_config.hpp"
#include <QMenuBar>
#include <QProcessEnvironment>
#include <QStatusBar>
#include <QVBoxLayout>
#include <check_gh-update.hpp>
#include <spdlog/spdlog.h>
#include <QSysInfo>
#include <thread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <nlohmann/json.hpp>
#include <QLabel>
#include <QFileDialog>
#include <QSettings>
#include <QProcess>
#include <QCoreApplication>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  spdlog::info("Initializing MainWindow...");
  setupUi();
}

void MainWindow::setupUi() {
  this->setWindowTitle("MitM Data Aggregator - Admin Control Plane");
  this->resize(1280, 800);

  m_tabWidget = new QTabWidget(this);

  bool isAdmin = mitm::config::ConfigManager::GetInstance().HasRole("ADMIN");
  bool isViewer = mitm::config::ConfigManager::GetInstance().HasRole("VIEWER") || isAdmin;
  bool isUploader = mitm::config::ConfigManager::GetInstance().HasRole("UPLOADER") || isAdmin;
  bool isBackupRestore = mitm::config::ConfigManager::GetInstance().HasRole("BACKUP-RESTORE") || isAdmin;

  m_dashboardWidget = new DashboardWidget(this);
  m_tabWidget->addTab(m_dashboardWidget, "📊 Dashboard");
  m_tabWidget->addTab(new SchedulerWidget(this), "⏱️ Scheduler");
  auto logsTabWidget = new QTabWidget(this);
  logsTabWidget->addTab(new AdminLogsWidget(this), "Admin-Logs");
  logsTabWidget->addTab(new AuditLogsWidget(this), "Jobs-Logs");
  logsTabWidget->addTab(new SystemLogsWidget(this), "System-Logs");
  logsTabWidget->addTab(new TransformationErrorsWidget(this), "Transformation Errors");
  m_tabWidget->addTab(logsTabWidget, "📜 Logs");

  if (isViewer) {
      m_tabWidget->addTab(new TransformationWidget(this), "🧩 Transformation Layer");
      m_tabWidget->addTab(new DlqWidget(this), "🚑 DLQ & Cursors");
  }
  if (isAdmin) {
      m_tabWidget->addTab(new SourcesWidget(this), "🔌 Source Credentials");
      m_tabWidget->addTab(new TargetCredentialsWidget(this), "🎯 Target Credentials");
      m_tabWidget->addTab(new RbacWidget(this), "👥 RBAC");
      m_tabWidget->addTab(new SettingsWidget(this), "⚙️ Settings & Key Vault");
  }
  if (isUploader) {
      m_tabWidget->addTab(new UploadWidget(this), "📤 Manual Upload");
  }
  if (isBackupRestore) {
      m_tabWidget->addTab(new BackupRestoreWidget(this), "💾 Backup/Restore");
  }

  this->setCentralWidget(m_tabWidget);

  if (!isAdmin) {
      // Viewer or Unknown user -> Disable all buttons except "Refresh", "Upload", and "Browse"
      QList<QPushButton*> buttons = m_tabWidget->findChildren<QPushButton*>();
      for (auto* btn : buttons) {
          if (!btn->text().contains("Refresh", Qt::CaseInsensitive) &&
              !btn->text().contains("Upload", Qt::CaseInsensitive) &&
              !btn->text().contains("Browse", Qt::CaseInsensitive)) {
              btn->setEnabled(false);
          }
      }
  }

  // Setup basic menu
  menuBar()->setNativeMenuBar(false);
  auto fileMenu = menuBar()->addMenu("&File");
  auto exitAction = fileMenu->addAction("E&xit");
  connect(exitAction, &QAction::triggered, this, &QWidget::close);

  auto settingsMenu = menuBar()->addMenu("&Settings");
  auto proxyAction = settingsMenu->addAction("Network-Proxy");
  connect(proxyAction, &QAction::triggered, this, &MainWindow::showProxyDialog);
  
  auto configAction = settingsMenu->addAction("Select Configuration...");
  connect(configAction, &QAction::triggered, this, &MainWindow::showConfigDialog);

  auto infoMenu = menuBar()->addMenu("&Info");
  auto userGuideAction = infoMenu->addAction("&User Guide");
  connect(userGuideAction, &QAction::triggered, this, &MainWindow::showUserGuideDialog);
  auto aboutAction = infoMenu->addAction("&About");
  connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

  QString versionStr = QString::fromStdString(std::string(rz::config::VERSION));
  auto env = QProcessEnvironment::systemEnvironment();
  QString osUser = env.value("USER", env.value("USERNAME", "unknown"));
  QString compName = QSysInfo::machineHostName();
  QString configName = QString::fromStdString(mitm::config::ConfigManager::GetInstance().GetConfig().name);

  QString statusText = QString("Version: %1 | User: %2 | Computer: %3 | Config: %4").arg(versionStr, osUser, compName, configName);
  QLabel *statusLabel = new QLabel(statusText, this);
  statusBar()->addWidget(statusLabel);
  spdlog::info("Application started. {}", statusText.toStdString());

  // Log frontend startup to backend
  {
      nlohmann::json j;
      j["action"] = "frontend_startup";
      j["details"] = {
          {"version", versionStr.toStdString()},
          {"user", osUser.toStdString()},
          {"computer", compName.toStdString()}
      };
      
      mitm::api::ApiClient::instance().post("/admin/action", QString::fromStdString(j.dump()).toUtf8(),
          [](const QByteArray& data, QNetworkReply* reply) {
              // Success
          },
          [](int statusCode, const QString& errorString) {
              spdlog::warn("Failed to log frontend startup to backend: {}", errorString.toStdString());
          }
      );
  }

  // Check for updates asynchronously
  auto proxy = mitm::config::ConfigManager::GetInstance().GetProxyString();
  std::thread([proxy]() {
    try {
      auto future = ghupdate::check_github_update_async(
          std::string(rz::config::PROJECT_HOMEPAGE_URL),
          std::string(rz::config::VERSION), proxy);
      auto result = future.get();

      if (result.hasUpdate) {
        spdlog::info("🚀 Update available: {}", result.latestVersion);
      } else {
        spdlog::info("Frontend is up-to-date.");
      }
    } catch (const std::exception &e) {
      spdlog::error("Update check failed: {}", e.what());
    }
  }).detach();
}

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QNetworkProxy>

void MainWindow::showAboutDialog() {
  QDialog dialog(this);
  dialog.setWindowTitle(
      "About " + QString::fromStdString(std::string(rz::config::PROJECT_NAME)));
  dialog.resize(400, 250);

  auto *layout = new QVBoxLayout(&dialog);

  QString infoText =
      QString("<b>%1</b><br/>"
              "%2<br/><br/>"
              "<b>Version:</b> %3<br/>"
              "<b>License:</b> %4<br/>"
              "<b>Copyright:</b> %5<br/><br/>")
          .arg(QString::fromStdString(std::string(rz::config::PROJECT_NAME)),
               QString::fromStdString(
                   std::string(rz::config::PROJECT_DESCRIPTION)),
               QString::fromStdString(std::string(rz::config::VERSION)),
               QString::fromStdString(std::string(rz::config::LICENSE)),
               QString::fromStdString(std::string(rz::config::COPYRIGHT)));

  auto *hLayout = new QHBoxLayout();
  layout->addLayout(hLayout);

  auto *logoLabel = new QLabel(&dialog);
  QPixmap logoPixmap("img/logo_256x256.png");
  if (!logoPixmap.isNull()) {
    // Scale it down a bit so it fits nicely
    logoLabel->setPixmap(logoPixmap.scaled(128, 128, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation));
  }
  hLayout->addWidget(logoLabel);

  auto *infoLabel = new QLabel(infoText, &dialog);
  infoLabel->setWordWrap(true);
  hLayout->addWidget(infoLabel);

  auto *updateLabel = new QLabel("<i>Checking for updates...</i>", &dialog);
  updateLabel->setWordWrap(true);
  layout->addWidget(updateLabel);

  auto *okButton = new QPushButton("OK", &dialog);
  connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
  layout->addStretch();
  layout->addWidget(okButton);

  auto proxy = mitm::config::ConfigManager::GetInstance().GetProxyString();
  QPointer<QLabel> safeLabel(updateLabel);
  std::thread([safeLabel, proxy]() {
    try {
      auto future = ghupdate::check_github_update_async(
          std::string(rz::config::PROJECT_HOMEPAGE_URL),
          std::string(rz::config::VERSION), proxy);
      auto result = future.get();

      QMetaObject::invokeMethod(
          safeLabel,
          [safeLabel, result]() {
            if (!safeLabel) return;
            if (result.hasUpdate) {
              safeLabel->setText(
                  QString("<font color='green'><b>🚀 Update available: "
                          "%1</b></font>")
                      .arg(QString::fromStdString(result.latestVersion)));
            } else {
              safeLabel->setText("You are using the latest version.");
            }
          },
          Qt::QueuedConnection);
    } catch (const std::exception &) {
      QMetaObject::invokeMethod(
          safeLabel,
          [safeLabel]() {
            if (!safeLabel) return;
            safeLabel->setText(
                "<font color='red'>Update check failed.</font><br/><i>Note: "
                "You may need to configure a proxy in the Settings.</i>");
          },
          Qt::QueuedConnection);
    }
  }).detach();

  dialog.exec();
}

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>

void MainWindow::showProxyDialog() {
  QDialog dialog(this);
  dialog.setWindowTitle("Network Proxy Settings");
  dialog.resize(350, 200);

  auto *layout = new QFormLayout(&dialog);

  auto proxyConfig =
      mitm::config::ConfigManager::GetInstance().GetConfig().proxy;

  auto *hostEdit =
      new QLineEdit(QString::fromStdString(proxyConfig.proxy_host), &dialog);
  auto *portEdit =
      new QLineEdit(QString::number(proxyConfig.proxy_port), &dialog);
  auto *userEdit = new QLineEdit(
      QString::fromStdString(proxyConfig.proxy_username), &dialog);
  auto *passEdit = new QLineEdit(
      QString::fromStdString(std::string(proxyConfig.proxy_password.c_str())), &dialog);
  passEdit->setEchoMode(QLineEdit::Password);
  auto *activeCheck = new QCheckBox("Enable Proxy", &dialog);
  activeCheck->setChecked(proxyConfig.proxy_active);

  layout->addRow("Proxy Host:", hostEdit);
  layout->addRow("Proxy Port:", portEdit);
  layout->addRow("Username:", userEdit);
  layout->addRow("Password:", passEdit);
  layout->addRow("", activeCheck);

  auto *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  layout->addRow(buttonBox);

  if (dialog.exec() == QDialog::Accepted) {
    mitm::config::ProxyConfig newConfig;
    newConfig.proxy_host = hostEdit->text().toStdString();
    bool ok;
    int port = portEdit->text().toInt(&ok);
    newConfig.proxy_port = ok ? port : 8080;
    newConfig.proxy_username = userEdit->text().toStdString();
    newConfig.proxy_password = passEdit->text().toStdString();
    newConfig.proxy_active = activeCheck->isChecked();

    mitm::config::ConfigManager::GetInstance().SaveUserConfig(newConfig);
  }
}

void MainWindow::showConfigDialog() {
    QString dataDir = QCoreApplication::applicationDirPath() + "/data";
    QString fileName = QFileDialog::getOpenFileName(this, "Select Configuration", dataDir, "Encrypted Configs (*.enc)");
    if (!fileName.isEmpty()) {
        QSettings settings;
        settings.setValue("LastConfigPath", fileName);
        
        QMessageBox::information(this, "Restart Required", "The application will now restart to apply the new configuration.");
        
        qApp->quit();
        QProcess::startDetached(qApp->arguments()[0], QStringList());
    }
}

void MainWindow::showUserGuideDialog() {
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("User Guide");
    dialog->resize(900, 700);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QTextBrowser *textBrowser = new QTextBrowser(dialog);
    textBrowser->setOpenExternalLinks(true);
    layout->addWidget(textBrowser);

    textBrowser->setText("Loading User Guide...");

    QNetworkAccessManager *manager = new QNetworkAccessManager(dialog);
    
    auto proxyCfg = mitm::config::ConfigManager::GetInstance().GetConfig().proxy;
    if (proxyCfg.proxy_active && !proxyCfg.proxy_host.empty()) {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        
        QString proxyHost = QString::fromStdString(proxyCfg.proxy_host);
        if (proxyHost.startsWith("https://", Qt::CaseInsensitive)) proxyHost = proxyHost.mid(8);
        else if (proxyHost.startsWith("http://", Qt::CaseInsensitive)) proxyHost = proxyHost.mid(7);
        
        proxy.setHostName(proxyHost);
        proxy.setPort(proxyCfg.proxy_port);
        if (!proxyCfg.proxy_username.empty()) {
            proxy.setUser(QString::fromStdString(proxyCfg.proxy_username));
            proxy.setPassword(QString::fromStdString(std::string(proxyCfg.proxy_password.c_str())));
        }
        manager->setProxy(proxy);
    }

    QString baseUrl = QString::fromStdString(std::string(rz::config::PROJECT_HOMEPAGE_URL));
    QString fetchUrl = baseUrl;
    if (fetchUrl.contains("github.com", Qt::CaseInsensitive)) {
        fetchUrl.replace("github.com", "raw.githubusercontent.com", Qt::CaseInsensitive);
        fetchUrl += "/main/docs/UserGuide.md";
    } else {
        fetchUrl += "/docs/UserGuide.md";
    }
    QUrl url(fetchUrl);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, dialog, [reply, textBrowser]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString content = QString::fromUtf8(reply->readAll());
            textBrowser->setMarkdown(content);
        } else {
            textBrowser->setHtml(QString("<b>Error loading User Guide:</b><br>%1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });

    dialog->show();
}
