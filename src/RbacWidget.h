#ifndef RBAC_WIDGET_H
#define RBAC_WIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>

class RbacWidget : public QWidget {
    Q_OBJECT

public:
    explicit RbacWidget(QWidget *parent = nullptr);

private slots:
    void fetchUsersAndRoles();
    void saveRoleAssignments();
    void onUserSelected();
    void onAddUserClicked();
    void onRemoveUserClicked();

private:
    void setupUi();

    QTableWidget* usersTable;
    QListWidget* rolesList;
    QPushButton* saveBtn;
    QPushButton* refreshBtn;
    QPushButton* addUserBtn;
    QPushButton* removeUserBtn;

    // Map to store current user's roles
    int currentUserId = -1;
};

#endif // RBAC_WIDGET_H
