#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <nlohmann/json.hpp>

class TransformationWidget : public QWidget {
    Q_OBJECT
public:
    explicit TransformationWidget(QWidget *parent = nullptr);

private slots:
    void onRefreshSources();
    void onAddSource();
    void onEditSource();
    void onDeleteSource();

    void onRefreshTargets();
    void onAddTarget();
    void onEditTarget();
    void onDeleteTarget();

    void onRefreshRules();
    void onAddRule();
    void onEditRule();
    void onDeleteRule();

    void onRefreshTransformations();
    void onAddTransformation();
    void onEditTransformation();
    void onDeleteTransformation();

    void onRefreshValidations();
    void onAddValidation();
    void onEditValidation();
    void onDeleteValidation();

private:
    QNetworkAccessManager* m_networkManager;

    QTabWidget* m_tabWidget;

    QTableWidget* m_sourcesTable;
    QTableWidget* m_targetsTable;
    QTableWidget* m_rulesTable;
    QTableWidget* m_transformTable;
    QTableWidget* m_validTable;

    QPushButton* m_refreshSourcesBtn;
    QPushButton* m_addSourceBtn;
    QPushButton* m_editSourceBtn;
    QPushButton* m_deleteSourceBtn;

    QPushButton* m_refreshTargetsBtn;
    QPushButton* m_addTargetBtn;
    QPushButton* m_editTargetBtn;
    QPushButton* m_deleteTargetBtn;

    QPushButton* m_refreshRulesBtn;
    QPushButton* m_addRuleBtn;
    QPushButton* m_editRuleBtn;
    QPushButton* m_deleteRuleBtn;

    QPushButton* m_refreshTransformBtn;
    QPushButton* m_addTransformBtn;
    QPushButton* m_editTransformBtn;
    QPushButton* m_deleteTransformBtn;

    QPushButton* m_refreshValidBtn;
    QPushButton* m_addValidBtn;
    QPushButton* m_editValidBtn;
    QPushButton* m_deleteValidBtn;

    QString getAuthHeader();
    void deleteEntity(const QString& endpoint, const QString& id, const std::function<void()>& onSuccess);
    void postEntity(const QString& endpoint, const nlohmann::json& payload, const std::function<void()>& onSuccess);
    void setupSourcesTab(QWidget* tab);
    void setupTargetsTab(QWidget* tab);
    void setupRulesTab(QWidget* tab);
    void setupTransformationsTab(QWidget* tab);
    void setupValidationsTab(QWidget* tab);
};
