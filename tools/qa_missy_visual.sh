#!/usr/bin/env bash
# Smoke-run the actual CS_PREVIEW=4 GBA executable in an Xvfb mGBA window.
# Two real framebuffer captures (opening and later beat); no mockup sprites.
set -euo pipefail
export QT_QPA_PLATFORM=xcb
export LIBGL_ALWAYS_SOFTWARE=1
export SDL_AUDIODRIVER=dummy
mgba-qt ./bore-missy-preview.gba > screenshots/emulator.log 2>&1 &
pid=$!
trap 'kill "$pid" 2>/dev/null || true' EXIT
sleep 14 # includes the game boot logo; film starts without visiting menus
kill -0 "$pid" || { cat screenshots/emulator.log; exit 1; }
win=$(xdotool search --name 'mGBA' | head -1 || true)
if [ -z "$win" ]; then
  cat screenshots/emulator.log
  echo "mGBA did not open a game window" >&2
  exit 1
fi
import -window "$win" screenshots/hospital-opening.png
xdotool windowfocus "$win" || true
# Exercise the normal A/B controls to reach the bedside examination.
# x and z are the standard default mGBA keyboard buttons.
for i in $(seq 1 16); do
  xdotool key --window "$win" x || true
  sleep 0.22
  xdotool key --window "$win" z || true
  sleep 0.22
done
sleep 2
import -window "$win" screenshots/hospital-bedside.png
for file in screenshots/*.png; do
  colors=$(identify -format '%k' "$file")
  width=$(identify -format '%w' "$file")
  height=$(identify -format '%h' "$file")
  echo "$file: ${width}x${height}, ${colors} unique colours"
  test "$width" -ge 240 && test "$height" -ge 160
  test "$colors" -ge 12 # catches a totally blank or crashed emulator screen
done
