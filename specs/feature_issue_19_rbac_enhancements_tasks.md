# Tasks: RBAC Enhancements (Frontend Issue #19)

- **[ ] Task 1:** Update `AuthResponse` and `User` API models in `include/api/` (or similar) to include `first_name`, `last_name`, `is_active`, and `client_ip`.
- **[ ] Task 2:** Update HTTP API Client (`src/api/`):
  - Pass `client_ip` in `POST /api/v1/auth/session` payload.
  - Implement `PUT /api/v1/iam/users/:id` for editing users.
  - Implement `DELETE /api/v1/iam/users/:id/session` for terminating sessions.
- **[ ] Task 3:** Implement UI for handling `is_active = false` login rejections with clear error feedback in the Login Window.
- **[ ] Task 4:** Update RBAC User Management UI (`src/ui/`):
  - Display `first_name`, `last_name`, and `is_active` in the users table.
  - Modify the "Add User" dialog to support the new fields.
  - Create the "Edit User" dialog for editing profile fields.
  - Add the "Terminate Session" button for ADMIN users.
- **[ ] Task 5:** Implement background session keep-alive (periodic ping to `/api/v1/auth/me`) and seamless session renewal mechanism if credentials are cached.
