#pragma once

#include <QWidget>
#include <QNetworkReply>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTextEdit>

class BackupRestoreWidget : public QWidget {
    Q_OBJECT

public:
    explicit BackupRestoreWidget(QWidget *parent = nullptr);
    ~BackupRestoreWidget();

private slots:
    void onBackupClicked();
    void onRestoreClicked();

private:
    void setupUi();
    void logMessage(const QString& msg);

    QPushButton *m_btnBackup;
    QPushButton *m_btnRestore;
    QTextEdit *m_logArea;
};
