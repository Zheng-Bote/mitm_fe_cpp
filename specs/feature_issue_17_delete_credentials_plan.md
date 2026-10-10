# Plan: Delete Source and Target Credentials

1. **Add Delete Button in UI**
   - **`SourcesWidget`**: Add a "Delete Selected" (`QPushButton`) button next to the "Add Credential" button in the layout.
   - **`TargetCredentialsWidget`**: Add a "Delete Selected" (`QPushButton`) button next to the "Add Credential" button in the layout.

2. **Implement Delete Logic for Source Credentials**
   - Connect the delete button's `clicked` signal in `SourcesWidget`.
   - Ensure an item is selected from the `QTableWidget` (`m_table`).
   - Extract the `ID` from the selected row.
   - Show a `QMessageBox::question` confirming the deletion.
   - On confirmation, call `mitm::api::ApiClient::instance().deleteResource("/api/v1/config/credentials/" + id, ...)` (Wait, the delete API is `deleteResource`).
   - On success, reload the sources list (`onRefresh()`).

3. **Implement Delete Logic for Target Credentials**
   - Connect the delete button's `clicked` signal in `TargetCredentialsWidget`.
   - Ensure an item is selected from the `QTableWidget` (`m_table`).
   - Extract the `ID` from the selected row.
   - Show a `QMessageBox::question` confirming the deletion.
   - On confirmation, call `mitm::api::ApiClient::instance().deleteResource("/api/v1/config/targets/" + id, ...)`
   - On success, reload the targets list (`onRefresh()`).

4. **Verify and Update Changelog**
   - Compile the frontend and verify it works.
   - Update `CHANGELOG.md` with the new feature under `Unreleased`.
