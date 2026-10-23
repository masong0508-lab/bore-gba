// counters.h - COUNTERS WITH A JOB: R next to a sink washes you up, R next to a microwave heats a snack.
// catalog.h catFn (from tools/make_catalog.py, item(..., fn=)): bit 0 a SINK, bit 1 a MICROWAVE. The COUNTERS tab of the BUY menu is full of them
// (SINK COUNTER, DOUBLE SINK, FARM SINK, MICROWAVE COUNTER, STEEL MICROWAVE, SINK + MICROWAVE); the kitchen's own KITCHEN SINK and MICROWAVE STAND work too.
// main.c: the 3 x 3 tiles round you are looked at with the other furniture (ctrFn per counter tile, summed in ctrM), lnear 17 = a counter with a job is in reach.
// The FRIDGE COUNTER is an F like every fridge: it needs nothing here.
// Include after skills.h (needs menu, moodEvent, sfxPlay and the life globals).
static int ctrFn(int x,int y){ int id=catIdAt(x,y); return id?(int)catFn[id-1]:0; }   // what the counter on tile (x, y) can do (0: nothing)
static const char* ctrHint(void){ return ctrM==3?"R USE THE COUNTER":ctrM==1?"R WASH YOUR HANDS":"R USE THE MICROWAVE"; }
static void ctrSink(void){   // wash up: hygiene back up a little each time; a proper cheer only when you were grubby
    if(sHyg>=98){ lnote="SQUEAKY CLEAN"; lnoteT=40; return; }
    int grubby=sHyg<60; sHyg+=15; if(sHyg>100) sHyg=100;
    lstun=35; lsp=0; lgrind=0; lnote=grubby?"SCRUBBED UP":"WASHED YOUR HANDS"; lnoteT=50; if(grubby) moodEvent(M_SHOWER); sfxPlay(SFX_SPLASH);
}
static void ctrMicro(void){   // a snack in ninety seconds: less than the fridge (no cooking in it, so no cooking skill either), but quick
    if(lfood>=95){ lnote="NOT HUNGRY"; lnoteT=40; return; }
    int hungry=lfood<60; lfood+=22; if(lfood>100) lfood=100; lbl+=6; if(lbl>99) lbl=99;
    lstun=45; lsp=0; lgrind=0; lnote="BEEP BEEP  YUM"; lnoteT=60; if(hungry) moodEvent(M_EAT); sfxPlay(SFX_MUNCH);
}
static void ctrUse(void){   // R at a counter with a job: a sink, a microwave, or (SINK + MICROWAVE) you pick
    int m=ctrM;
    if(m==3){ static const char* const it[2]={"WASH YOUR HANDS","MICROWAVE A SNACK"}; int c=menu("THE COUNTER",it,2); if(c<0) return; m=c?2:1; }
    if(m==1) ctrSink(); else ctrMicro();
}
