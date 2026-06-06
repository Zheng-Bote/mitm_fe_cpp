#pragma once

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QNetworkAccessManager>

class AuditLogsWidget : public QWidget {
    Q_OBJECT

public:
    explicit AuditLogsWidget(QWidget *parent = nullptr);
    ~AuditLogsWidget() override = default;

private slots:
    void onRefresh();
    void onExportCsv();

private:
    QString getAuthHeader();

    QTableView* m_tableView;
    QStandardItemModel* m_model;
    QPushButton* m_refreshButton;
    QPushButton* m_exportButton;
    QNetworkAccessManager* m_networkManager;
};
