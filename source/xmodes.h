// xmodes.h - the packs' MODES (expand.h exModeRun): BORTUA FIGHTER's ARCADE MODE (bortua.h) and BOREagotchi's HANDHELD (gotchi.h).
static int exModeRun(int p){
    if(p==XP_GOTCHI) return goRun();   // gotchi.h
    if(p==XP_FIGHT) return bfRun();    // bortua.h
    if(p==XP_KING) return kgRun();     // kingscreen.h
    return 0;
}
