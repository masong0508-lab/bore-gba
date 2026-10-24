// kinghand.h - EP6 THE LAND BOREFORE TIME, Phase 4 step d: BUILDING BY HAND. PURE C (needs kingdom.h and kingsim.h before it; no hardware).
// The cursor is the cell under the middle of the screen (the D-pad scrolls the map under it), so there is no cursor to move: the camera is the cursor.
// A "mode" is what A does there: lay or lift a ROAD, put a building SITE down (workers carry its goods and raise it), or DEMOLISH what stands there.
// NOTE (step 4c): main.c #defines W, D and H. No single capital letters as names here.
#ifndef KINGHAND_H
#define KINGHAND_H
enum { KHM_ROAD, KHM_HUT, KHM_HOUSE, KHM_WOODCUT, KHM_QUARRY, KHM_SAWMILL, KHM_MASON, KHM_STORE, KHM_DEMO, KHM_N };   // KHM_ROAD + 1 + building type = the building's mode
enum { KHR_NONE, KHR_ROAD, KHR_UNROAD, KHR_SITE, KHR_RAZED, KHR_NOTHING, KHR_SPOT, KHR_FULL, KHR_ARG };   // what an A press did
typedef struct { int mode; } KHand;
static const char* const khName[KHM_N]={ "ROAD", "HUT", "HOUSE", "WOODCUTTER", "QUARRY", "SAWMILL", "MASON", "STOREHOUSE", "DEMOLISH" };
static const char* const khMsg[9]={ "", "ROAD LAID", "ROAD LIFTED", "SITE PLACED", "DEMOLISHED", "NOTHING HERE", "CANNOT BUILD HERE", "NO ROOM FOR MORE", "OUT OF THE REALM" };
static inline void khInit(KHand*B){ B->mode=KHM_ROAD; }
static inline void khCycle(KHand*B,int dir){ B->mode=(B->mode+(dir>0?1:KHM_N-1))%KHM_N; }
// the cell under the middle of the screen, from the camera in pixels (the screen is 240 x 160: its middle is 120, 80)
static inline int khCx(int camx){ return (camx+120)>>3; }
static inline int khCy(int camy){ return (camy+80)>>3; }
// the building whose 2 x 2 footprint covers cell (x,y), or -1
static inline int khBldAt(const KSim*S,int x,int y){
    for(int i=0;i<S->R->nB;i++){ const KBld*b=&S->B[i]; if(x>=b->x&&x<b->x+kgFootW(b->type)&&y>=b->y&&y<b->y+kgFootH(b->type)) return i; }
    return -1; }
// A pressed in a mode on cell (x,y). A building's top-left corner is the cursor cell. Returns a KHR_ code; the caller redraws the map when it is not a refusal.
static inline int khAct(KHand*B,KSim*S,int x,int y){
    int d=S->dim; if(x<0||y<0||x>=d||y>=d) return KHR_ARG;
    if(B->mode==KHM_DEMO){ int i=khBldAt(S,x,y); if(i<0) return KHR_NOTHING; return ksDemolish(S,i)==KSE_OK?KHR_RAZED:KHR_ARG; }
    if(B->mode==KHM_ROAD){ int c=S->cells[y*d+x];
        if(c&KC_BLD) return KHR_SPOT;
        if((c&7)==KT_ROAD){ return ksRoadSet(S,x,y,0)==KSE_OK?KHR_UNROAD:KHR_ARG; }
        int e=ksRoadSet(S,x,y,1); return e==KSE_OK?KHR_ROAD:e==KSE_CAP?KHR_FULL:e==KSE_ARG?KHR_ARG:KHR_SPOT; }
    { int e=ksPlace(S,B->mode-KHM_HUT,x,y,0); return e>=0?KHR_SITE:e==KSE_CAP?KHR_FULL:e==KSE_ARG?KHR_ARG:KHR_SPOT; }
}
// the cursor's look: bank 15 of the palette is bank 0 brightened, so a blinking cell shows where A will act
static inline u16 khPalCur(u16 c){ int r=c&31, g=(c>>5)&31, b=(c>>10)&31; r+=(31-r)*2/3; g+=(31-g)*2/3; b+=(31-b)*2/3; return (u16)(r|(g<<5)|(b<<10)); }
#endif
