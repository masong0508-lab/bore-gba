#!/bin/bash
# Host timing of the shared simulation in each port's size/cap configuration.
# These are comparable CPU benchmarks, not timings on DS/DSi/3DS hardware.
set -e
cd "$(dirname "$0")/.." || exit 1
size=${GG_BENCH_SIZE:-128}
ticks=${GG_BENCH_TICKS:-12000}
seed=${GG_BENCH_SEED:-777}
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
echo "Host simulation timings: planet $size, ticks $ticks, seed $seed"
for port in ds dsi_ds dsi16 3ds pc_big; do
    case "$port" in
        ds) flags='-DGG_MAXC_TEST=12000' ;;
        dsi_ds) flags='-DGG_MAXC_TEST=12000 -DGG_DSI' ;;
        dsi16) flags='-DGG_DSI16' ;;
        3ds) flags='-D__3DS__' ;;
        pc_big) flags='-DGG_BIG' ;;
    esac
    gcc -O2 -DNDSSHIM $flags -Icompat -Isource -Wno-all -include unistd.h -o "$out/$port" pc_prof.c -lm
    printf '%s: ' "$port"
    "$out/$port" "$size" "$ticks" "$seed" | grep -E '^(planet|tick ms|FRAME simulate)'
done
