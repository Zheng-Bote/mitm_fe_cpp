/**
 * SPDX-FileComment: Helpers for the encrypted configuration backup (issue #49)
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file BackupFile.h
 * @brief Passphrase checks, backup file classification and request bodies for
 *        POST /api/v1/system/backup and /restore (mitm_http-server >= 3.6.0)
 */

#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace mitm::backup {

/// Minimum passphrase length enforced by mitm_http-server.
constexpr int kMinPassphraseChars = 12;
/// File extension of encrypted backups.
inline const QString kFileExtension = QStringLiteral("mitmbak");

/// Returns an error text, or an empty string if the passphrase is acceptable.
inline QString passphraseError(const QString& passphrase, const QString& confirmation, bool needsConfirmation) {
    if (passphrase.size() < kMinPassphraseChars) {
        return QString("The passphrase must have at least %1 characters.").arg(kMinPassphraseChars);
    }
    if (needsConfirmation && passphrase != confirmation) {
        return "The passphrases do not match.";
    }
    return {};
}

enum class Kind { Encrypted, LegacyUnencrypted, Invalid };

/// Classifies the content of a backup file.
inline Kind classify(const QByteArray& content) {
    const QJsonDocument doc = QJsonDocument::fromJson(content);
    if (!doc.isObject()) return Kind::Invalid;
    const QJsonObject o = doc.object();
    if (o.contains("format") && o.contains("ciphertext") && o.contains("kdf") && o.contains("cipher")) return Kind::Encrypted;
    if (o.contains("version") && o.contains("data")) return Kind::LegacyUnencrypted;
    return Kind::Invalid;
}

/// Body of POST /api/v1/system/backup.
inline QByteArray backupBody(const QString& passphrase) {
    return QJsonDocument(QJsonObject{{"passphrase", passphrase}}).toJson(QJsonDocument::Compact);
}

/// Body of POST /api/v1/system/restore; the document is embedded unchanged.
inline QByteArray restoreBody(const QByteArray& document, const QString& passphrase) {
    return QJsonDocument(QJsonObject{{"passphrase", passphrase}, {"backup", QJsonDocument::fromJson(document).object()}})
        .toJson(QJsonDocument::Compact);
}

/// File name of a new backup, e.g. <dir>/2026-11-18_120000_backup-dev_alice.mitmbak.
inline QString fileName(const QString& dir, const QString& timestamp, const QString& configName, const QString& osUser) {
    return QString("%1/%2_backup-%3_%4.%5").arg(dir, timestamp, configName, osUser, kFileExtension);
}

/// Human-readable summary of the restore response data {tables, deactivated_programs, scheduler_reloaded}.
inline QString restoreSummary(const QByteArray& data) {
    const QJsonObject o = QJsonDocument::fromJson(data).object();
    QStringList lines;
    const QJsonObject tables = o.value("tables").toObject();
    int rows = 0;
    for (auto it = tables.begin(); it != tables.end(); ++it) rows += it.value().toInt();
    lines << QString("%1 tables, %2 rows restored.").arg(tables.size()).arg(rows);
    QStringList deactivated;
    for (const auto& v : o.value("deactivated_programs").toArray()) deactivated << v.toString();
    if (!deactivated.isEmpty()) {
        lines << QString("Deactivated jobs (not in the backup): %1").arg(deactivated.join(", "));
    }
    if (o.contains("scheduler_reloaded") && !o.value("scheduler_reloaded").toBool()) {
        lines << "Warning: the scheduler could not be reloaded; restart it to apply the jobs.";
    }
    return lines.join("\n");
}

} // namespace mitm::backup
