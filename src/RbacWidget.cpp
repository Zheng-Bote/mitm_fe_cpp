#include "RbacWidget.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>

#include "Config.h"

RbacWidget::RbacWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void RbacWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("<h2>RBAC Management</h2>"));
    topLayout->addStretch();
    refreshBtn = new QPushButton("Refresh", this);
    topLayout->addWidget(refreshBtn);
    mainLayout->addLayout(topLayout);

    auto* contentLayout = new QHBoxLayout();
    
    // Users Table
    auto* usersLayout = new QVBoxLayout();
    
    auto* usersHeaderLayout = new QHBoxLayout();
    usersHeaderLayout->addWidget(new QLabel("Users"));
    usersHeaderLayout->addStretch();
    addUserBtn = new QPushButton("Add User", this);
    removeUserBtn = new QPushButton("Remove User", this);
    usersHeaderLayout->addWidget(addUserBtn);
    usersHeaderLayout->addWidget(removeUserBtn);
    usersLayout->addLayout(usersHeaderLayout);
    
    usersTable = new QTableWidget(0, 3, this);
    usersTable->setHorizontalHeaderLabels({"ID", "Username", "Active"});
    usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    usersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    usersTable->setSortingEnabled(true);
    usersLayout->addWidget(usersTable);
    
    // Roles List
    auto* rolesLayout = new QVBoxLayout();
    rolesLayout->addWidget(new QLabel("Assign Roles"));
    rolesList = new QListWidget(this);
    rolesList->setSelectionMode(QAbstractItemView::MultiSelection);
    rolesLayout->addWidget(rolesList);
    
    saveBtn = new QPushButton("Save Role Assignments", this);
    rolesLayout->addWidget(saveBtn);
    
    contentLayout->addLayout(usersLayout, 2);
    contentLayout->addLayout(rolesLayout, 1);
    
    mainLayout->addLayout(contentLayout);

    connect(refreshBtn, &QPushButton::clicked, this, &RbacWidget::fetchUsersAndRoles);
    connect(usersTable, &QTableWidget::itemSelectionChanged, this, &RbacWidget::onUserSelected);
    connect(saveBtn, &QPushButton::clicked, this, &RbacWidget::saveRoleAssignments);
    connect(addUserBtn, &QPushButton::clicked, this, &RbacWidget::onAddUserClicked);
    connect(removeUserBtn, &QPushButton::clicked, this, &RbacWidget::onRemoveUserClicked);
}

void RbacWidget::fetchUsersAndRoles() {
    auto manager = new QNetworkAccessManager(this);
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QString auth = mitm::config::ConfigManager::GetInstance().GetAuthHeader();

    // Fetch Roles
    QNetworkRequest rolesReq(QUrl(host + "/admin/rbac/roles"));
    rolesReq.setRawHeader("Authorization", auth.toLocal8Bit());
    
    auto rolesReply = manager->get(rolesReq);
    connect(rolesReply, &QNetworkReply::finished, this, [this, rolesReply]() {
        if (rolesReply->error() == QNetworkReply::NoError) {
            rolesList->clear();
            auto doc = QJsonDocument::fromJson(rolesReply->readAll());
            for (const auto& v : doc.array()) {
                auto obj = v.toObject();
                auto* item = new QListWidgetItem(obj["name"].toString());
                item->setData(Qt::UserRole, obj["id"].toInt());
                rolesList->addItem(item);
            }
        }
        rolesReply->deleteLater();
    });

    // Fetch Users
    QNetworkRequest usersReq(QUrl(host + "/admin/rbac/users"));
    usersReq.setRawHeader("Authorization", auth.toLocal8Bit());
    
    auto usersReply = manager->get(usersReq);
    connect(usersReply, &QNetworkReply::finished, this, [this, usersReply]() {
        if (usersReply->error() == QNetworkReply::NoError) {
            usersTable->setSortingEnabled(false);
            usersTable->setRowCount(0);
            auto doc = QJsonDocument::fromJson(usersReply->readAll());
            auto arr = doc.array();
            for (int i = 0; i < arr.size(); ++i) {
                auto obj = arr[i].toObject();
                usersTable->insertRow(i);
                
                auto* idItem = new QTableWidgetItem(QString::number(obj["id"].toInt()));
                usersTable->setItem(i, 0, idItem);
                
                auto* nameItem = new QTableWidgetItem(obj["username"].toString());
                usersTable->setItem(i, 1, nameItem);
                
                auto* activeItem = new QTableWidgetItem(obj["is_active"].toBool() ? "Yes" : "No");
                usersTable->setItem(i, 2, activeItem);
            }
            usersTable->setSortingEnabled(true);
        }
        usersReply->deleteLater();
    });
}

void RbacWidget::onUserSelected() {
    currentUserId = -1;
    rolesList->clearSelection();
    
    auto ranges = usersTable->selectedRanges();
    if (ranges.isEmpty()) return;
    
    int row = ranges.first().topRow();
    currentUserId = usersTable->item(row, 0)->text().toInt();

    auto manager = new QNetworkAccessManager(this);
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QString auth = mitm::config::ConfigManager::GetInstance().GetAuthHeader();

    QNetworkRequest req(QUrl(host + "/admin/rbac/user_roles?user_id=" + QString::number(currentUserId)));
    req.setRawHeader("Authorization", auth.toLocal8Bit());
    
    auto reply = manager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            auto doc = QJsonDocument::fromJson(reply->readAll());
            auto roleIdsArray = doc.array();
            QList<int> userRoles;
            for (const auto& v : roleIdsArray) {
                userRoles.append(v.toInt());
            }
            
            for (int i = 0; i < rolesList->count(); ++i) {
                auto* item = rolesList->item(i);
                int roleId = item->data(Qt::UserRole).toInt();
                if (userRoles.contains(roleId)) {
                    item->setSelected(true);
                }
            }
        }
        reply->deleteLater();
    });
}

void RbacWidget::saveRoleAssignments() {
    if (currentUserId == -1) {
        QMessageBox::warning(this, "Error", "Please select a user first.");
        return;
    }

    QJsonArray roleIds;
    for (int i = 0; i < rolesList->count(); ++i) {
        auto* item = rolesList->item(i);
        if (item->isSelected()) {
            roleIds.append(item->data(Qt::UserRole).toInt());
        }
    }

    QJsonObject payload;
    payload["user_id"] = currentUserId;
    payload["role_ids"] = roleIds;

    auto manager = new QNetworkAccessManager(this);
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QString auth = mitm::config::ConfigManager::GetInstance().GetAuthHeader();

    QNetworkRequest req(QUrl(host + "/admin/rbac/assign"));
    req.setRawHeader("Authorization", auth.toLocal8Bit());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    auto reply = manager->post(req, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Success", "Roles assigned successfully.");
        } else {
            QMessageBox::critical(this, "Error", "Failed to assign roles: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void RbacWidget::onAddUserClicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add New User");
    dialog.resize(300, 150);

    auto* layout = new QFormLayout(&dialog);
    auto* userEdit = new QLineEdit(&dialog);
    auto* passEdit = new QLineEdit(&dialog);
    passEdit->setEchoMode(QLineEdit::Password);

    layout->addRow("Username:", userEdit);
    layout->addRow("Password:", passEdit);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString username = userEdit->text().trimmed();
        QString password = passEdit->text();

        if (username.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(this, "Error", "Username and password cannot be empty.");
            return;
        }

        QJsonObject payload;
        payload["username"] = username;
        payload["password"] = password;

        auto manager = new QNetworkAccessManager(this);
        QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
        QString auth = mitm::config::ConfigManager::GetInstance().GetAuthHeader();

        QNetworkRequest req(QUrl(host + "/admin/rbac/user/create"));
        req.setRawHeader("Authorization", auth.toLocal8Bit());
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        auto reply = manager->post(req, QJsonDocument(payload).toJson());
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QMessageBox::information(this, "Success", "User added successfully.");
                fetchUsersAndRoles();
            } else {
                QMessageBox::critical(this, "Error", "Failed to add user: " + reply->errorString());
            }
            reply->deleteLater();
        });
    }
}

void RbacWidget::onRemoveUserClicked() {
    if (currentUserId == -1) {
        QMessageBox::warning(this, "Error", "Please select a user to remove.");
        return;
    }

    auto replyAction = QMessageBox::question(this, "Confirm", "Are you sure you want to remove user ID " + QString::number(currentUserId) + "?", QMessageBox::Yes | QMessageBox::No);
    if (replyAction != QMessageBox::Yes) {
        return;
    }

    auto manager = new QNetworkAccessManager(this);
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    QString auth = mitm::config::ConfigManager::GetInstance().GetAuthHeader();

    QNetworkRequest req(QUrl(host + "/admin/rbac/user/delete?id=" + QString::number(currentUserId)));
    req.setRawHeader("Authorization", auth.toLocal8Bit());

    auto reply = manager->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Success", "User removed successfully.");
            currentUserId = -1;
            rolesList->clearSelection();
            fetchUsersAndRoles();
        } else {
            QMessageBox::critical(this, "Error", "Failed to remove user: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

