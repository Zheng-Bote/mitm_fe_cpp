# Feature Spec: Global JSON Pre-Validation

## Related Issue
[Issue #15: Feature: Global JSON Pre-Validation for Input Fields](https://github.com/Zheng-Bote/mitm_fe_cpp/issues/15)

## Problem / Intent
Currently, JSON input fields (like the Validation Chain configuration) allow malformed JSON to be submitted, causing silent failures or backend rejections. A global frontend JSON validation check is required to intercept invalid syntax (like unescaped backslashes or literal newlines) and provide immediate user feedback.

## Requirements (EARS)
1. **The system shall** provide a global `JsonValidator` (or similar utility function) capable of parsing a JSON string and returning success or detailed error information.
2. **When** a user interacts with any JSON input field and triggers a save/submit action, **the system shall** parse the input using the global utility.
3. **If** the JSON syntax is invalid, **the system shall** abort the save operation.
4. **If** the JSON syntax is invalid, **the system shall** display an immediate visual error message to the user containing the specific parse error (e.g., line number and issue).

## Scope
`admin-frontend/mitm_fe_cpp`
- JSON validation utility layer
- All UI forms/components that take JSON as input

## Constraints & SpecDD Alignment
- Isolated feature: Only UI logic and local string parsing is affected.
- No changes to backend APIs or database schemas.
- Must use standard C++ libraries or the project's existing JSON library (e.g., `nlohmann/json`) to ensure compatibility.
- Ensure cross-platform compatibility (Linux/Windows/macOS) as typical for C++ frontend apps.

## Acceptance Criteria
- [ ] Global JSON validation method exists and is unit-tested.
- [ ] Validation is hooked into all existing JSON text inputs in the UI.
- [ ] Attempting to save `[{"pattern": "^(0[1-9])\n"}]` (invalid newline) triggers a UI error and prevents the save.
- [ ] Attempting to save valid JSON succeeds.
