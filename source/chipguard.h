// chipguard.h - SAVE CHIP GUARD: warns at boot when the save chip is SMALLER than the one this save was last used on.
// Typical cause: the emulator's save type was changed (or reset after a new ROM), so a 128 KB flash save is read as 32 KB SRAM.
// The slots past the smaller chip then look empty, and saving over them can wreck the save. The guard keeps a tiny tag at CG_OFF
// (inside the 16 bytes reserved for SLOT_DIR, which uses only 4): 'C' 'K', chip size in 32 KB units (1, 2 or 4), checksum.
// The tag only ever grows, so the warning stays until the chip is right again. Needs before it: svType, svSize, svRd, svWr, rect, box, text, tw, present, numStr.
#define CG_OFF 5000
_Static_assert(CG_OFF>=4992+4&&CG_OFF+4<=5008,"the chip tag must sit in the free part of the SLOT_DIR block");
static int cgUnits(void){ return svType==SV_FLASH?(int)(svSize>>15):1; }   // 1 = 32 KB SRAM, 2 = 64 KB flash, 4 = 128 KB flash
static const char* cgName(int u){ return u>=4?"128 KB FLASH":u==2?"64 KB FLASH":"32 KB SRAM"; }
static void chipGuard(void){
    int cur=cgUnits(), tag=0;
    if(svRd(CG_OFF)=='C'&&svRd(CG_OFF+1)=='K'){ int u=svRd(CG_OFF+2); if(svRd(CG_OFF+3)==(u8)(0x43+u)&&(u==1||u==2||u==4)) tag=u; }
    if(tag>cur){   // the save was last used on a bigger chip: say so, and leave the tag alone
        rect(0,0,SW,SH,RGB(3,4,8)); box(10,14,220,132);
        text(120-tw("SAVE CHIP CHANGED",1)/2,22,"SAVE CHIP CHANGED",GOLD,1);
        text(18,40,"THIS SAVE WAS LAST USED ON",WHITE,1); text(18,50,cgName(tag),GOLD,1);
        text(18,66,"THIS RUN HAS ONLY",WHITE,1); text(18,76,cgName(cur),GOLD,1);
        text(18,92,"SET THE SAVE TYPE TO FLASH",DIMC,1); text(18,102,"128K IN YOUR EMULATOR, THEN",DIMC,1); text(18,112,"RESTART. SAVING NOW CAN HIDE",DIMC,1); text(18,122,"OR DAMAGE YOUR SLOTS.",DIMC,1);
        text(120-tw("A  CONTINUE ANYWAY",1)/2,134,"A  CONTINUE ANYWAY",WHITE,1);
        present();
#ifndef CG_NOWAIT
        while(!((u16)~REG_KEYINPUT&K_A)){} while((u16)~REG_KEYINPUT&K_A){}
#endif
        return;
    }
    if(tag!=cur){ svWr(CG_OFF,'C'); svWr(CG_OFF+1,'K'); svWr(CG_OFF+2,cur); svWr(CG_OFF+3,0x43+cur); }   // first run, or the chip got bigger
}
