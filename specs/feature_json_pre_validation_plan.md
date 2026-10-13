# Architecture Plan: Global JSON Pre-Validation

## Current State Analysis
A code search reveals that JSON inputs in multiple dialogs (`RuleEditorDialog.cpp`, `TransformationWidget.cpp`, `JobEditorDialog.cpp`) are either silently defaulting to empty arrays/objects on invalid syntax using `catch(...)`, or not catching the exception at all (which crashes the frontend).
Example:
```cpp
try { j["validation_chain"] = json::parse(m_validationEdit->toPlainText().toStdString()); } 
catch(...) { j["validation_chain"] = json::array(); }
```

## Solution Design

### 1. Global Utility: `JsonValidator`
Create a helper class/namespace (`src/JsonValidator.h`/`src/JsonValidator.cpp`) that provides a static method:
```cpp
static bool validate(const QString& jsonString, QString& outErrorMessage);
```
Under the hood, this will use `nlohmann::json::parse(..., nullptr, false)` or `try-catch` to retrieve the `json::parse_error` details (specifically byte offset and error message) and return them gracefully to the caller.

### 2. UI Hooking
Instead of relying on late, silent `catch(...)` blocks during the `accept()` or `save()` logic, the UI components will intercept the save action, validate all JSON fields using `JsonValidator::validate()`, and stop the action if validation fails.

Affected dialogs / components:
- `src/RuleEditorDialog.cpp`: `m_transformEdit` (Transform Chain) and `m_validationEdit` (Validation Chain).
- `src/TransformationWidget.cpp`: Add/Edit Function (`ePar` parameters) and Add/Edit Validation (`ePar` parameters).
- `src/JobEditorDialog.cpp`: `m_argsEdit` (Job Arguments).

### 3. Error Feedback
When `JsonValidator::validate()` returns false, the dialog will show a `QMessageBox::critical` (or `warning`) displaying the exact `outErrorMessage` from `nlohmann::json`, allowing the user to easily find and fix the syntax issue.

## Drift Control / SpecDD Checks
- Does not change network payload schemas, only ensures compliance before dispatch.
- Purely local Qt6 frontend functionality.
- Uses existing `nlohmann::json` dependency.
