# Tray Grid — notes for coding agents

A KDE Plasma 6 panel widget: an arrow that opens a compact, label-free icon grid of StatusNotifierItem (tray) apps.
User-facing docs are in `README.md`; the "why it is like this" list is in `CONTRIBUTING.md`. Read both before changing code.

## Commands

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build
ctest --test-dir build --output-on-failure    # end-to-end, private D-Bus session
cmake --install build && plasmawindowed com.github.yakupemreyerli.traygrid
build/bin/sni-dump --menus                    # read-only view of the live tray
tools/readme-shot.sh docs/screenshots/popup.png   # README screenshot, staged apps only
python3 tools/brand.py                        # logo and social preview from the palette
```

## Layout

- `src/` — C++ QML plugin: `statusnotifierhost` (registers with `org.kde.StatusNotifierWatcher`), `statusnotifieritem`
  (one app: properties, signals, icons via `sniimage`), `dbusmenuclient` (own `com.canonical.dbusmenu` client, shown as
  `QMenu`), `statusnotifiermodel` (list model for QML), `popuphelper` (menu placement, xdg-activation tokens).
- `package/` — the plasmoid: `CompactRepresentation.qml` (arrow), `FullRepresentation.qml` (grid), `TrayIconDelegate.qml`,
  `ConfigGeneral.qml`. The plugin is copied into `contents/lib` at install and imported by directory.
- `tests/gridtest.cpp` + `tests/mockwatcher.*` + `tools/test-item.cpp` — the end-to-end test and its fake tray app.

## Rules

- Everything stays per-user: no root, no `QML_IMPORT_PATH`, no session environment changes.
- Never touch the user's live panel or restart plasmashell while working; test in `plasmawindowed` or with `ctest`.
- `gridtest` must keep refusing to run on a bus that already has a real `StatusNotifierWatcher`.
- Public images (README, store, social) never show a real desktop: stage them with `tools/readme-shot.sh` and `tools/brand.py`.
- No new dependencies beyond Qt 6, KDE Frameworks 6, libplasma and ECM without discussing it in an issue first.
- Keep SPDX headers (`GPL-2.0-or-later`). Tests green before every commit. Commit messages describe what and why.

## Behaviour worth knowing

- By default the grid shows every registered item; users hide apps per item in the settings page (`excludedIds`).
- libayatana items get a random Id per launch, so settings key them by Title (`configId`).
- Left click falls back to the menu when `Activate` fails (libappindicator apps have none), as Plasma's tray does.
- After reinstalling, plasmashell keeps the old library until it restarts; `plasmawindowed` always loads fresh.
- Not implemented yet: `OverlayIconPixmap`, `AttentionMovie`.
