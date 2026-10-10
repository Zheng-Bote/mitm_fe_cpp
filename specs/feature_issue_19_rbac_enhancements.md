# Feature Specification: RBAC Enhancements (Issue #19)

## 1. Description
This specification details the frontend enhancements required to support the new RBAC profile fields (`first_name`, `last_name`, `is_active`) and session management features introduced in the core-layer (Issue #53). 

## 2. Requirements
- **Edit User**: Add a UI dialog allowing users with the "ADMIN" role to edit a user's `first_name`, `last_name`, and `is_active` status. This involves calling `PUT /api/v1/iam/users/:id`.
- **Add User**: Update the existing "Add User" form/dialog to include fields for `first_name`, `last_name`, and `is_active`.
- **Authentication (`/api/v1/auth/session`)**: Modify the authentication request to include the client's local IP address (`client_ip`) in the payload.
- **User Profile (`/api/v1/auth/me`)**: Update the frontend models to parse and display the new profile fields (`first_name`, `last_name`, `client_ip`, and `is_active`) returned by the `/api/v1/auth/me` endpoint.
- **Session Management**: Add a "Terminate Session" button in the RBAC user management view (visible only to ADMIN users) that calls `DELETE /api/v1/iam/users/:id/session`.
- **Inactive User Handling**: The frontend must gracefully handle the scenario where the backend rejects a login attempt due to the user being inactive (`is_active = false`), displaying an appropriate error message to the user.
- **Session Renewal**: If the application is running and the user is logged in, the application should automatically attempt to renew or extend the session before it expires, preventing unexpected logouts during active use.

## 3. Scope Boundaries
- This issue only covers the C++ frontend (ImGui based) changes. The corresponding backend changes were implemented in the core-layer under Issue #53.
- The `client_ip` should be resolved locally as best effort before sending to the backend.

## 4. Architecture / Integration
- API client models in `include/api/` will be updated to reflect the new JSON payloads.
- The UI layer (e.g., `src/ui/rbac_window.cpp` or similar) will be updated with ImGui input fields and buttons.
