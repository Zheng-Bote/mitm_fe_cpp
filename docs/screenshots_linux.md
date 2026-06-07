# MitM Admin Frontend - Linux Screenshots

This document showcases the graphical user interface of the **MitM Data Aggregator Admin Frontend** when running on a Linux desktop environment.

> [!WARNING]\
> This page is in DRAFT status, it is still under construction

### 1. Application Startup

The initial prompt asking the user for the master password to decrypt the secure `config.enc` file via libsodium.
![Application Startup](screenshots/linux/01.png)

### 2. Dashboard View

The main dashboard displaying real-time system metrics, scheduler health, and an overview of active jobs.
![Dashboard View](screenshots/linux/02.png)

### 3. Scheduler Overview

The Scheduler tab listing all currently configured jobs, their cron schedules, and their last execution statuses.
![Scheduler Overview](screenshots/linux/03.png)

### 4. Job Editor & Cron

Editing an existing job, featuring the interactive Cron Expression editor for precise scheduling.
![Job Editor](screenshots/linux/04.png)

### 5. System Logs

The System Logs tab providing real-time log streaming from the backend with filtering capabilities.
![System Logs](screenshots/linux/05.png)

### 6. Audit Logs

Detailed audit logs tracking job execution histories and system changes.
![Audit Logs](screenshots/linux/06.png)

### 7. Rules & Mapping

The interface for defining and managing data transformation rules and field mappings.
![Rules and Mapping](screenshots/linux/07.png)

### 8. DLQ & Cursors

Monitoring the Dead Letter Queue (DLQ) for failed messages and managing system cursors.
![DLQ and Cursors](screenshots/linux/08.png)

### 9. Settings & Key Vault

Configuration interface for securely injecting the Envelope Encryption master keys and managing backend connections.
![Settings and Key Vault](screenshots/linux/09.png)

### 10. Network Proxy Setup

The per-user proxy configuration dialog accessible via the Settings menu, allowing secure HTTP/HTTPS proxy overrides.
![Network Proxy Setup](screenshots/linux/10.png)

### 11. About Dialog

The "About" dialog displaying the current version, copyright information, and automatically checking for updates on GitHub.
![About Dialog](screenshots/linux/11.png)

### 12. Update Notification

The About dialog notifying the user that a new release is available for download.
![Update Notification](screenshots/linux/12.png)

### 13. Application Overview

A comprehensive look at the MitM Admin Control Plane with multiple active widgets.
![Application Overview](screenshots/linux/13.png)
