#!/bin/bash
# Renders the README screenshot: the icon grid on a private D-Bus session, with well-known apps
# instead of your own tray, then readme-compose.py puts it above a Plasma-like panel. Usage: tools/readme-shot.sh OUTPUT.png [SCALE]
set -euo pipefail
cikti=$(realpath -m "$1"); olcek=${2:-2}
kok=$(cd "$(dirname "$0")/.." && pwd); bin="$kok/build/bin"; paket="$kok/build/stage/com.github.yakupemreyerli.traygrid"
ikon=$(mktemp -d); trap 'rm -rf "$ikon"' EXIT
papirus=${PAPIRUS:-$HOME/.local/share/icons/Papirus/64x64/apps}
uygulamalar=("steam:Steam" "discord:Discord" "spotify:Spotify" "docker-desktop:Docker Desktop"
             "Nextcloud:Nextcloud" "telegram:Telegram" "keepassxc:KeePassXC" "dropbox:Dropbox")
for u in "${uygulamalar[@]}"; do rsvg-convert -w 64 -h 64 "$papirus/${u%%:*}.svg" -o "$ikon/${u%%:*}.png"; done
export ikon bin paket cikti olcek
dbus-run-session -- bash -c '
  sira=0
  for u in steam:Steam discord:Discord spotify:Spotify "docker-desktop:Docker Desktop" Nextcloud:Nextcloud telegram:Telegram keepassxc:KeePassXC dropbox:Dropbox; do
    QT_QPA_PLATFORM=offscreen "$bin/traygrid-test-item" --id "demo-$sira" --title "${u#*:}" --icon-file "$ikon/${u%%:*}.png" --quit-after 20 >/dev/null &
    sira=$((sira+1)); sleep 0.15
  done
  QT_QPA_PLATFORMTHEME=kde QT_SCALE_FACTOR=$olcek "$bin/gridtest" --preview "$paket" "$ikon/grid.png"
'
python3 "$kok/tools/readme-compose.py" "$ikon/grid.png" "$cikti" "$olcek"
