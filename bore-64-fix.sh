#!/bin/sh
# Fixes the compile error from bore-64-patch.sh (main.c:6017 'nbTp' is a pointer). Run in ~/bore-gba: sh bore-64-fix.sh
python3 -I - <<'EOF'
p="source/main.c"; s=open(p).read(); n=s.count("&nbTp.")
if n!=1: print("expected 1 broken spot, found",n); raise SystemExit(1)
open(p,"w").write(s.replace("&nbTp.","&&nbTp->")); print("fixed")
EOF
