#include "JsonValidator.h"
#include <nlohmann/json.hpp>
#include <exception>

bool JsonValidator::validate(const QString& jsonString, QString& outErrorMessage) {
    // Empty strings might be treated as valid depending on context, but typically for arrays/objects they aren't valid JSON.
    // However, if we accept empty strings as empty objects/arrays in our app, we should handle it. 
    // Usually, our UI defaults to "{}" or "[]".
    if (jsonString.trimmed().isEmpty()) {
        outErrorMessage = "JSON cannot be empty.";
        return false;
    }

    try {
        (void)nlohmann::json::parse(jsonString.toStdString());
        return true;
    } catch (const nlohmann::json::parse_error& e) {
        outErrorMessage = QString::fromStdString(e.what());
        return false;
    } catch (const std::exception& e) {
        outErrorMessage = QString::fromStdString(e.what());
        return false;
    }
}
