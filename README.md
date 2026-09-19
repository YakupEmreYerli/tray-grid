# <img src="docs/brand/logo.svg" width="40" height="40" align="top" alt=""> Tray Grid

[![CI](https://github.com/YakupEmreYerli/tray-grid/actions/workflows/ci.yml/badge.svg)](https://github.com/YakupEmreYerli/tray-grid/actions/workflows/ci.yml) [![License: GPL-2.0-or-later](https://img.shields.io/badge/license-GPL--2.0--or--later-05182F)](LICENSE) [![KDE Plasma 6](https://img.shields.io/badge/KDE%20Plasma-6-ADD5FF?logo=kde&logoColor=white)](https://kde.org/plasma-desktop/)

Plasma's tray tucks background apps behind an arrow, then shows them as a long list with a label on every row. Tray Grid is a panel widget that shows them the way Windows does: a small arrow, and a compact grid of icons that opens above it.

> Türkçe: [README.tr.md](README.tr.md)

![Tray Grid open above the panel, showing eight tray apps as an icon grid](docs/screenshots/popup.png)

It talks to tray apps directly over D-Bus, the same StatusNotifierItem protocol Plasma's own tray uses, so every app that shows up in the system tray shows up here: Electron apps, Qt and GTK apps, libappindicator apps.

## Features

- **Icons only.** A grid of 1–8 columns, no labels; the app's own tooltip on hover.
- **Clicks do what the tray does.** Left click activates the app (or opens its menu if it has nothing to activate), middle click is the secondary action, the wheel scrolls, right click opens the app's own menu with submenus, checkboxes and radio items.
- **Live.** Apps that start, quit, or change their icon update the grid immediately.
- **Your pick.** A settings page lists the running tray apps with a checkbox each; untick the ones you'd rather keep out of the grid. Column count, icon size and whether idle ("passive") apps are shown are configurable too.
- **Native.** Follows your Plasma colour scheme and icon theme; the arrow points away from whichever screen edge the panel is on.

## Install

Tray Grid is built from source and installs per user, no root needed. It needs Plasma 6, KDE Frameworks 6.10+ and Qt 6.8+.

```bash
# Arch: sudo pacman -S --needed cmake ninja extra-cmake-modules qt6-declarative kwindowsystem kiconthemes libplasma
git clone https://github.com/YakupEmreYerli/tray-grid.git && cd tray-grid
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build && ctest --test-dir build
cmake --install build
```

Then right-click the panel → **Add or Manage Widgets** → **Tray Grid**, and drag it next to the system tray. To try it without touching your panel: `plasmawindowed com.github.yakupemreyerli.traygrid`.

After updating, restart Plasma once (`systemctl --user restart plasma-plasmashell`): the widget's library is loaded a single time per session.

To uninstall: `xargs rm < build/install_manifest.txt`.

## Living next to Plasma's tray

Plasma's system tray keeps listing the same apps, and its own arrow appears whenever anything is in its hidden section. To end up with a single arrow, open the system tray's settings → **Entries** and:

1. Set every application entry to **Disabled**. Plasma then shows them nowhere; Tray Grid still does.
2. Set every remaining entry to either **Always shown** or **Disabled**. Anything on "Shown when relevant" can slip into the hidden section and bring Plasma's arrow back.
3. Leave **Always show all entries** off; it overrides "Disabled".

A newly installed app appears in Plasma's tray too the first time it runs, until you disable it there once.

## Known limits

- Apps built on libayatana (LocalSend, for example) pick a new random tray ID on every launch. Tray Grid remembers them by title, but Plasma's "Disabled" setting can't stick to them.
- Overlay icons sent as raw pixmaps and animated attention icons are not drawn yet; named overlay icons are.
- An app that ships a single white tray icon (Docker Desktop) is hard to see on a light colour scheme, here as in Plasma's own tray.

## How it works

| Part | What it does |
| --- | --- |
| `src/statusnotifierhost.*` | Registers as a StatusNotifierHost with `org.kde.StatusNotifierWatcher` and tracks the registered items |
| `src/statusnotifieritem.*`, `src/sniimage.*` | One tray app: properties, icon by name, theme path or ARGB pixmap, change signals |
| `src/dbusmenuclient.cpp` | A from-scratch `com.canonical.dbusmenu` client (libdbusmenu-qt6 isn't packaged everywhere), shown as a native menu |
| `src/statusnotifiermodel.*` | The list model the QML grid binds to |
| `package/contents/ui/` | The panel arrow, the grid popup and the settings page |

The C++ plugin ships inside the widget package (`contents/lib`) and is loaded with a directory import, so installing never touches `QML_IMPORT_PATH` or your session environment.

## Development

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build
ctest --test-dir build --output-on-failure   # end-to-end, on a private D-Bus session
build/bin/sni-dump --menus                   # what your live tray apps expose, read-only
```

The end-to-end test starts its own D-Bus session with a mock watcher and a scriptable test app, so it never touches your real tray. It checks that the package loads, items appear and update, clicks and the wheel reach the app, menus build and their entries reach the app, and items vanish when their app exits.

The README screenshot is staged on a private D-Bus session with well-known apps by `tools/readme-shot.sh`, so nobody's own tray ends up in it. Brand assets are rendered from `tools/brand.py`; see the [brand kit](docs/brand/README.md). Contribution rules: [CONTRIBUTING.md](CONTRIBUTING.md).

## License

GPL-2.0-or-later, like most of KDE. See [LICENSE](LICENSE).
