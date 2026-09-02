#include "AuditLogsWidget.h"
#include "ApiClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QDateTime>
#include <QTimeZone>
#include <QProcessEnvironment>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <spdlog/spdlog.h>
#include "Config.h"
#include "schematas/job_audit_logs_generated.h"
#include "ExportReportDialog.h"
#include <QRegularExpression>
#include <QDir>
#include <QSettings>
#include <QFileInfo>
#include "xlsxdocument.h"
#include "xlsxchart.h"

AuditLogsWidget::AuditLogsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);

    auto headerLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton("Refresh Job Audits", this);
    m_exportButton = new QPushButton("Export CSV", this);
    m_exportReportButton = new QPushButton("Export Report", this);
    
    m_useDateRangeCheckbox = new QCheckBox("Filter by Date", this);
    m_startDateEdit = new QDateEdit(QDate::currentDate().addDays(-7), this);
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setEnabled(false);
    
    m_endDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setEnabled(false);
    
    connect(m_useDateRangeCheckbox, &QCheckBox::toggled, m_startDateEdit, &QWidget::setEnabled);
    connect(m_useDateRangeCheckbox, &QCheckBox::toggled, m_endDateEdit, &QWidget::setEnabled);

    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_useDateRangeCheckbox);
    headerLayout->addWidget(m_startDateEdit);
    headerLayout->addWidget(m_endDateEdit);
    headerLayout->addWidget(m_exportButton);
    headerLayout->addWidget(m_exportReportButton);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    m_tableView = new QTableView(this);
    m_model = new QStandardItemModel(0, 5, this);
    QTimeZone tz = QTimeZone::systemTimeZone();
    QString tsHeader = QString("Timestamp (%1)").arg(QString(tz.id()));
    m_model->setHorizontalHeaderLabels({"ID", tsHeader, "Run ID", "Component", "Message"});
    
    m_tableView->setModel(m_model);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSortingEnabled(true);
    
    mainLayout->addWidget(m_tableView);

    connect(m_refreshButton, &QPushButton::clicked, this, &AuditLogsWidget::onRefresh);
    connect(m_exportButton, &QPushButton::clicked, this, &AuditLogsWidget::onExportCsv);
    connect(m_exportReportButton, &QPushButton::clicked, this, &AuditLogsWidget::onExportReport);
}

void AuditLogsWidget::onRefresh() {
    m_refreshButton->setEnabled(false);
    
    QString url = "/admin/logs/job-audit_bin";
    if (m_useDateRangeCheckbox->isChecked()) {
        QString from = m_startDateEdit->date().toString("yyyy-MM-dd");
        QString to = m_endDateEdit->date().toString("yyyy-MM-dd");
        url += QString("?from=%1&to=%2").arg(from, to);
    }
    
    mitm::api::ApiClient::instance().get(url,
        [this](const QByteArray& data, QNetworkReply* reply) {

        m_refreshButton->setEnabled(true);

        
            m_model->setRowCount(0);
            try {
                
                flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
                if (!schematas::VerifyJobAuditLogListBuffer(verifier)) {
                    spdlog::error("Invalid JobAuditLogs FlatBuffer data received");
                } else {
                    auto list = schematas::GetJobAuditLogList(data.constData());
                    if (list && list->logs()) {
                        auto arr = list->logs();
                        for (int i = 0; i < arr->size(); ++i) {
                            auto log = arr->Get(i);
                            if (!log) continue;
                            QList<QStandardItem*> rowItems;
                            rowItems << new QStandardItem(QString::number(log->id()));
                            
                            QString rawTs = log->ts() ? QString::fromUtf8(log->ts()->c_str()) : "";
                            QDateTime dt = QDateTime::fromString(rawTs, Qt::ISODate);
                            QString tsStr = dt.isValid() ? dt.toLocalTime().toString("yyyy-MM-dd HH:mm:ss") : rawTs;
                            rowItems << new QStandardItem(tsStr);
                            
                            rowItems << new QStandardItem(QString::number(log->run_id()));
                            
                            QString component = log->component() ? QString::fromUtf8(log->component()->c_str()) : "";
                            QString message = log->message() ? QString::fromUtf8(log->message()->c_str()) : "";

                            rowItems << new QStandardItem(component);
                            rowItems << new QStandardItem(message);
                            m_model->appendRow(rowItems);
                        }
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("FlatBuffers parsing error in AuditLogs: {}", e.what());
            }
        
        m_tableView->resizeColumnsToContents();
        
            },
        [this](int statusCode, const QString& errorString) {

        m_refreshButton->setEnabled(true);

        
            spdlog::error("AuditLogs API failed: {}", errorString.toStdString());
            m_model->setRowCount(0);
            m_model->appendRow({new QStandardItem("Error"), new QStandardItem(errorString)});
        
        m_tableView->resizeColumnsToContents();
        
            }
    );
}

void AuditLogsWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Audit Logs", "audit_logs.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot open file for writing.");
        return;
    }

    QTextStream out(&file);
    // Write headers
    for (int col = 0; col < m_model->columnCount(); ++col) {
        out << "\"" << m_model->horizontalHeaderItem(col)->text() << "\"";
        if (col < m_model->columnCount() - 1) out << ",";
    }
    out << "\n";

    // Write data
    for (int row = 0; row < m_model->rowCount(); ++row) {
        for (int col = 0; col < m_model->columnCount(); ++col) {
            auto item = m_model->item(row, col);
            QString text = item ? item->text() : "";
            text.replace("\"", "\"\""); // Escape quotes
            out << "\"" << text << "\"";
            if (col < m_model->columnCount() - 1) out << ",";
        }
        out << "\n";
    }

    file.close();
    QMessageBox::information(this, "Export Successful", "Logs exported successfully to\n" + fileName);
}

void AuditLogsWidget::onExportReport() {
    ExportReportDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString jobName = dialog.getJobName();
    QString topic = dialog.getTopic();
    QDateTime startDate = dialog.getStartDate();
    QDateTime endDate = dialog.getEndDate();

    QSettings settings;
    QString lastReportDir = settings.value("export/lastReportDir", QDir::homePath()).toString();
    /* QString defaultFileName = QDir(lastReportDir).filePath(QString("%1__%2-%3_%4_report.xlsx")
        .arg(QDate::currentDate().toString("yyyy-MM-dd"))
        .arg(startDate.toString("yyyy-MM-dd_HHmm"))
        .arg(endDate.toString("yyyy-MM-dd_HHmm"))
        .arg(topic));
    */
     QString defaultFileName = QDir(lastReportDir).filePath(QString("%1__%4_report__%2-%3.xlsx")
        .arg(QDate::currentDate().toString("yyyy-MM-dd"))
        .arg(startDate.toString("yyyy-MM-dd_HHmm"))
        .arg(endDate.toString("yyyy-MM-dd_HHmm"))
        .arg(topic));
    QString fileName = QFileDialog::getSaveFileName(this, "Save Report", defaultFileName, "Excel Files (*.xlsx)");
    if (fileName.isEmpty()) return;
    
    settings.setValue("export/lastReportDir", QFileInfo(fileName).absolutePath());

    QXlsx::Document xlsx;
    if (!xlsx.sheetNames().isEmpty()) {
        xlsx.renameSheet(xlsx.sheetNames().first(), "Batch-Uploads");
    } else {
        xlsx.addSheet("Batch-Uploads");
    }

    QXlsx::Format titleFmt;
    titleFmt.setFontBold(true);
    titleFmt.setFillPattern(QXlsx::Format::PatternSolid);
    titleFmt.setPatternBackgroundColor(QColor(220, 230, 241));
    titleFmt.setPatternForegroundColor(QColor(220, 230, 241));

    QXlsx::Format subTitleFmt;
    subTitleFmt.setFillPattern(QXlsx::Format::PatternSolid);
    subTitleFmt.setPatternBackgroundColor(QColor(220, 230, 241));
    subTitleFmt.setPatternForegroundColor(QColor(220, 230, 241));

    QXlsx::Format headerFmt;
    headerFmt.setFontBold(true);
    headerFmt.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFmt.setFillPattern(QXlsx::Format::PatternSolid);
    headerFmt.setPatternBackgroundColor(QColor(220, 230, 241));
    headerFmt.setPatternForegroundColor(QColor(220, 230, 241));

    QString subTitleStr = QString("%1 - %2 %3").arg(
        startDate.toString("dd.MM.yyyy"),
        endDate.toString("dd.MM.yyyy"),
        topic.isEmpty() ? "" : topic
    );

    xlsx.selectSheet("Batch-Uploads");
    xlsx.write("A1", "Batch-Uploads", titleFmt);
    xlsx.mergeCells("A1:B1", titleFmt);
    xlsx.write("A2", subTitleStr, subTitleFmt);
    xlsx.mergeCells("A2:B2", subTitleFmt);
    
    xlsx.write("A3", "Timestamp", headerFmt);
    xlsx.write("B3", "Message", headerFmt);
    
    xlsx.addSheet("Upload-Report");
    xlsx.selectSheet("Upload-Report");
    
    xlsx.write("A1", "Upload-Report", titleFmt);
    xlsx.mergeCells("A1:G1", titleFmt);
    xlsx.write("A2", subTitleStr, subTitleFmt);
    xlsx.mergeCells("A2:G2", subTitleFmt);

    xlsx.write("A3", "Timestamp", headerFmt);
    xlsx.write("B3", "Records Total", headerFmt);
    xlsx.write("C3", "Records Added", headerFmt);
    xlsx.write("D3", "Records Updated", headerFmt);
    xlsx.write("E3", "Records Skipped", headerFmt);
    xlsx.write("F3", "Records Rejected", headerFmt);
    xlsx.write("G3", "Errors", headerFmt);

    int row1 = 4;
    int row2 = 4;
    
    int sumAdded = 0;
    int sumUpdated = 0;
    int sumSkipped = 0;
    int sumRejected = 0;
    int sumErrors = 0;

    for (int i = 0; i < m_model->rowCount(); ++i) {
        QString tsStr = m_model->item(i, 1)->text();
        QDateTime dt = QDateTime::fromString(tsStr, "yyyy-MM-dd HH:mm:ss");
        if (dt.isValid() && (dt < startDate || dt > endDate)) continue;

        QString comp = m_model->item(i, 3)->text();
        if (!jobName.isEmpty() && !comp.contains(jobName, Qt::CaseInsensitive)) continue;

        QString msg = m_model->item(i, 4)->text();
        // Remove invalid XML control characters which can corrupt sharedStrings.xml
        msg.remove(QRegularExpression("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F]"));
        
        if (!topic.isEmpty() && !msg.contains(topic, Qt::CaseInsensitive)) continue;

        // Excel cell character limit is 32,767. Truncate to prevent sharedStrings.xml corruption.
        QString excelMsg = msg;
        if (excelMsg.length() > 32700) {
            excelMsg = excelMsg.left(32700) + "... (truncated)";
        }

        xlsx.selectSheet("Batch-Uploads");
        xlsx.write(row1, 1, tsStr);
        xlsx.write(row1, 2, excelMsg);
        row1++;
        
        QRegularExpression totalRe("Records Total\\s*[:=]\\s*(\\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression addedRe("Records Added\\s*[:=]\\s*(\\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression updatedRe("Records Updated\\s*[:=]\\s*(\\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression skippedRe("Records Skipped\\s*[:=]\\s*(\\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression rejectedRe("Records Rejected\\s*[:=]\\s*(\\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression errorsRe("Errors\\s*[:=]\\s*(\\d+)", QRegularExpression::CaseInsensitiveOption);
        
        auto matchTotal = totalRe.match(msg);
        auto matchAdded = addedRe.match(msg);
        auto matchUpdated = updatedRe.match(msg);
        auto matchSkipped = skippedRe.match(msg);
        auto matchRejected = rejectedRe.match(msg);
        auto matchErrors = errorsRe.match(msg);
        
        bool isRawResponse = msg.contains("Response:", Qt::CaseInsensitive) && 
                             (msg.contains("Upload | Target:", Qt::CaseInsensitive) || msg.contains("CORITY_SAAS", Qt::CaseInsensitive));
        
        if (!isRawResponse && (matchTotal.hasMatch() || matchAdded.hasMatch() || matchUpdated.hasMatch() || matchSkipped.hasMatch() || matchRejected.hasMatch() || matchErrors.hasMatch())) {
            xlsx.selectSheet("Upload-Report");
            xlsx.write(row2, 1, tsStr);

            int added = matchAdded.hasMatch() ? matchAdded.captured(1).toInt() : 0;
            int updated = matchUpdated.hasMatch() ? matchUpdated.captured(1).toInt() : 0;
            int skipped = matchSkipped.hasMatch() ? matchSkipped.captured(1).toInt() : 0;
            int rejected = matchRejected.hasMatch() ? matchRejected.captured(1).toInt() : 0;
            int errors = matchErrors.hasMatch() ? matchErrors.captured(1).toInt() : 0;
            
            sumAdded += added;
            sumUpdated += updated;
            sumSkipped += skipped;
            sumRejected += rejected;
            sumErrors += errors;

            int total = matchTotal.hasMatch() ? matchTotal.captured(1).toInt() : (added + updated + skipped + rejected + errors);
            
            xlsx.write(row2, 2, total);
            if (matchAdded.hasMatch()) xlsx.write(row2, 3, added);
            if (matchUpdated.hasMatch()) xlsx.write(row2, 4, updated);
            if (matchSkipped.hasMatch()) xlsx.write(row2, 5, skipped);
            if (matchRejected.hasMatch()) xlsx.write(row2, 6, rejected);
            if (matchErrors.hasMatch()) xlsx.write(row2, 7, errors);
            
            row2++;
        }
    }
    
    // Write sum formulas in Upload-Report starting at I3 horizontally
    xlsx.selectSheet("Upload-Report");
    int lastDataRow = row2 > 4 ? row2 - 1 : 4;
    
    xlsx.write("I3", "Records Total", headerFmt);
    xlsx.write("I4", QString("=SUM(B4:B%1)").arg(lastDataRow));
    
    xlsx.write("J3", "Records Added", headerFmt);
    xlsx.write("J4", QString("=SUM(C4:C%1)").arg(lastDataRow));
    
    xlsx.write("K3", "Records Updated", headerFmt);
    xlsx.write("K4", QString("=SUM(D4:D%1)").arg(lastDataRow));
    
    xlsx.write("L3", "Records Skipped", headerFmt);
    xlsx.write("L4", QString("=SUM(E4:E%1)").arg(lastDataRow));
    
    xlsx.write("M3", "Records Rejected", headerFmt);
    xlsx.write("M4", QString("=SUM(F4:F%1)").arg(lastDataRow));
    
    xlsx.write("N3", "Errors", headerFmt);
    xlsx.write("N4", QString("=SUM(G4:G%1)").arg(lastDataRow));

    xlsx.addSheet("Chart");
    xlsx.selectSheet("Chart");
    
    xlsx.write("A1", "Upload Statistics Summary", titleFmt);
    xlsx.mergeCells("A1:B1", titleFmt);
    xlsx.write("A2", subTitleStr, subTitleFmt);
    xlsx.mergeCells("A2:B2", subTitleFmt);

    xlsx.write("A3", "Category", headerFmt);
    xlsx.write("B3", "Count", headerFmt);
    
    xlsx.write("A4", "Records Added");
    xlsx.write("B4", sumAdded);
    
    xlsx.write("A5", "Records Updated");
    xlsx.write("B5", sumUpdated);
    
    xlsx.write("A6", "Records Skipped");
    xlsx.write("B6", sumSkipped);
    
    xlsx.write("A7", "Records Rejected");
    xlsx.write("B7", sumRejected);
    
    xlsx.write("A8", "Errors");
    xlsx.write("B8", sumErrors);

    QXlsx::Chart *pieChart = xlsx.insertChart(3, 3, QSize(600, 400));
    if (pieChart) {
        pieChart->setChartType(QXlsx::Chart::CT_PieChart);
        pieChart->setChartTitle("Upload Statistics Distribution");
        pieChart->setChartLegend(QXlsx::Chart::Right);
        pieChart->addSeries(QXlsx::CellRange("A3:B8"), xlsx.currentWorksheet(), true, true, true);
    }

    xlsx.selectSheet("Batch-Uploads");
    xlsx.autosizeColumnWidth();
    
    xlsx.selectSheet("Upload-Report");
    xlsx.autosizeColumnWidth();
    
    xlsx.selectSheet("Chart");
    xlsx.autosizeColumnWidth();

    if (!xlsx.saveAs(fileName)) {
        QMessageBox::critical(this, "Error", "Failed to save the Excel file.");
    } else {
        QMessageBox::information(this, "Success", "Report exported successfully.");
    }
}
