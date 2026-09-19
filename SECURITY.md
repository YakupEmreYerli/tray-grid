# Security

Please report vulnerabilities privately to **yakupemreyerli0@gmail.com**, not in a public issue. You'll get an answer within a week.

## Scope and what's already in place

- Tray Grid only talks to the **session** D-Bus, as the logged-in user. It opens no network connections and stores nothing but its own widget settings.
- It only calls methods other apps publish for exactly this purpose (StatusNotifierItem `Activate`, `SecondaryActivate`, `Scroll`, and dbusmenu `Event`), and only when you click.
- Menu labels and tooltips from apps are shown as plain text.
- Any app on your session bus can already register a tray item; a misbehaving app showing a misleading icon or menu is outside what this widget can prevent.
