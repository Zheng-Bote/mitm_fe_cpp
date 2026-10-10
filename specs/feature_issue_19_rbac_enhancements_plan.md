# Plan: RBAC Enhancements (Frontend Issue #19)

## 1. Overview
The goal is to update the C++ frontend to support the new user profile fields (`first_name`, `last_name`, `is_active`) and `client_ip` tracking introduced in the backend. We will also implement a periodic background task to keep the session alive (or renew it) and show proper feedback when login is rejected for inactive users.

## 2. API Client Updates
- **Models (`include/api/auth_models.h` or similar)**:
  - Add `first_name`, `last_name`, `is_active`, and `client_ip` to the Auth/User response models.
  - Update the Session Creation request model to include `client_ip`.
- **API Endpoints (`src/api/auth_client.cpp` & `src/api/iam_client.cpp` or similar)**:
  - Ensure the HTTP client passes `client_ip` when calling `POST /api/v1/auth/session`.
  - Add API function for `PUT /api/v1/iam/users/:id` to update user profiles.
  - Add API function for `DELETE /api/v1/iam/users/:id/session` to terminate a user's session.

## 3. UI Layer Updates
- **Login Window (`src/ui/login_window.cpp` or similar)**:
  - Detect login failure (`401 Unauthorized` or specific error string) due to inactive user and display a specific error message (e.g., "Login rejected: User account is inactive").
- **RBAC / User Management Window (`src/ui/rbac_window.cpp` or similar)**:
  - Add columns/display for `First Name`, `Last Name`, and `Active` status in the user list.
  - **Edit User Dialog**: When editing a user, show input fields for `first_name`, `last_name`, and a checkbox for `is_active`. Submit via the new `PUT` API.
  - **Add User Dialog**: Update the creation form to include the new profile fields.
  - **Terminate Session Button**: Add a button for "ADMIN" users in the user list that calls the terminate session API.

## 4. Session Renewal / Keep-Alive Mechanism
- **Application State / Event Loop**:
  - Implement a timer or polling mechanism that periodically (e.g., every 5-10 minutes) pings an authorized endpoint (like `GET /api/v1/auth/me`) to extend the 2h idle timeout.
  - If the absolute 24h timeout is approaching (or if a 401 is received despite the idle keep-alive), the application should attempt a seamless re-authentication using cached credentials (if stored securely in memory), or gracefully prompt the user to log in again without immediately dropping their current view state if possible.

## 5. Execution Steps
1. Update API models and JSON serialization/deserialization logic.
2. Implement local IP resolution (best effort) to populate `client_ip` for the session request.
3. Update the login UI to handle inactive user rejection.
4. Update the RBAC UI (tables and forms) for `Edit User` and `Add User`.
5. Implement the "Terminate Session" button.
6. Implement the background session keep-alive/renewal mechanism.
