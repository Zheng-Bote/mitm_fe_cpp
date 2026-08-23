#include "ExportReportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDate>

ExportReportDialog::ExportReportDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Export Report Configuration");
    resize(400, 200);

    auto mainLayout = new QVBoxLayout(this);
    auto formLayout = new QFormLayout();

    m_jobEdit = new QLineEdit("mitm_delivery", this);
    m_topicEdit = new QLineEdit("Employee", this);

    m_startEdit = new QDateTimeEdit(QDateTime(QDate::currentDate().addDays(-7), QTime(0, 0)), this);
    m_startEdit->setCalendarPopup(true);

    m_endEdit = new QDateTimeEdit(QDateTime(QDate::currentDate(), QTime(23, 59, 59)), this);
    m_endEdit->setCalendarPopup(true);

    formLayout->addRow("mitm-deliver Job:", m_jobEdit);
    formLayout->addRow("Topic:", m_topicEdit);
    formLayout->addRow("Start Date:", m_startEdit);
    formLayout->addRow("End Date:", m_endEdit);

    mainLayout->addLayout(formLayout);

    auto btnLayout = new QHBoxLayout();
    m_generateBtn = new QPushButton("Generate", this);
    m_cancelBtn = new QPushButton("Cancel", this);
    btnLayout->addStretch();
    btnLayout->addWidget(m_generateBtn);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_generateBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QString ExportReportDialog::getJobName() const {
    return m_jobEdit->text();
}

QString ExportReportDialog::getTopic() const {
    return m_topicEdit->text();
}

QDateTime ExportReportDialog::getStartDate() const {
    return m_startEdit->dateTime();
}

QDateTime ExportReportDialog::getEndDate() const {
    return m_endEdit->dateTime();
}
