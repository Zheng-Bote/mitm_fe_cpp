#pragma once

#include <QMainWindow>
#include <QTabWidget>

class DashboardWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupUi();

    QTabWidget* m_tabWidget;
    DashboardWidget* m_dashboardWidget;
};
