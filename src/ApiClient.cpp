/**
 * SPDX-FileComment: ApiClient
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ApiClient.h"
#include "Config.h"
#include <QUrl>
#include <QDebug>
#include <QTimer>
#include <QElapsedTimer>
#include <spdlog/spdlog.h>

namespace mitm::api {

ApiClient::ApiClient(QObject* parent) 
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
}

QNetworkRequest ApiClient::createRequest(const QString& path) const {
    auto& config = mitm::config::ConfigManager::GetInstance();
    QUrl url(config.GetHostUrl() + path);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", config.GetAuthHeader().toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(10000);
    return request;
}

void ApiClient::handleReply(QNetworkReply* reply, const SuccessCallback& onSuccess, const ErrorCallback& onError) {
    QElapsedTimer* timer = new QElapsedTimer();
    timer->start();

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, onSuccess, onError]() {
        qint64 elapsed = timer->elapsed();
        delete timer;
        reply->deleteLater();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode == 401 || statusCode == 403) {
            emit unauthorized();
        }

        QString method = reply->request().attribute(QNetworkRequest::CustomVerbAttribute).toString();
        if (method.isEmpty()) {
            switch (reply->operation()) {
                case QNetworkAccessManager::GetOperation: method = "GET"; break;
                case QNetworkAccessManager::PostOperation: method = "POST"; break;
                case QNetworkAccessManager::PutOperation: method = "PUT"; break;
                case QNetworkAccessManager::DeleteOperation: method = "DELETE"; break;
                default: method = "UNKNOWN"; break;
            }
        }
        QString urlPath = reply->url().path();

        if (reply->error() == QNetworkReply::NoError) {
            spdlog::info("[Telemetry] API SUCCESS: {} {} - Latency: {}ms - Status: {}", method.toStdString(), urlPath.toStdString(), elapsed, statusCode);
            if (onSuccess) {
                onSuccess(reply->readAll(), reply);
            }
        } else {
            spdlog::warn("[Telemetry] API ERROR: {} {} - Latency: {}ms - Status: {} - Error: {}", method.toStdString(), urlPath.toStdString(), elapsed, statusCode, reply->errorString().toStdString());
            if (onError) {
                onError(statusCode, reply->errorString());
            } else {
                qWarning() << "API Error:" << statusCode << reply->errorString();
            }
        }
    });
}

void ApiClient::get(const QString& path, const SuccessCallback& onSuccess, const ErrorCallback& onError) {
    QNetworkRequest request = createRequest(path);
    QNetworkReply* reply = m_manager->get(request);
    handleReply(reply, onSuccess, onError);
}

void ApiClient::post(const QString& path, const QByteArray& data, const SuccessCallback& onSuccess, const ErrorCallback& onError) {
    QNetworkRequest request = createRequest(path);
    QNetworkReply* reply = m_manager->post(request, data);
    handleReply(reply, onSuccess, onError);
}

void ApiClient::post(const QString& path, QHttpMultiPart* multiPart, const SuccessCallback& onSuccess, const ErrorCallback& onError) {
    QNetworkRequest request = createRequest(path);
    // Qt overrides Content-Type for multipart, but we can explicitly clear it just in case
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant());
    QNetworkReply* reply = m_manager->post(request, multiPart);
    multiPart->setParent(reply);
    handleReply(reply, onSuccess, onError);
}

void ApiClient::put(const QString& path, const QByteArray& data, const SuccessCallback& onSuccess, const ErrorCallback& onError) {
    QNetworkRequest request = createRequest(path);
    QNetworkReply* reply = m_manager->put(request, data);
    handleReply(reply, onSuccess, onError);
}

void ApiClient::deleteResource(const QString& path, const SuccessCallback& onSuccess, const ErrorCallback& onError) {
    QNetworkRequest request = createRequest(path);
    QNetworkReply* reply = m_manager->deleteResource(request);
    handleReply(reply, onSuccess, onError);
}

} // namespace mitm::api
