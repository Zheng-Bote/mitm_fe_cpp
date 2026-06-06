#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

class SettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    ~SettingsWidget() override = default;

private slots:
    void onUnlockVault();

private:
    QLineEdit* m_masterKeyInput;
    QPushButton* m_unlockButton;
};
