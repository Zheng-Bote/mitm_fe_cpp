# Tasks: Global JSON Pre-Validation

- **[x] Task 1:** Create `src/JsonValidator.h` and `src/JsonValidator.cpp` providing `JsonValidator::validate(const QString& jsonString, QString& outErrorMessage)`. Update `CMakeLists.txt` to include `src/JsonValidator.cpp`.
- **[x] Task 2:** Update `src/RuleEditorDialog.cpp`. In `accept()` (or where save occurs), validate `m_transformEdit` and `m_validationEdit`. If invalid, show `QMessageBox` and `return`. Remove the silent `catch(...)` overrides.
- **[x] Task 3:** Update `src/TransformationWidget.cpp`. In `onEditFunction()` and `onEditValidation()` save callbacks, validate the parameters text field (`ePar`). Show error and abort if invalid. Remove silent `catch(...)` overrides.
- **[x] Task 4:** Update `src/JobEditorDialog.cpp`. In `accept()`, validate `m_argsEdit`. Show error and abort if invalid.
- **[x] Task 5:** Compile the C++ frontend to verify changes. Update `CHANGELOG.md`.
