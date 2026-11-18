/**
 * SPDX-FileComment: Tests for the version check helpers (issue #45)
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#include <QtTest>
#include "UpdateCheck.h"

using namespace mitm::update;

class UpdateCheckTest : public QObject {
    Q_OBJECT
private slots:
    void latestReleaseUrlUsesHomepage() {
        QCOMPARE(latestReleaseUrl("https://github.com/Zheng-Bote/mitm_fe_cpp").toString(),
                 QString("https://github.com/Zheng-Bote/mitm_fe_cpp/releases/latest"));
    }

    void versionFromTagUrl() {
        QCOMPARE(versionFromReleaseTagUrl(QUrl("https://github.com/Zheng-Bote/mitm_fe_cpp/releases/tag/v2.1.0")), QString("2.1.0"));
        QCOMPARE(versionFromReleaseTagUrl(QUrl("https://github.com/o/r/releases/tag/1.4.2")), QString("1.4.2"));
    }

    void noVersionWithoutTag() {
        // GitHub redirects to /releases when a repository has no release yet.
        QVERIFY(versionFromReleaseTagUrl(QUrl("https://github.com/o/r/releases")).isEmpty());
        QVERIFY(versionFromReleaseTagUrl(QUrl("https://github.com/login")).isEmpty());
    }

    void proxyHintOnlyForProxyErrors() {
        const QString proxy = failureText(QNetworkReply::ProxyAuthenticationRequiredError, 0, "Proxy requires authentication");
        QVERIFY(proxy.contains("Network-Proxy"));
        QVERIFY(proxy.contains("Proxy requires authentication"));

        const QString http = failureText(QNetworkReply::ContentAccessDenied, 403, "Forbidden");
        QCOMPARE(http, QString("Update check failed: HTTP 403 from GitHub."));

        const QString network = failureText(QNetworkReply::HostNotFoundError, 0, "Host github.com not found");
        QVERIFY(!network.contains("Network-Proxy"));
        QVERIFY(network.contains("Host github.com not found"));

        QVERIFY(failureText(QNetworkReply::NoError, 200, "").contains("no release information"));
    }
};

QTEST_APPLESS_MAIN(UpdateCheckTest)
#include "UpdateCheckTest.moc"
