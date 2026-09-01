/**
 * SPDX-FileComment: TopicDependenciesWidget
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>

class TopicDependenciesWidget : public QWidget {
    Q_OBJECT
public:
    explicit TopicDependenciesWidget(QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onAdd();
    void onDelete();

private:
    QTableView* m_tableView;
    QStandardItemModel* m_model;
    QPushButton* m_refreshButton;
    QPushButton* m_addButton;
    QPushButton* m_deleteButton;
};
