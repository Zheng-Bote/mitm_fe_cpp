/**
 * SPDX-FileComment: MainWindow
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file MainWindow.h
 * @brief MainWindow
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#pragma once

#include <QMainWindow>
#include <QTabWidget>

class DashboardWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void showAboutDialog();
    void showProxyDialog();
    void showConfigDialog();

private:
    void setupUi();

    QTabWidget* m_tabWidget;
    DashboardWidget* m_dashboardWidget;
};
