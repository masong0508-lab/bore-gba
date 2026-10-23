# BORE creator open: the menu picture no longer freezes while the creator opens. The creature area is cleared and shown at once, the panel and the Bore follow,
# and the creator's music starts after the first picture is on screen (it used to start before anything was drawn). Run inside ~/bore-gba. Safe to run twice.
python3 -I - <<'PYEOF'
import sys
s=open("source/main.c").read()
def req(old,new,label):
    global s
    if s.count(new)>=1: return
    if s.count(old)!=1: print("anchor not found once:",label); sys.exit(1)
    s=s.replace(old,new)
req("static void creatureEditor(void){ creatorMusStart(); for(;;){","static u8 ceMusPend;   // the creator's music waits for its first picture\nstatic void creatureEditor(void){ if(sUnlock&&sClassic) creatorMusStart(); else ceMusPend=1; for(;;){","editor")
req("if(!custom) buildLook(); }   // BIG HEAD goes away when the Konami code is off\n    for(;;){","if(!custom) buildLook(); }   // BIG HEAD goes away when the Konami code is off\n    clipAll(); rect(0,0,PANEL_X,SH,SKY); edCopyRows(0,0,PANEL_X,SH);   // at once: the old menu picture goes, the Bore draws in a moment later\n    for(;;){","clear")
req("edCopyRows(0,0,PANEL_X,SH); }   // (clipped: nothing spills onto the panel drawn before it)\n            vsyncUi(); dirty=0;","edCopyRows(0,0,PANEL_X,SH); }   // (clipped: nothing spills onto the panel drawn before it)\n            vsyncUi(); dirty=0;\n            if(ceMusPend){ ceMusPend=0; creatorMusStart(); }   // the first picture is up: now the tune","music")
open("source/main.c","w").write(s)
PYEOF
[ $? -eq 0 ] || exit 1
echo "creator open patched"
