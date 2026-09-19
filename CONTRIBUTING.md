# Contributing

Bug reports and pull requests are welcome. Please open an issue first for anything larger than a fix, so we can agree on the shape before you write it.

## Setting up

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build
ctest --test-dir build --output-on-failure
```

Try changes in a window of their own before putting them on your panel:

```bash
cmake --install build && plasmawindowed com.github.yakupemreyerli.traygrid
```

## Before you send a pull request

- `ctest` passes. New behaviour that talks to tray apps gets a case in `tests/gridtest.cpp`, driven through `tools/test-item.cpp`.
- Every source file keeps its SPDX header (`GPL-2.0-or-later`).
- Commit messages say what changed and why, in the imperative.

## Things that are the way they are on purpose

- **The plugin lives inside the package** (`contents/lib`) and `main.qml` imports it by directory. A system-wide QML module would need root or a `QML_IMPORT_PATH` in the session environment; this way a per-user install just works. The log's "does not contain a module identifier directive" warning is the price, and it's harmless.
- **The dbusmenu client is our own** (`src/dbusmenuclient.cpp`). `libdbusmenu-qt6` isn't packaged on every distribution, and Plasma's own importer is private API.
- **We don't use `org.kde.plasma.private.systemtray`.** Its model isn't constructible from QML and it can change with any Plasma release.
- **Menus are native `QMenu`s**, like Plasma's tray and `PlasmaExtras.Menu`. On Wayland they're anchored with `_q_waylandPopupAnchorRect`; keep that when touching the popup code.
- **Every call into an app first hands it an xdg-activation token** (`ProvideXdgActivationToken`), otherwise Wayland won't let the app raise its window.
- **libayatana items are keyed by title, not Id** (`configId`): their Id is random per launch.
- **The tests never touch your real tray.** `gridtest` refuses to run if a real `StatusNotifierWatcher` is on the bus; ctest wraps it in `dbus-run-session`.
- **Plasma loads the library once per session**, so after reinstalling you have to restart plasmashell to see C++ changes (QML changes show up in a fresh `plasmawindowed`).
- **Brand assets are generated.** Edit the palette in `tools/brand.py`, never the files under `docs/brand/`.
