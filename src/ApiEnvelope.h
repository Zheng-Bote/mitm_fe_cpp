/**
 * SPDX-FileComment: Response envelope of the core-layer API (core-layer #93)
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

// Every JSON response under /api/v1 is {id, status, code, title, detail, meta, data}.
namespace mitm::api::envelope {

/// The envelope object of a response body, or an empty object if the body is no envelope.
inline QJsonObject parse(const QByteArray& body) {
    const QJsonObject obj = QJsonDocument::fromJson(body).object();
    for (const char* key : {"id", "status", "code", "meta", "data"}) {
        if (!obj.contains(QLatin1String(key))) {
            return {};
        }
    }
    return obj;
}

/// The `data` payload of an envelope as JSON (object or array); empty for `data: null`.
/// A body that is no envelope (e.g. FlatBuffers) is returned unchanged.
inline QByteArray unwrap(const QByteArray& body) {
    const QJsonObject env = parse(body);
    if (env.isEmpty()) {
        return body;
    }
    const QJsonValue data = env.value("data");
    if (data.isObject()) {
        return QJsonDocument(data.toObject()).toJson(QJsonDocument::Compact);
    }
    if (data.isArray()) {
        return QJsonDocument(data.toArray()).toJson(QJsonDocument::Compact);
    }
    return {};
}

/// The application `code` of an error envelope (e.g. `USER_INACTIVE`), or empty.
inline QString code(const QByteArray& body) {
    return parse(body).value("code").toString();
}

/// `detail` (or `title`) of an error envelope, or `fallback`.
inline QString errorMessage(const QByteArray& body, const QString& fallback) {
    const QJsonObject env = parse(body);
    const QString detail = env.value("detail").toString();
    if (!detail.isEmpty()) {
        return detail;
    }
    const QString title = env.value("title").toString();
    return title.isEmpty() ? fallback : title;
}

} // namespace mitm::api::envelope
