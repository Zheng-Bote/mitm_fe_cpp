/**
 * SPDX-FileComment: Tests for the core-layer response envelope helpers
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#include <QtTest>
#include "ApiEnvelope.h"

using namespace mitm::api;

namespace {
QByteArray env(int status, const char* code, const char* data, const char* detail = "null") {
    return QString(R"({"id":"req-1","status":%1,"code":"%2","title":"T","detail":%3,"meta":{"timestamp":"","version":"3.5.0"},"data":%4})")
        .arg(status).arg(code, detail, data).toUtf8();
}
}

class ApiEnvelopeTest : public QObject {
    Q_OBJECT
private slots:
    void unwrapsObjectAndArrayData() {
        QCOMPARE(QJsonDocument::fromJson(envelope::unwrap(env(200, "OK", R"({"bootstrap_required":true})"))).object().value("bootstrap_required").toBool(), true);
        QCOMPARE(QJsonDocument::fromJson(envelope::unwrap(env(200, "OK", R"([{"id":1},{"id":2}])"))).array().size(), 2);
    }

    void nullDataBecomesEmpty() {
        QVERIFY(envelope::unwrap(env(200, "DELETED", "null")).isEmpty());
    }

    void nonEnvelopeBodiesPassUnchanged() {
        const QByteArray flatbuffers("\x01\x02\x03", 3);
        QCOMPARE(envelope::unwrap(flatbuffers), flatbuffers);
        const QByteArray plain(R"({"tag_name":"v1.0.0"})");
        QCOMPARE(envelope::unwrap(plain), plain);
    }

    void errorDetailCodeAndFallback() {
        const QByteArray inactive = env(403, "USER_INACTIVE", "null", R"("Account is inactive.")");
        QCOMPARE(envelope::code(inactive), QString("USER_INACTIVE"));
        QCOMPARE(envelope::errorMessage(inactive, "fallback"), QString("Account is inactive."));
        QCOMPARE(envelope::errorMessage(env(500, "INTERNAL_ERROR", "null"), "fallback"), QString("T"));
        QCOMPARE(envelope::errorMessage("not json", "fallback"), QString("fallback"));
        QVERIFY(envelope::code("not json").isEmpty());
    }
};

QTEST_APPLESS_MAIN(ApiEnvelopeTest)
#include "ApiEnvelopeTest.moc"
