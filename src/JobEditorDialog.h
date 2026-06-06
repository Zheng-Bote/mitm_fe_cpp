#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <nlohmann/json.hpp>

class JobEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit JobEditorDialog(QWidget *parent = nullptr);
    void setJob(const nlohmann::json& job);
    nlohmann::json getJob() const;

private:
    QLineEdit* m_nameEdit;
    QLineEdit* m_commandEdit;
    QLineEdit* m_argsEdit; // JSON als String
    QCheckBox* m_enabledCheck;
    QCheckBox* m_restartCheck;
    
    // Einfacher Cron Editor
    QComboBox* m_cronMin;
    QComboBox* m_cronHour;
    QComboBox* m_cronDom;
    QComboBox* m_cronMonth;
    QComboBox* m_cronDow;

    int m_jobId;
    
    void setupUi();
    void populateCronCombo(QComboBox* cb, int min, int max, const QStringList& extras = {});
};
