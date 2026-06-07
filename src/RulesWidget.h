/**
 * SPDX-FileComment: RulesWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file RulesWidget.h
 * @brief RulesWidget
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class RulesWidget : public QWidget {
    Q_OBJECT

public:
    explicit RulesWidget(QWidget *parent = nullptr);
    ~RulesWidget() override = default;

private slots:
    void onAddRule();

private:
    QTableWidget* m_rulesTable;
    QPushButton* m_addButton;
    QPushButton* m_saveButton;
};
