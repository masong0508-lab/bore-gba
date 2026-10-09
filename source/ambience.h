// ambience.h - NATURE SOUNDS: which outdoor bed plays under the game and how loud (the mixing is main.c's ambMix, the beds tools/make_ambience.py).
//   DAY      songbirds in the trees and a breeze, from 6:00 to 20:00 (fx.h's night: simIsNight)
//   NIGHT    crickets, a far tree cricket, peepers, a bullfrog, an owl now and then
//   RAIN     falling rain and drips off the roof (RAIN and STORM weather; the storm's thunder is fx.h's)
//   WIND     gusts and the wind finding gaps (SNOW weather)
// Indoors (a tile inside a room's walls, or an upper floor) the bed is softer and duller, as through a window. A new bed fades the old one
// out first (about a second each way), so dawn, dusk and a change of weather pass smoothly. OPTIONS > AUDIO > NATURE SOUNDS sets the level
// (with MASTER VOLUME), 0 switches it off, and SOUND OFF silences it with everything else. Only in play: ambOff when you leave.
// Needs before it: main.c's ambD / ambN / ambP / ambG / ambK and audStart, fx.h (wx, simIsNight), wInside, the player (lfx, lfy, curFl).
enum { AMB_NONE, AMB_DAY, AMB_NIGHT, AMB_RAIN, AMB_WIND };
static u8 ambCur EWRAM_BSS;   // the bed loaded in the channel now
static const u8* ambBed(int b){ return b==AMB_DAY?amb_day:b==AMB_NIGHT?amb_night:b==AMB_RAIN?amb_rain:b==AMB_WIND?amb_wind:0; }
static void ambTick(void){   // once a frame in play
    int want=AMB_NONE, lvl=0, k=256;
    if(sSnd&&xo[XO_AMBV]&&!prShown()){
        u8 w=wx;
        want=(w==WX_RAIN||w==WX_STORM)?AMB_RAIN:w==WX_SNOW?AMB_WIND:simIsNight()?AMB_NIGHT:AMB_DAY;
        static const u8 trim[5]={0,210,190,150,170};   // each bed's level (out of 256) at the default NATURE SOUNDS: there to hear, never in the way of the music
        lvl=(oAmbGain()*trim[want])>>8;
        int tx=(int)(lfx>>8), ty=(int)(lfy>>8), in=curFl>0||(tx>=0&&ty>=0&&tx<MW&&ty<MH&&wInside[ty][tx]);
        if(in){ lvl=lvl*2/5; k=64; }   // indoors: through the walls (ambMix dulls it: a quarter step low pass)
    }
    ambK=k;
    int tg=ambCur==want?lvl:0, g=ambG;   // a different bed: the old one fades out first
    if(g<tg){ g+=4; if(g>tg) g=tg; } else if(g>tg){ g-=6; if(g<tg) g=tg; }
    if(g==0&&ambCur!=want){ ambG=0; const u8*b=ambBed(want); ambD=0; ambN=b?*(const u32*)b:0; ambP=b?(u32)(rnd8()<<8)%(ambN?ambN:1):0; ambD=b?(const signed char*)(b+4):0; ambCur=(u8)want; }   // (a random start: the loop never begins the same way)
    ambG=g;
    if(g&&!mOn) audStart();   // (the mixer may be asleep: no song, no effect)
}
static void ambOff(void){ ambG=0; ambD=0; ambCur=AMB_NONE; }   // leaving play: silent at once
