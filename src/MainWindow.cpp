#include "MainWindow.h"
#include "DashboardWidget.h"
#include "SchedulerWidget.h"
#include "RulesWidget.h"
#include "DlqWidget.h"
#include "SettingsWidget.h"
#include "SystemLogsWidget.h"
#include "AuditLogsWidget.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <spdlog/spdlog.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    spdlog::info("Initializing MainWindow...");
    setupUi();
}

void MainWindow::setupUi() {
    this->setWindowTitle("MitM Data Aggregator - Admin Control Plane");
    this->resize(1280, 800);

    m_tabWidget = new QTabWidget(this);

    m_dashboardWidget = new DashboardWidget(this);
    m_tabWidget->addTab(m_dashboardWidget, "📊 Dashboard");
    m_tabWidget->addTab(new SchedulerWidget(this), "⏱️ Scheduler");
    m_tabWidget->addTab(new SystemLogsWidget(this), "📜 System Logs");
    m_tabWidget->addTab(new AuditLogsWidget(this), "🕵️ Audit Logs");
    m_tabWidget->addTab(new RulesWidget(this), "🧩 Rules & Mapping");
    m_tabWidget->addTab(new DlqWidget(this), "🚑 DLQ & Cursors");
    m_tabWidget->addTab(new SettingsWidget(this), "⚙️ Settings & Key Vault");

    this->setCentralWidget(m_tabWidget);

    // Setup basic menu
    auto fileMenu = menuBar()->addMenu("&File");
    auto exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    statusBar()->showMessage("Ready. Disconnected from API.", 5000);
}
