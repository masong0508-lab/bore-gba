#!/usr/bin/env bash
# Render the actual C cutscene source on the host for quick visual iteration.
# Usage: bash tools/render_cutscene.sh SCENE BEAT FRAME OUTPUT.png [FIRST_BEAT]
#        bash tools/render_cutscene.sh --check-patient
# No ROM build or emulator is replaced by this preview: hardware timing/audio
# and register-based fades are deliberately outside its scope.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT

# Read primitives from the game's source on every build to avoid a separate
# approximation of clipping, line/disc shapes, font layout, or color blending.
python3 - "$root" "$scratch/render_core.h" <<'PY'
from pathlib import Path
import re, sys
src = (Path(sys.argv[1]) / 'source/main.c').read_text()

def function(name):
    pattern = re.compile(r'^.*\b'+re.escape(name)+r'\([^;\n]*\)\s*\{', re.M)
    match = pattern.search(src)
    if not match:
        raise SystemExit(f'Missing host primitive {name}')
    begin = match.start()
    p = src.index('{', match.start())
    depth = 1
    i = p + 1
    # Selected primitives have no braces in string literals or comments.
    while depth:
        depth += (src[i] == '{') - (src[i] == '}')
        i += 1
    return src[begin:i]

parts = [function(n) for n in ('clipSet','clipAll','px','rect','line','disc','rnd8')]
start = src.index('#include "fontdata.h"')
end = src.index('static void num(', start)
parts.append(src[start:end])
parts.append(re.search(r'^enum \{ SFX_BONK.*?;', src, re.M).group())
Path(sys.argv[2]).write_text('\n\n'.join(parts)+'\n')

# csPlay applies saved-story caption alternatives before drawing; preserve
# those words too, without bringing the hardware playback loop into the host.
cut = (Path(sys.argv[1]) / 'source/cutscene.h').read_text()
start = cut.index('        if(id==3&&bi==0){ csAlternate=*b;')
end = cut.index('        int total=csLayout(b)', start)
body = cut[start:end]
Path(sys.argv[2]).with_name('render_caption.h').write_text(
    'static const CsBeat *renderCaption(int id,int bi,const CsBeat *b){\n'
    '    static CsBeat csAlternate;\n'+body+'    return b;\n}\n')
PY

"${CC:-cc}" -std=c11 -O2 -fno-strict-aliasing -Wno-unused-function \
  -Wno-unused-variable -Wno-missing-field-initializers -Wno-misleading-indentation \
  -I"$scratch" -I"$root/source" "$root/tools/render_cutscene.c" -o "$scratch/render_cutscene"

if [ "$#" -eq 1 ] && [ "$1" = --check-patient ]; then
  "$scratch/render_cutscene" --check-patient
  exit
fi
if [ "$#" -lt 4 ] || [ "$#" -gt 5 ]; then
  "$scratch/render_cutscene"
fi
out=$4
mkdir -p "$(dirname "$out")"
args=("$1" "$2" "$3" "$scratch/frame.ppm")
if [ "$#" -eq 5 ]; then args+=("$5"); fi
"$scratch/render_cutscene" "${args[@]}"
case "$out" in
  *.ppm) cp "$scratch/frame.ppm" "$out" ;;
  *) python3 - "$scratch/frame.ppm" "$out" <<'PY'
from PIL import Image
import sys
Image.open(sys.argv[1]).save(sys.argv[2])
PY
    ;;
esac
printf '%s\n' "$out"
