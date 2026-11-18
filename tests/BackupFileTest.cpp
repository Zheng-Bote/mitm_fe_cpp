/**
 * SPDX-FileComment: Tests for the encrypted backup helpers (issue #49)
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#include <QtTest>
#include "BackupFile.h"

using namespace mitm::backup;

namespace {
const QByteArray kEncrypted = R"({"format":1,"server_version":"3.6.0","created_at":"x","kdf":{"algorithm":"argon2id","salt":"c2FsdA=="},"cipher":{"algorithm":"AES-256-GCM","nonce":"bm9uY2U="},"ciphertext":"Y3Q="})";
}

class BackupFileTest : public QObject {
    Q_OBJECT
private slots:
    void passphraseRules() {
        QVERIFY(passphraseError("short", "short", true).contains("12"));
        QVERIFY(passphraseError("long enough pass", "different pass!", true).contains("do not match"));
        QVERIFY(passphraseError("long enough pass", "long enough pass", true).isEmpty());
        QVERIFY(passphraseError("long enough pass", "", false).isEmpty());
    }

    void classifiesFiles() {
        QCOMPARE(classify(kEncrypted), Kind::Encrypted);
        QCOMPARE(classify(R"({"version":"3.5.0","data":{"scheduled_programs":[]}})"), Kind::LegacyUnencrypted);
        QCOMPARE(classify("not json"), Kind::Invalid);
        QCOMPARE(classify(R"({"foo":1})"), Kind::Invalid);
    }

    void requestBodies() {
        const QJsonObject backup = QJsonDocument::fromJson(backupBody("p\"ass phrase 1")).object();
        QCOMPARE(backup.value("passphrase").toString(), QString("p\"ass phrase 1"));

        const QJsonObject restore = QJsonDocument::fromJson(restoreBody(kEncrypted, "pass phrase 12")).object();
        QCOMPARE(restore.value("passphrase").toString(), QString("pass phrase 12"));
        QCOMPARE(restore.value("backup").toObject(), QJsonDocument::fromJson(kEncrypted).object());
    }

    void fileNameUsesEncryptedExtension() {
        QCOMPARE(fileName("/b", "2026-11-18_120000", "dev", "alice"), QString("/b/2026-11-18_120000_backup-dev_alice.mitmbak"));
    }

    void summarizesRestore() {
        const QString s = restoreSummary(R"({"tables":{"roles":4,"admin_users":6},"deactivated_programs":["job_c"],"scheduler_reloaded":false})");
        QVERIFY(s.contains("2 tables, 10 rows"));
        QVERIFY(s.contains("job_c"));
        QVERIFY(s.contains("scheduler could not be reloaded"));
        QVERIFY(!restoreSummary(R"({"tables":{},"deactivated_programs":[],"scheduler_reloaded":true})").contains("Warning"));
    }
};

QTEST_APPLESS_MAIN(BackupFileTest)
#include "BackupFileTest.moc"
