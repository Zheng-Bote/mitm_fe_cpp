# Feature Specification: Delete Source and Target Credentials

## Feature Intent
Enable users to delete selected Source and Target Credentials from the UI. Currently, the application lacks a "delete selected item" functionality in the Source / Target Credentials views.

## Requirements (EARS Syntax)
1. While the user is in the Source Credentials view, if the user selects an item and clicks "Delete", the system shall prompt for confirmation.
2. While the user is in the Target Credentials view, if the user selects an item and clicks "Delete", the system shall prompt for confirmation.
3. When the user confirms the deletion, the system shall call the corresponding delete API endpoint to remove the credential.
4. If the deletion is successful, the system shall refresh the credentials list in the UI.

## Scope
Admin Frontend (mitm_fe_cpp):
- SourcesWidget
- TargetCredentialsWidget

## SpecDD Architecture Alignment (Drift Control)
- [x] **Architecture:** The layered architecture is maintained (no direct bypass from Collector to Delivery).
- [ ] **Architecture:** Feature affects architecture: SpecKit feature forces update of the SpecDD .sdd
- [x] **Security:** Envelope Encryption (AES-GCM) is NOT bypassed for PII data.
- [x] **Data Model:** Core PostgreSQL schemas remain intact.
- [x] **Standards:** SPDX headers and English documentation will be maintained.

## Acceptance Criteria
- [ ] "Delete" button is available in the Source Credentials view and functions correctly.
- [ ] "Delete" button is available in the Target Credentials view and functions correctly.
- [ ] Deletion requires a confirmation prompt.
- [ ] CHANGELOG.md and README.md are up-to-date.
