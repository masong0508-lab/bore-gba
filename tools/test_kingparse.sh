#!/bin/sh
# test_kingparse.sh - parse the WHOLE game (source/main.c, which includes every king*.h) with the host compiler, and fail on anything the ARM build would
# reject that a host can see: a macro of main.c clashing with a name in the realm code (#define W 6 vs a struct member W), a missing declaration
# (memset, size_t), a wrong argument count. From the repo root:   sh tools/test_kingparse.sh
# The ONLY errors allowed are the ARM-only attribute('target("arm")') ones (baseline: 14), which a host compiler cannot parse. Nothing is linked or run.
T=${TMPDIR:-/tmp}
cc -fsyntax-only source/main.c >"$T/kp.out" 2>&1
bad=$(grep -E "error:" "$T/kp.out" | grep -v "attribute 'target'" | wc -l)
imp=$(grep -E "source/king[a-z]*\.h.*(implicit declaration|incompatible implicit|undeclared|redefined)" "$T/kp.out" | wc -l)
tgt=$(grep -c "attribute 'target'" "$T/kp.out")
if [ "$bad" -ne 0 ] || [ "$imp" -ne 0 ]; then grep -E "error:|source/king[a-z]*\.h.*(implicit|undeclared|redefined)" "$T/kp.out" | grep -v "attribute 'target'" | head -20; echo "kingparse: FAILED ($bad error(s), $imp declaration problem(s) in the king files)"; exit 1; fi
echo "kingparse: all checks passed (host parse of main.c: 0 real errors, $tgt ARM-only target errors)"
