#!/usr/bin/env bash
# Boots the real, compiled hospital cutscene preview in a Linux X11 mGBA window.
# Pixel captures come from XWD -> PNG, not the ImageMagick 'import' command.
set -euo pipefail
mkdir -p screenshots
rom=${1:-bore-missy-preview.gba}
label=${2:-hospital}
test -s "$rom"
export QT_QPA_PLATFORM=xcb
export LIBGL_ALWAYS_SOFTWARE=1
export SDL_AUDIODRIVER=dummy
mgba-qt "./$rom" > "screenshots/${label}-emulator.log" 2>&1 &
pid=$!
cleanup(){ kill "$pid" 2>/dev/null || true; wait "$pid" 2>/dev/null || true; }
trap cleanup EXIT
alive(){
  if ! kill -0 "$pid" 2>/dev/null; then
    cat "screenshots/${label}-emulator.log"
    echo 'mGBA terminated during the hospital preview' >&2
    exit 1
  fi
}
# Startup speed varies with runner load. Wait for this process's visible window,
# then for the actual cinematic below; elapsed wall time is not a ready signal.
deadline=$((SECONDS+60))
# Search only visible windows; prefer the actual emulator over an offscreen UI.
# Select only the window owned by this invocation. A shared CI desktop may
# contain other emulator windows; their animation must not satisfy this smoke.
win=''
while (( SECONDS < deadline )); do
  alive
  win=$(xdotool search --onlyvisible --pid "$pid" --name 'mGBA' | head -n 1 || true)
  if [ -n "$win" ]; then break; fi
  sleep 0.5
done
if [ -z "$win" ]; then
  cat "screenshots/${label}-emulator.log"
  xdotool search --onlyvisible --name mGBA || true
  echo 'mGBA game window not found in Xvfb' >&2
  exit 1
fi
echo "mGBA window: $win"
xdotool getwindowname "$win" || true
capture(){
  local name="$1"
  local src="screenshots/$name.xwd"
  xwd -silent -id "$win" -out "$src"
  # The Qt client window includes its menu row above the 3:2 GBA display.
  # Crop that chrome away so colour/change checks measure GAME pixels only.
  local win_w win_h game_h top
  read -r win_w win_h <<< "$(magick identify -format '%w %h' "$src")"
  if (( win_w < 240 || win_w % 3 != 0 )); then
    echo "Unexpected mGBA capture width: ${win_w}" >&2
    exit 1
  fi
  game_h=$((win_w*2/3))
  top=$((win_h-game_h))
  if (( top < 0 || game_h < 160 )); then
    echo "No full 240x160 GBA viewport in ${win_w}x${win_h} window" >&2
    exit 1
  fi
  magick "$src" -crop "${win_w}x${game_h}+0+${top}" +repage -strip "screenshots/$name.png"
  rm -f "$src"
}
cinematic_ready(){
  local file="$1" panel colors
  # csDraw's fixed gold divider (y=116) and blue caption margin (y=158)
  # distinguish the cinematic from a colourful boot logo or a black fade.
  # Sample at GBA resolution; allow 5-bit expansion/rounding differences.
  panel=$(magick "$file" -sample '240x160!' -format '%[fx:abs(u.p{2,116}.r-14/31)<0.04&&abs(u.p{2,116}.g-11/31)<0.04&&abs(u.p{2,116}.b-3/31)<0.04&&abs(u.p{2,158}.r-2/31)<0.04&&abs(u.p{2,158}.g-3/31)<0.04&&abs(u.p{2,158}.b-8/31)<0.04]' info:)
  [ "$panel" = 1 ] || return 1
  colors=$(magick "$file" -sample '240x160!' -crop '240x104+0+12' +repage -format '%k' info:)
  [ "$colors" -ge 12 ]
}
wait_cinematic(){
  local name="$1" until="$2" ready=0
  while (( SECONDS < until )); do
    alive
    if ! xdotool getwindowname "$win" >/dev/null 2>&1; then
      echo 'mGBA window disappeared during the hospital preview' >&2
      exit 1
    fi
    capture "$name"
    if cinematic_ready "screenshots/$name.png"; then
      ready=$((ready+1))
      if (( ready >= 2 )); then return; fi
    else
      ready=0
    fi
    sleep 0.5
  done
  echo "Timed out waiting for a visible Missy cinematic ($name); last capture retained" >&2
  cat "screenshots/${label}-emulator.log"
  exit 1
}
wait_cinematic "${label}-opening" "$deadline"
# A headless Xvfb server may have no window manager; focus is best-effort.
# Send keys to the explicit window below, and fail if that injection fails.
xdotool windowfocus "$win" || true
# mGBA's default key bindings: X for A, Z for B. Alternate them so the
# typewriter and caption pagination don't swallow one giant held-down key.
for i in $(seq 1 16); do
  xdotool key --clearmodifiers --window "$win" x
  sleep 0.22
  xdotool key --clearmodifiers --window "$win" z
  sleep 0.22
done
wait_cinematic "${label}-bedside" "$((SECONDS+15))"
for file in "screenshots/${label}-opening.png" "screenshots/${label}-bedside.png"; do
  dims=$(magick identify -format '%w %h %k' "$file")
  read -r width height colors <<< "$dims"
  echo "$file: ${width}x${height}, ${colors} unique colours"
  test "$width" -ge 240 && test "$height" -ge 160
  test $((width*2)) -eq $((height*3))  # cropped game viewport must be 3:2, not emulator chrome
  test "$colors" -ge 12
done
# Capturing twice the same frozen frame is not a valid visual smoke test.
if cmp -s "screenshots/${label}-opening.png" "screenshots/${label}-bedside.png"; then
  echo "No visual change across dialogue and A/B advances" >&2
  exit 1
fi
echo "Missy ${label} mGBA capture: PASS"
