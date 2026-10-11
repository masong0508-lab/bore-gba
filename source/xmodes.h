// xmodes.h - the packs' MODES (expand.h exModeRun): BORTUA FIGHTER's ARCADE MODE (bortua.h) and BOREagotchi's HANDHELD (gotchi.h).
static int exModeRun(int p){
    if(p==XP_GOTCHI) return goRun();   // gotchi.h
    toast("ARCADE MODE IS COMING IN THE NEXT UPDATE"); return 0;
}
