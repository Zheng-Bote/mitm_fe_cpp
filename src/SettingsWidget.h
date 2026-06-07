/**
 * SPDX-FileComment: SettingsWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file SettingsWidget.h
 * @brief SettingsWidget
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

class SettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    ~SettingsWidget() override = default;

private slots:
    void onUnlockVault();

private:
    QLineEdit* m_masterKeyInput;
    QPushButton* m_unlockButton;
};
