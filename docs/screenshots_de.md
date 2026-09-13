# MitM Data Aggregator - Screenshots

Diese Seite zeigt die wichtigsten Ansichten des MitM Data Aggregator Admin Frontends.

## Dashboard und Informationen

### Dashboard

Das Dashboard bietet einen aktuellen Überblick über Systemzustand, Scheduler-Version,
Datenbank, geplante Jobs sowie die Anzahl der vorhandenen Protokolle und Fehler.

![Dashboard mit Systemstatus und Kennzahlen](screenshots/dashboard.png)

### Netzwerk-Proxy

Über die Netzwerkeinstellungen kann ein HTTP/HTTPS-Proxy einschließlich Port und
Anmeldedaten aktiviert und konfiguriert werden.

![Dialog zur Konfiguration eines Netzwerk-Proxys](screenshots/network-proxy.png)

### Umgebungen

Über die Umgebungsübersicht kann zwischen den verschienen Umgebungen gewechselt werden.

![Übersicht der Umgebungen](screenshots/switch_env.png)

### Info und Aktualisierung

Der Info-Dialog zeigt Produktname, Version, Lizenz und Copyright. Bei einer verfügbaren
Aktualisierung wird ein entsprechender Hinweis angezeigt.

![Info-Dialog mit verfügbarem Update](screenshots/info_update-available.png)

Der Dialog bestätigt alternativ, dass bereits die aktuelle Version verwendet wird.

![Info-Dialog mit aktueller Version](screenshots/info_update-ok.png)

### Integrierte Hilfe

Der User Guide erläutert die Funktionen der Desktop Applikation.

![Integrierter Benutzerleitfaden](screenshots/user-guide.png)

## Scheduler und manueller Upload

### Scheduler-Übersicht

Die Scheduler-Ansicht listet Jobs mit Befehl, Cron-Ausdruck, Aktivierungsstatus,
nächstem Ausführungszeitpunkt und aktuellem Laufstatus auf.

![Scheduler-Übersicht mit konfigurierten Jobs](screenshots/scheduler.png)

Die aktualisierte Übersicht erlaubt außerdem, ausgewählte Jobs direkt auszuführen oder
anzuhalten.

### Job bearbeiten

Im Bearbeitungsdialog werden Name, Befehl, JSON-Argumente, Aktivierung und der
Cron-Zeitplan eines Jobs verwaltet.

![Dialog zum Bearbeiten eines Scheduler-Jobs](screenshots/scheduler_job.png)

### Manueller Datei-Upload

Benutzer mit der Rolle `UPLOADER` können CSV- oder XLSX-Dateien für ein Topic auswählen
und den zugehörigen Collector unmittelbar starten.

![Formular für den manuellen Datei-Upload](screenshots/manual-upload.png)

## Protokolle und Zustellung

### Job-Protokolle

Die Job-Protokolle dokumentieren Ausführungen nach Zeitraum, Run-ID und Komponente.
Sie können als CSV-Datei oder Bericht exportiert werden.

![Job-Protokolle mit Datumsfilter und Export](screenshots/audit_job-logs.png)

### Berichts-Export

Der Berichts-Export grenzt die auszuwertenden Zustellvorgänge über Job, Topic sowie
Start- und Enddatum ein.

![Dialog zur Konfiguration eines Berichts-Exports](screenshots/report-export.png)

### Erfolgreiche Zustellung

Die Ansicht zeigt eine erfolgreiche Zustellung an den Zielendpunkt mit HTTP-Status
`200 (OK)` und einer erfolgreichen Importverarbeitung.

![Erfolgreiche Zustellung mit HTTP-Status 200](screenshots/delivery_ok.png)

### Fehlerhafte Zustellung

Diese Aufnahme dokumentiert die Analyse von Antwortstatus und Token-Daten bei einer
fehlerhaften oder zu untersuchenden Zustellung.

![Analyse einer problematischen Zustellung](screenshots/delivery_error.png)

### Unverschlüsselte Übertragung

Die Aufnahme hält einen Fall fest, in dem eine Übertragung ohne Verschlüsselung
betrachtet wird.

![Beispiel einer nicht verschlüsselten Übertragung](<screenshots/2026-08-13 104438_ no encryption.png>)

## Transformationen und Zielstruktur

### Ziel-Felder

Die Liste der Ziel-Felder enthält Topic, Feldname, Datentyp sowie Kennzeichnungen für
Pflichtfelder und Verschlüsselung.

![Übersicht der Ziel-Felder](screenshots/transformation-layer.png)

### Ziel-Feld bearbeiten

Im Bearbeitungsdialog lassen sich Topic, Feldname, Datentyp, Pflichtstatus und
Verschlüsselung eines Ziel-Feldes festlegen.

![Dialog zum Bearbeiten eines Ziel-Feldes](<screenshots/Screenshot 2026-09-10 142346.png>)

### Zuordnungsregeln

Die Regelübersicht ordnet Quellfelder Ziel-Feldern zu und zeigt Priorität sowie
Transformations- und Validierungsketten.

![Übersicht der Zuordnungsregeln](screenshots/rules.png)

### Transformationen konfigurieren

Der Regel-Editor bietet einen Builder für Transformationsketten, beispielsweise zum
Bereinigen von Leerzeichen, Ändern der Groß-/Kleinschreibung oder Umwandeln von Typen.

![Auswahl verfügbarer Transformationen](screenshots/rules_transformation.png)

### Validierungen konfigurieren

Im selben Editor können Validierungen wie Pflichtfeldprüfung, E-Mail-Prüfung,
Mustervergleich sowie Längen- und Wertebereichsprüfungen ergänzt werden.

![Auswahl verfügbarer Validierungen](screenshots/transformation_validation.png)

## Zugänge und Berechtigungen

### Quellzugänge

Die Verwaltung der Quellzugänge zeigt konfigurierte Connectoren, Topics und ihren
Aktivierungsstatus.

![Übersicht der Quellzugänge](screenshots/sources.png)

### Zielzugänge

Für Zielsysteme können Adaptertyp, Endpunkt und das zugehörige Konfigurations-Payload
pro Topic gepflegt werden.

![Dialog zum Bearbeiten eines Zielzugangs](screenshots/targets.png)

### Rollenverwaltung

Die RBAC-Verwaltung ordnet Benutzern Rollen wie `ADMIN`, `VIEWER`, `UPLOADER` und
`BACKUP-RESTORE` zu.

![RBAC-Verwaltung mit Benutzer- und Rollenzuordnung](screenshots/rbac.png)

## Schlüsselverwaltung und Konfigurationssicherung

### Key Vault

Die Key-Vault-Ansicht zeigt den Sperrstatus und ermöglicht das Entsperren mit
Master-Key oder Windows Hello sowie die Rotation des Master-Keys.

Die leere Eingabemaske zeigt die vorgesehenen Schritte zum Entsperren und Erzeugen eines
neuen Master-Keys.

![Key Vault mit leeren Eingabefeldern](screenshots/settings_master-key.png)

### Backup und Restore

Im Bereich Backup/Restore können Konfigurationen gesichert und wiederhergestellt werden.

![Backup- und Restore-Ansicht](screenshots/backup-restore.png)

Nach einer erfolgreichen Sicherung bestätigt ein Dialog den Ablageort der erzeugten
Konfigurationsdatei.
