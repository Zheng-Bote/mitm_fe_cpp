# TODOS

- [x] not all logs are shown
- [x] not all events are logged

- [x] menu window not modal on-top

- [ ] delivery-layer needs more steering options (topic-based?)


  ### 1. Ausbau des "Smart Suggest" (Auto-Map) 🪄

  Wir haben den Button  ✨ Auto-Map  im Frontend bereits als Platzhalter angelegt. Wir könnten
  im Go-Backend eine Logik implementieren, die alle Quellfelder mit den Zielfeldern abgleicht
  (z. B. über Fuzzy-String-Matching / Levenshtein-Distanz). Das System würde dann
  vollautomatisch
  passende Regeln generieren (z. B.  first_name  ->  FirstName ). Das spart bei großen
  Datenbanken enorm viel Zeit bei der initialen Konfiguration.

  ### 2. DLQ (Dead Letter Queue) Management & Replay 🚑

  Wenn das Senden von Paketen an die externe SaaS-Plattform fehlschlägt, landen diese in der
  DLQ. Wir könnten im Frontend-Tab "🚑 DLQ & Cursors" Features einbauen wie:

  • One-Click Replay: Einen Button, um fehlgeschlagene Pakete nach Behebung des Fehlers (z. B.
  API war offline) massenhaft erneut in die Queue zu schieben.
  • Payload Editor: Die Möglichkeit, das fehlerhafte JSON-Paket direkt im Frontend zu
  korrigieren und dann erst erneut zu versenden.

  ### 3. Sicherheit: Authentication Upgrade & RBAC 🔐

  Aktuell läuft die Kommunikation zwischen Frontend und Go-Backend über statisches  Basic Auth
  . Wenn die Plattform wächst, wäre ein Umstieg auf JWT (JSON Web Tokens) oder gar eine
  Anbindung an ein zentrales Identity Management (OIDC/OAuth2) sinnvoll. So könnten wir "Role-
  Based Access Control" (RBAC) einführen – sodass z. B. ein "Viewer" zwar Logs lesen, aber
  keine Mappings löschen darf.

  ### 4. Automatisiertes Testing der Transformationen 🧪

  Das Herzstück des Aggregators ist der Transformation-Layer. Wir haben zwar jetzt eine schöne
  UI dafür, aber um Regressionen zu vermeiden, könnten wir im Backend eine solide Test-Suite
  aufbauen, die alle möglichen Ketten ( uppercase  ->  trim  ->  parse_date ) mit Randfällen (
  null , leere Strings, falsche Datumsformate) vollautomatisch durchprüft.

  ### 5. Keychain-Integration fürs Frontend 🔑

  Beim Start des Frontends muss man aktuell jedes Mal das Passwort eintippen, um  config.enc
  zu entschlüsseln. Wir könnten das Frontend an den System-Schlüsselbund (z. B. KWallet/Secret
  Service unter Linux, Credential Manager unter Windows) anbinden. Das Passwort wird dann
  sicher vom Betriebssystem verwaltet und man spart sich die manuelle Eingabe beim Start.
  
  ──────