# TODOS

### 1. Sicherheit & Authentifizierung

• MFA (Multi-Faktor-Authentifizierung): Da der Scheduler sensible PII-Daten und Zugänge
verwaltet, wäre die Integration von TOTP (z.B. Google/Microsoft Authenticator) für Admin-Konten eine
starke Aufwertung.

### 2. Keychain-Integration fürs Frontend 🔑

Beim Start des Frontends muss man aktuell jedes Mal das Passwort eintippen, um config.enc
zu entschlüsseln. Wir könnten das Frontend an den System-Schlüsselbund (z. B. KWallet/Secret
Service unter Linux, Credential Manager unter Windows) anbinden. Das Passwort wird dann
sicher vom Betriebssystem verwaltet und man spart sich die manuelle Eingabe beim Start.

### 2. Betrieb & Resilienz (Operations)

• Hot-Reloading der Konfiguration: Aktuell erfordert eine Änderung an der config.json
einen Neustart des Daemons. Durch das Abfangen eines SIGHUP -Signals oder den Einsatz von
File-Watchern (wie fsnotify ) könnte der Scheduler neue Konfigurationen on-the-fly einlesen.
• Proaktives Alerting: Bislang landen fehlerhafte Jobs in der Dead Letter Queue (DLQ) oder
im Audit-Log. Der Scheduler könnte um ein Notification-Modul erweitert werden (z. B. E-Mail,
Slack/Teams Webhooks), das Alarm schlägt, wenn ein Job x-mal hintereinander fehlschlägt oder
die DLQ-Einträge einen kritischen Schwellenwert überschreiten.

### 3. Monitoring & Observability

• Prometheus Metrics Exporter: Die Vorgaben verlangen bereits Monitoring-Endpunkte (
/metrics ). Falls noch nicht vollständig umgesetzt, sollten hier detaillierte Go-Metriken
(Routinen, Memory) sowie Business-Metriken (Anzahl gestarteter Jobs, Success/Failure Rates,
DLQ-Größe) exponiert werden.
• Verteilte Tracing-IDs: Da der Scheduler Collector-, Transformation- und Delivery-Layer
orchestriert, könnte jedem Lauf (Run) eine eindeutige Correlation-ID / Trace-ID mitgegeben
werden, die an alle aufgerufenen Binaries/Systeme durchgereicht wird. So lassen sich Fehler
über Log-Grenzen hinweg perfekt nachvollziehen.

### 4. Scheduler Features

• Abhängigkeiten zwischen Jobs (DAGs): Momentan scheinen Jobs per Cron-Expression gesteuert
zu werden. Oft müssen Jobs aber sequenziell laufen (z. B. Transformation darf erst starten,
wenn der Collector fertig ist). Eine Erweiterung um Abhängigkeiten (Job B startet on_success
von Job A) würde den Scheduler in Richtung eines leichtgewichtigen Apache Airflow rücken.
• Job Timeouts & Kill Switches: Ein Feature, das Jobs automatisch via Context-Cancellation
beendet, wenn sie ein definiertes Zeitlimit (z. B. max_duration_seconds ) überschreiten, um
hängende Prozesse zu vermeiden.

---

Frage, keine Umsetzung: wie könnte eine Implementierung aussehen, wenn zu einem
Topic/Datensatz mehrere Daten-Quellen benötigt werden, die zeitversetzt Daten leiferNß

▸ Das ist ein sehr guter Punkt. Wenn wir das Konzept an das bestehende Datenmodell (
001_raw_ingestions.sql ) und den Oracle-Collector ( mitm_collector_ora ) anpassen, verändert
sich der Ablauf leicht.

Anstatt einen einzelnen Datenbank-Eintrag immer wieder zu aktualisieren (Upsert), nutzt das
bestehende Design ein Append-Only Event-Log Pattern (Landing Zone). Jeder Collector schreibt
sein eigenes verschlüsseltes Fragment als neue Zeile in die Tabelle raw_ingestion .

Hier ist das konkrete Szenario, wie die Implementierung ohne Code-Änderungen an der
Datenbankstruktur aussehen könnte:

### 1. Daten-Ingestion durch den Oracle-Collector ( mitm_collector_ora )

Der mitm_collector_ora liest beispielsweise Abteilungs- und Rollen-Zuweisungen aus einem
Oracle-HR-System.
• Er liest Datensatz A (z. B. Mitarbeiter "Max Mustermann").
• Er ermittelt oder generiert eine einheitliche correlation_id (z. B. einen UUIDv5-Hash
aus der Mitarbeiter-ID).
• Er verschlüsselt den Payload (Envelope Encryption mit DEK) und führt ein INSERT in
raw_ingestion aus:
• topic = employee.onboarding
• source_system = ORACLE_HR
• correlation_id = [UUID von Max]
• payload = [Verschlüsselte Oracle-Daten]
• status = pending

### 2. Daten-Ingestion durch einen zweiten Collector (z.B. Active Directory)

Ein anderer Collector (oder ein Webhook) liefert Stunden später die Netzwerk-Informationen
für denselben Mitarbeiter.

• Er führt ebenfalls ein INSERT in raw_ingestion aus:
• topic = employee.onboarding
• source_system = ACTIVE_DIRECTORY
• correlation_id = [UUID von Max]
• payload = [Verschlüsselte AD-Daten]
• status = pending

Wir haben nun zwei separate Zeilen in der raw_ingestion Tabelle mit derselben
correlation_id , beide im Status pending .

### 3. Der Orchestrator / Scheduler (Aggregation Logik)

Da der Index idx_raw_pending_topics existiert, kann der Scheduler extrem performant
pollen. Der Orchestrator geht nun nicht mehr auf Zeilen-Ebene, sondern gruppiert nach der
correlation_id .

Die Abfrage-Logik (Pseudocode):

    SELECT correlation_id, array_agg(source_system) as sources
    FROM raw_ingestion
    WHERE topic = 'employee.onboarding' AND status = 'pending'
    GROUP BY correlation_id;

Der Check:
Der Orchestrator weiß durch eine Konfiguration, dass employee.onboarding zwingend
ORACLE_HR und ACTIVE_DIRECTORY benötigt.

• Er iteriert über die gruppierten Ergebnisse.
• Mitarbeiter Max ( sources: ['ORACLE_HR', 'ACTIVE_DIRECTORY'] ): Beide Quellen sind da! Der
Orchestrator holt beide payloads , entschlüsselt sie via DEK/KEK, verschmilzt die JSON-
Objekte zu einem großen "Golden Record" und reicht diesen an den Transformation-Layer weiter.
Beide Zeilen in der DB werden danach auf status = 'completed' gesetzt.
• Mitarbeiter Anna ( sources: ['ORACLE_HR'] ): Hier fehlt das AD. Der Orchestrator macht
nichts. Die Zeile bleibt auf pending .

### 4. Timeout-Behandlung (Garbage Collection)

Um zu verhindern, dass Annas Datensatz für immer im Status pending bleibt, führt der
Scheduler regelmäßig einen Clean-up Job aus:
Er sucht nach Zeilen mit status = 'pending' , deren created_at älter als z. B. 48 Stunden
ist.
Diese Zeilen werden auf status = 'failed' gesetzt. Der fehlerhafte Zustand kann dann
ausgelesen und als Alert (oder Dead-Letter) an das Monitoring gemeldet werden.

Zusammenfassend:
Durch die Struktur in 001_raw_ingestions.sql wird die Aggregation in den Lese-Prozess
(Orchestrator) verlagert, statt sie beim Schreiben durch die Collectors zu erzwingen. Der
mitm_collector_ora muss nichts über andere Systeme wissen – er wirft sein Puzzleteil
einfach verschlüsselt in die Tabelle. Der Orchestrator setzt die Teile anhand der
correlation_id zusammen, sobald alle benötigten source_systems für ein Topic den Status
pending aufweisen.
