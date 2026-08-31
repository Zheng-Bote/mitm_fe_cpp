# MitM Data Aggregator - Admin Frontend User Guide

Welcome to the **Admin Frontend** for the Man-in-the-Middle (MitM) Data Aggregator pipeline!

This native desktop application is built with modern **C++23** and **Qt 6**, designed to provide a highly responsive, secure, and user-friendly interface for monitoring and orchestrating the entire MitM data ingestion and delivery pipeline.

---

## 1. Overview & Architecture

The MitM Admin Frontend acts as the control panel for the Go-based Scheduler engine. It communicates via secure **HTTP/HTTPS REST APIs** and highly efficient **FlatBuffers** binary endpoints exposed by the Scheduler component.

### Security and Role-Based Access Control (RBAC)

The frontend dynamically adapts its features based on your authorized roles:

- **Authentication**: Uses transparent OS-level authentication or biometric unlock (Windows Hello).
- **ADMIN**: Full read/write access, including job execution/stopping, user management, and credentials.
- **VIEWER**: Read-only access to most tables. Destructive or modifying actions (Add/Edit/Delete, Save) are disabled.
- **UPLOADER**: Access to the Manual Upload Widget.
- **BACKUP-RESTORE**: Access to the Backup/Restore tab.

---

## 2. Navigating the Interface

### 📊 Dashboard

The **Dashboard** provides a high-level, real-time overview of the system's health and statistics.

- **System Health**: Indicates whether the backend Scheduler is online and connected to the PostgreSQL database.
- **Summary Cards**: Displays total counts for Scheduled Jobs, System Logs, Admin Audit Logs, Job Audit Logs, and Transformation Errors, along with the oldest entry timestamp.
- _Usage:_ Click the **Refresh Dashboard** button to poll the latest live data.

### ⏱️ Scheduler

The **Scheduler** tab orchestrates all automated ingestion and delivery jobs.

- **Table View:** See configured jobs, execution command, cron schedules, next run time (in your local timezone), and real-time **Active State** (e.g., `Running ⚙️` or `Idle`).
- **+ Add / Edit Job:** Manage jobs using an intuitive Cron Editor with predefined intervals (e.g., `*/6`, `*/8`, `*/12` hours).
- **▶ Execute Selected / ⏹ Stop Selected:** Manually trigger or forcefully terminate running jobs (requires `ADMIN` role).
- **Delete Selected:** Removes a job entirely from the pipeline.

### 📤 Manual Upload

A dedicated tab allowing users with the `UPLOADER` or `ADMIN` role to manually inject `.csv` and `.xlsx` files into the pipeline. The files are securely transmitted to the backend and immediately trigger the collector worker.
The first line of the uploaded file is used as the column names (headers) for the uploaded data, and the following lines are used as the data rows.

### 📜 Logs & Auditing

Extensive tracing capabilities utilizing efficient FlatBuffers APIs. All timestamps are automatically converted to your system's local timezone.

- **System Logs:** Diagnostic view into the Scheduler's internal operations and component logs.
- **Admin Logs:** Records all UI-driven configuration changes, user management actions, and frontend logins.
- **Job Audits:** Tracks execution events mapped to a specific `Run ID`.
  - **Export Excel Report:** Generates a comprehensive `.xlsx` report including a Pie Chart and dynamically calculated upload statistics (Records Added, Updated, Skipped, etc.) grouped in the `Upload-Report` and `Batch-Uploads` sheets. The export dialog remembers your last used directory.

### 🧩 Transformation Layer

Manage the data mapping pipeline components:

- **Sources & Targets**: Define source files/APIs and map them to target SaaS fields.
- **Rules & Transformations**: Define parsing and replacement logic. Features a **Live Preview** to test Regex and Date parsing locally without modifying backend state.
- **✨ Auto-Map (Smart Suggest):** Automatically generates mapping rules from comma-separated headers using fuzzy string matching.
- **Topic Dependencies**: Manage Stateful Aggregation requirements and execution order.

### ⚠️ Transformation Errors & DLQ

- **Transformation Errors:** Inspect payloads that failed validation or parsing during the transformation phase.
- **DLQ (Dead Letter Queue):** Manages payloads that failed to deliver to the target SaaS platform. Administrators can inspect the truncated `Error Message` and click **"Requeue Selected"** to attempt delivery again via the API.

### 👥 RBAC Management

Manage system users and their roles directly from the frontend (requires `ADMIN` role). Features dynamic "Add User", "Remove User", and role assignment capabilities communicating with the Go backend API.

### 💾 Backup & Restore

Allows users with `BACKUP-RESTORE` or `ADMIN` roles to seamlessly export and import the complete system configuration (jobs, sources, targets, rules) as JSON. Backups are automatically saved to your local `<Binary-Folder>/data/backup/` directory.

### ⚙️ Settings & Configuration

- **Configuration Profiles**: Switch between different encrypted environment configurations (`*.enc`). The active profile is displayed in the status bar.
- **Network Proxy**: Configure HTTP/HTTPS proxies securely. Proxies are stored using libsodium (AES-GCM) encryption in the `configs/` directory.

---

## 3. Keyboard Shortcuts and Tips

- **Sorting Data:** You can click the column headers in any of the tables to easily sort data alphanumerically.
- **Local Timezones:** Timestamps in all log tables are automatically converted from raw backend UTC to your system's local time.
- **Window Resizing:** The UI is completely responsive.
- **Selecting Items:** In tables, you must click a specific row to highlight it before interacting with context buttons like "Edit", "Delete", or "Requeue".

## 4. Troubleshooting

- **API Errors / Red Status:** If the Dashboard shows a red `Offline` or `Auth Error` status, verify that the Go Scheduler backend is running, the correct configuration profile is selected, and your OS user has been granted access via RBAC.
- **Exporting Errors:** Ensure you have write permissions to the directory when exporting CSV or Excel reports.
