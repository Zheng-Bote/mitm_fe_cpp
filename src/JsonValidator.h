#pragma once

#include <QString>

class JsonValidator {
public:
    static bool validate(const QString& jsonString, QString& outErrorMessage);
};
