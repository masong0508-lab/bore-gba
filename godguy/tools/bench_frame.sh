#!/bin/bash
# Repeatable desktop CPU timing of the gameplay frame for each GodGuy port.
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
frames=${GG_STRESS_FRAMES:-1200}
wide=${GG_STRESS_WIDE:-0}

run() {
    local name=$1 size=$2 hq=$3
    shift 3
    printf '\n--- %s ---\n' "$name"
    (cd "$tmp" && "$@" "$size" "$frames" "$hq")
}

build() {
    local name=$1
    shift
    "${CC:-gcc}" -O2 -DNDSSHIM -Icompat -Isource -include unistd.h "$@" pc_stress.c -lm -o "$tmp/$name"
}

if (( $# == 0 )); then set -- ds ds_sd dsi_ds dsi16 3ds pc; fi
for port in "$@"; do
    case "$port" in
        ds|ds_sd|dsi_ds|dsi16)
            flags=()
            case "$port" in
                ds) flags=(-DGG_MAXC_TEST=12000) ;;
                dsi_ds) flags=(-DGG_MAXC_TEST=12000 -DGG_DSI) ;;
                dsi16) flags=(-DGG_DSI16) ;;
            esac
            build "$port" "${flags[@]}"
            run "$port" 256 0 "$tmp/$port"
            if [[ $port == dsi16 && $wide == 1 ]]; then run 'dsi16 wide' 768 0 "$tmp/$port"; fi
            ;;
        3ds)
            build 3ds -D__3DS__ -DSHIM_HQ -DGG_HQVOX -DGG_STRESS_SOFTWARE
            run 'Old 3DS' 256 0 "$tmp/3ds"
            run 'New 3DS' 256 1 "$tmp/3ds"
            if [[ $wide == 1 ]]; then
                run 'Old 3DS wide' 1024 0 "$tmp/3ds"
                run 'New 3DS wide' 1024 1 "$tmp/3ds"
            fi
            ;;
        pc)
            build pc -DGG_BIG -DGG_STRESS_SOFTWARE
            run 'PC shim' 256 0 "$tmp/pc"
            if [[ $wide == 1 ]]; then run 'PC shim wide' 1024 0 "$tmp/pc"; fi
            ;;
        *) printf 'Unknown port %s\n' "$port" >&2; exit 2 ;;
    esac
done
