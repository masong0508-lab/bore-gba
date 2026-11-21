#!/usr/bin/env bash
# Boots the real, compiled hospital cutscene preview in a Linux X11 mGBA window.
# Pixel captures come from XWD -> PNG, not the ImageMagick 'import' command.
set -euo pipefail
mkdir -p screenshots
export QT_QPA_PLATFORM=xcb
export LIBGL_ALWAYS_SOFTWARE=1
export SDL_AUDIODRIVER=dummy
mgba-qt ./bore-missy-preview.gba > screenshots/emulator.log 2>&1 &
pid=$!
trap 'kill "$pid" 2>/dev/null || true' EXIT
sleep 14  # game plays its short studio logo before the cinematic preview
if ! kill -0 "$pid" 2>/dev/null; then
  cat screenshots/emulator.log
  echo 'mGBA terminated before the hospital preview' >&2
  exit 1
fi
# Search only visible windows; prefer the actual emulator over an offscreen UI.
win=$(xdotool search --onlyvisible --name 'mGBA' | head -n 1 || true)
if [ -z "$win" ]; then
  cat screenshots/emulator.log
  xwininfo -root -tree || true
  echo 'mGBA game window not found in Xvfb' >&2
  exit 1
fi
echo "mGBA window: $win"
xwininfo -id "$win" | head -n 26 || true
capture(){
  local name="$1"
  xwd -silent -id "$win" -out "screenshots/$name.xwd"
  magick "screenshots/$name.xwd" -strip "screenshots/$name.png"
  rm -f "screenshots/$name.xwd"
}
capture hospital-opening
xdotool windowfocus "$win" || true
# mGBA's default key bindings: X for A, Z for B. Alternate them so the
# typewriter and caption pagination don't swallow one giant held-down key.
for i in $(seq 1 16); do
  xdotool key --clearmodifiers --window "$win" x || true
  sleep 0.22
  xdotool key --clearmodifiers --window "$win" z || true
  sleep 0.22
done
sleep 2
capture hospital-bedside
for file in screenshots/*.png; do
  read -r width height colors < <(magick identify -format '%w %h %k\\n' "$file")
  echo "$file: ${width}x${height}, ${colors} unique colours"
  test "$width" -ge 240 && test "$height" -ge 160
  test "$colors" -ge 12
done
# Capturing twice the same frozen frame is not a valid visual smoke test.
if cmp -s screenshots/hospital-opening.png screenshots/hospital-bedside.png; then
  echo "No visual change across dialogue and A/B advances" >&2
  exit 1
fi
echo 'Missy hospital mGBA capture: PASS'
