#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <functional>
#include <QHttpMultiPart>

namespace mitm::api {

class ApiClient : public QObject {
    Q_OBJECT
public:
    static ApiClient& instance() {
        static ApiClient instance;
        return instance;
    }

    using SuccessCallback = std::function<void(const QByteArray&, QNetworkReply*)>;
    using ErrorCallback = std::function<void(int statusCode, const QString& errorString)>;

    void get(const QString& path, const SuccessCallback& onSuccess, const ErrorCallback& onError = nullptr);
    void post(const QString& path, const QByteArray& data, const SuccessCallback& onSuccess, const ErrorCallback& onError = nullptr);
    void post(const QString& path, QHttpMultiPart* multiPart, const SuccessCallback& onSuccess, const ErrorCallback& onError = nullptr);
    void put(const QString& path, const QByteArray& data, const SuccessCallback& onSuccess, const ErrorCallback& onError = nullptr);
    void deleteResource(const QString& path, const SuccessCallback& onSuccess, const ErrorCallback& onError = nullptr);

signals:
    void unauthorized();

private:
    explicit ApiClient(QObject* parent = nullptr);
    ~ApiClient() override = default;

    ApiClient(const ApiClient&) = delete;
    ApiClient& operator=(const ApiClient&) = delete;

    QNetworkRequest createRequest(const QString& path) const;
    void handleReply(QNetworkReply* reply, const SuccessCallback& onSuccess, const ErrorCallback& onError);

    QNetworkAccessManager* m_manager;
};

} // namespace mitm::api
