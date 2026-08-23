#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QPushButton>

class ExportReportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportReportDialog(QWidget *parent = nullptr);
    ~ExportReportDialog() override = default;

    QString getJobName() const;
    QString getTopic() const;
    QDateTime getStartDate() const;
    QDateTime getEndDate() const;

private:
    QLineEdit* m_jobEdit;
    QLineEdit* m_topicEdit;
    QDateTimeEdit* m_startEdit;
    QDateTimeEdit* m_endEdit;
    QPushButton* m_generateBtn;
    QPushButton* m_cancelBtn;
};
