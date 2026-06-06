#include "RulesWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <spdlog/spdlog.h>

RulesWidget::RulesWidget(QWidget *parent) : QWidget(parent) {
    auto mainLayout = new QVBoxLayout(this);

    auto headerLayout = new QHBoxLayout();
    headerLayout->addWidget(new QLabel("<b>Transformation Rules Mapping</b>", this));
    headerLayout->addStretch();
    m_addButton = new QPushButton("+ Add Rule", this);
    m_saveButton = new QPushButton("Save Rules", this);
    headerLayout->addWidget(m_addButton);
    headerLayout->addWidget(m_saveButton);
    mainLayout->addLayout(headerLayout);

    m_rulesTable = new QTableWidget(0, 4, this);
    m_rulesTable->setHorizontalHeaderLabels({"Source Column", "Target Column", "Function/Regex", "Status"});
    m_rulesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(m_rulesTable);

    // Add some mock data to show the concept
    m_rulesTable->insertRow(0);
    m_rulesTable->setItem(0, 0, new QTableWidgetItem("EMP_ID"));
    m_rulesTable->setItem(0, 1, new QTableWidgetItem("employeeNumber"));
    m_rulesTable->setItem(0, 2, new QTableWidgetItem("TO_STRING"));
    m_rulesTable->setItem(0, 3, new QTableWidgetItem("Active 🟢"));

    connect(m_addButton, &QPushButton::clicked, this, &RulesWidget::onAddRule);
}

void RulesWidget::onAddRule() {
    int row = m_rulesTable->rowCount();
    m_rulesTable->insertRow(row);
    m_rulesTable->setItem(row, 0, new QTableWidgetItem("new_source_field"));
    m_rulesTable->setItem(row, 1, new QTableWidgetItem("new_target_field"));
    m_rulesTable->setItem(row, 2, new QTableWidgetItem("TRIM"));
    m_rulesTable->setItem(row, 3, new QTableWidgetItem("Draft 🟡"));
    spdlog::info("Added new blank rule to mapping table.");
}
