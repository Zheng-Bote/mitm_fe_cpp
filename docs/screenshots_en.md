# MitM Data Aggregator - Screenshots

This page shows the main views of the MitM Data Aggregator Admin Frontend.

## Dashboard and Information

### Dashboard

The dashboard provides a current overview of the system state, scheduler version,
database, scheduled jobs as well as the number of available logs and errors.

![Dashboard with system status and metrics](screenshots/dashboard.png)

### Network Proxy

Via the network settings, an HTTP/HTTPS proxy including port and
credentials can be activated and configured.

![Dialog for configuring a network proxy](screenshots/network-proxy.png)

### Environments

Via the environment overview, it is possible to switch between the different environments.

![Overview of environments](screenshots/switch_env.png)

### Info and Update

The info dialog shows the product name, version, license, and copyright. If an update
is available, a corresponding notification is displayed.

![Info dialog with available update](screenshots/info_update-available.png)

Alternatively, the dialog confirms that the current version is already in use.

![Info dialog with current version](screenshots/info_update-ok.png)

### Integrated Help

The User Guide explains the features of the desktop application.

![Integrated user guide](screenshots/user-guide.png)

## Scheduler and Manual Upload

### Scheduler Overview

The scheduler view lists jobs with their command, cron expression, activation status,
next execution time, and current run status.

![Scheduler overview with configured jobs](screenshots/scheduler.png)

The updated overview also allows running or stopping selected jobs directly.

### Edit Job

In the edit dialog, the name, command, JSON arguments, activation, and the
cron schedule of a job are managed.

![Dialog for editing a scheduler job](screenshots/scheduler_job.png)

### Manual File Upload

Users with the `UPLOADER` role can select CSV or XLSX files for a topic
and immediately start the corresponding collector.

![Form for manual file upload](screenshots/manual-upload.png)

## Logs and Delivery

### Job Logs

The job logs document executions by time period, run ID, and component.
They can be exported as a CSV file or report.

![Job logs with date filter and export](screenshots/audit_job-logs.png)

### Report Export

The report export narrows down the delivery processes to be evaluated by job, topic, as well as
start and end date.

![Dialog for configuring a report export](screenshots/report-export.png)

### Successful Delivery

The view shows a successful delivery to the target endpoint with HTTP status
`200 (OK)` and successful import processing.

![Successful delivery with HTTP status 200](screenshots/delivery_ok.png)

### Failed Delivery

This recording documents the analysis of response status and token data in the case of a
failed delivery or one to be investigated.

![Analysis of a problematic delivery](screenshots/delivery_error.png)

### Unencrypted Transmission

The recording captures a case where a transmission without encryption
is being viewed.

![Example of an unencrypted transmission](<screenshots/2026-08-13 104438_ no encryption.png>)

## Transformations and Target Structure

### Target Fields

The list of target fields contains the topic, field name, data type, as well as indicators for
mandatory fields and encryption.

![Overview of target fields](screenshots/transformation-layer.png)

### Edit Target Field

In the edit dialog, the topic, field name, data type, mandatory status, and
encryption of a target field can be defined.

![Dialog for editing a target field](<screenshots/Screenshot 2026-09-10 142346.png>)

### Mapping Rules

The rule overview maps source fields to target fields and shows the priority as well as
transformation and validation chains.

![Overview of mapping rules](screenshots/rules.png)

### Configure Transformations

The rule editor offers a builder for transformation chains, for example for
trimming whitespace, changing case, or converting types.

![Selection of available transformations](screenshots/rules_transformation.png)

### Configure Validations

In the same editor, validations such as mandatory field check, email check,
pattern matching, as well as length and value range checks can be added.

![Selection of available validations](screenshots/transformation_validation.png)

## Accesses and Permissions

### Source Accesses

The management of source accesses shows configured connectors, topics, and their
activation status.

![Overview of source accesses](screenshots/sources.png)

### Target Accesses

For target systems, the adapter type, endpoint, and the corresponding configuration payload
can be maintained per topic.

![Dialog for editing a target access](screenshots/targets.png)

### Role Management

The RBAC management assigns users roles such as `ADMIN`, `VIEWER`, `UPLOADER`, and
`BACKUP-RESTORE`.

![RBAC management with user and role assignment](screenshots/rbac.png)

## Key Management and Configuration Backup

### Key Vault

The key vault view shows the lock status and allows unlocking with
master key or Windows Hello, as well as rotating the master key.

The empty input mask shows the intended steps for unlocking and generating a
new master key.

![Key vault with empty input fields](screenshots/settings_master-key.png)

### Backup and Restore

In the backup/restore section, configurations can be backed up and restored.

![Backup and restore view](screenshots/backup-restore.png)

After a successful backup, a dialog confirms the storage location of the generated
configuration file.
