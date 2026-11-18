/**
 * SPDX-FileComment: Version check against the latest GitHub release without the GitHub API
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file UpdateCheck.h
 * @brief Reads the latest release from the redirect of <homepage>/releases/latest (issue #45)
 */

#pragma once

#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QUrl>
#include <functional>

namespace mitm::update {

/// URL that GitHub redirects to the tag of the latest release.
inline QUrl latestReleaseUrl(const QString& homepage) {
    return QUrl(homepage + "/releases/latest");
}

/// Returns the version of a ".../releases/tag/<tag>" URL without a leading "v", or an empty string.
inline QString versionFromReleaseTagUrl(const QUrl& url) {
    const QString marker = "/releases/tag/";
    const QString path = url.path();
    const qsizetype pos = path.lastIndexOf(marker);
    if (pos < 0) return {};
    QString tag = path.mid(pos + marker.size());
    if (tag.startsWith('v') || tag.startsWith('V')) tag = tag.mid(1);
    return tag;
}

inline bool isProxyError(QNetworkReply::NetworkError error) {
    switch (error) {
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
    case QNetworkReply::ProxyAuthenticationRequiredError:
    case QNetworkReply::UnknownProxyError:
        return true;
    default:
        return false;
    }
}

/// User-facing text for a failed version check; the proxy hint is shown for proxy errors only.
inline QString failureText(QNetworkReply::NetworkError error, int httpStatus, const QString& errorString) {
    if (isProxyError(error)) {
        return QString("Update check failed: %1<br/><i>Please check the proxy settings (Settings → Network-Proxy).</i>")
            .arg(errorString.toHtmlEscaped());
    }
    if (httpStatus >= 400) {
        return QString("Update check failed: HTTP %1 from GitHub.").arg(httpStatus);
    }
    if (error != QNetworkReply::NoError) {
        return QString("Update check failed: %1").arg(errorString.toHtmlEscaped());
    }
    return "Update check failed: no release information found.";
}

struct Result {
    bool ok{false};
    QString latest; ///< latest version without "v" (ok == true)
    QString error;  ///< user-facing failure text (ok == false)
};

/// Sends HEAD <homepage>/releases/latest through proxy and reports the tag of the redirect target.
inline void fetchLatestVersion(QObject* context, const QNetworkProxy& proxy, const QString& homepage,
                               std::function<void(const Result&)> done) {
    auto* manager = new QNetworkAccessManager(context);
    manager->setProxy(proxy);

    QNetworkRequest request(latestReleaseUrl(homepage));
    request.setRawHeader("User-Agent", "MitmAdminFrontend");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

    QNetworkReply* reply = manager->head(request);
    QObject::connect(reply, &QNetworkReply::finished, context, [reply, manager, done = std::move(done)]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QUrl target = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        Result result;
        if (reply->error() == QNetworkReply::NoError && status >= 300 && status < 400) {
            result.latest = versionFromReleaseTagUrl(reply->url().resolved(target));
            result.ok = !result.latest.isEmpty();
        }
        if (!result.ok) {
            result.error = failureText(reply->error(), status, reply->errorString());
        }
        reply->deleteLater();
        manager->deleteLater();
        done(result);
    });
}

} // namespace mitm::update
