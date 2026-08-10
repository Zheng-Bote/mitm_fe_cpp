/**
 * SPDX-FileComment: JobEditorDialog
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file JobEditorDialog.cpp
 * @brief JobEditorDialog
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "JobEditorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QStringList>

JobEditorDialog::JobEditorDialog(QWidget *parent) : QDialog(parent), m_jobId(0) {
    setupUi();
}

void JobEditorDialog::populateCronCombo(QComboBox* cb, int min, int max, const QStringList& extras) {
    cb->setEditable(true);
    cb->addItem("*");
    for (const auto& e : extras) cb->addItem(e);
    for (int i = min; i <= max; ++i) {
        cb->addItem(QString::number(i));
    }
}

void JobEditorDialog::setupUi() {
    this->setWindowTitle("Edit Job");
    this->resize(450, 400);

    auto mainLayout = new QVBoxLayout(this);
    auto formLayout = new QFormLayout();

    m_nameEdit = new QLineEdit(this);
    m_commandEdit = new QLineEdit(this);
    m_argsEdit = new QLineEdit("{}", this); // default empty JSON object
    m_enabledCheck = new QCheckBox("Enabled", this);
    m_restartCheck = new QCheckBox("Restart on Exit", this);

    formLayout->addRow("Name:", m_nameEdit);
    formLayout->addRow("Command:", m_commandEdit);
    formLayout->addRow("JSON Args:", m_argsEdit);
    formLayout->addRow("", m_enabledCheck);
    formLayout->addRow("", m_restartCheck);

    mainLayout->addLayout(formLayout);

    // Cron Group
    auto cronGroup = new QGroupBox("Cron Expression (Min / Hour / Day / Month / Weekday)", this);
    auto cronLayout = new QHBoxLayout(cronGroup);
    
    m_cronMin = new QComboBox(this);
    m_cronHour = new QComboBox(this);
    m_cronDom = new QComboBox(this);
    m_cronMonth = new QComboBox(this);
    m_cronDow = new QComboBox(this);

    populateCronCombo(m_cronMin, 0, 59, {"*/5", "*/15", "*/30"});
    populateCronCombo(m_cronHour, 0, 23, {"*/2", "*/4", "*/6", "*/8", "*/12"});
    populateCronCombo(m_cronDom, 1, 31);
    populateCronCombo(m_cronMonth, 1, 12);
    populateCronCombo(m_cronDow, 0, 6, {"1-5"}); // 0=Sun, 6=Sat

    cronLayout->addWidget(m_cronMin);
    cronLayout->addWidget(m_cronHour);
    cronLayout->addWidget(m_cronDom);
    cronLayout->addWidget(m_cronMonth);
    cronLayout->addWidget(m_cronDow);

    mainLayout->addWidget(cronGroup);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void JobEditorDialog::setJob(const nlohmann::json& job) {
    m_jobId = job.value("id", 0);
    m_nameEdit->setText(QString::fromStdString(job.value("name", "")));
    m_commandEdit->setText(QString::fromStdString(job.value("command", "")));
    
    if (job.contains("args") && !job["args"].is_null()) {
        m_argsEdit->setText(QString::fromStdString(job["args"].dump()));
    } else {
        m_argsEdit->setText("{}");
    }

    m_enabledCheck->setChecked(job.value("enabled", false));
    m_restartCheck->setChecked(job.value("restart_on_exit", false));

    QString cronExpr = QString::fromStdString(job.value("cron_expr", "* * * * *"));
    QStringList parts = cronExpr.split(" ", Qt::SkipEmptyParts);
    if (parts.size() == 5) {
        m_cronMin->setCurrentText(parts[0]);
        m_cronHour->setCurrentText(parts[1]);
        m_cronDom->setCurrentText(parts[2]);
        m_cronMonth->setCurrentText(parts[3]);
        m_cronDow->setCurrentText(parts[4]);
    }
}

nlohmann::json JobEditorDialog::getJob() const {
    nlohmann::json job;
    if (m_jobId > 0) job["id"] = m_jobId;
    job["name"] = m_nameEdit->text().toStdString();
    job["command"] = m_commandEdit->text().toStdString();
    
    try {
        job["args"] = nlohmann::json::parse(m_argsEdit->text().toStdString());
    } catch (...) {
        job["args"] = nlohmann::json::object(); // fallback
    }

    job["enabled"] = m_enabledCheck->isChecked();
    job["restart_on_exit"] = m_restartCheck->isChecked();
    
    QString cronExpr = QString("%1 %2 %3 %4 %5")
        .arg(m_cronMin->currentText())
        .arg(m_cronHour->currentText())
        .arg(m_cronDom->currentText())
        .arg(m_cronMonth->currentText())
        .arg(m_cronDow->currentText());
    
    job["cron_expr"] = cronExpr.toStdString();
    return job;
}
