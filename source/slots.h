// slots.h - ROOM SLOTS: several named saves of a room, the person living in it and their life.
//
// WHAT A SLOT HOLDS      Chunks, each optional: ROOM (tiles, floors, wallpaper, run-length packed), PERSON (the creature: look, and
//                        its hand-built blocks if any), LIFE (needs, cash, job, clock, skill: the same block sims.h saves: 52 bytes "SIM3", older slots 24 bytes "SIM2").
// WHERE                  SRAM 20480.., SLOT_N slots of SLOT_SZ bytes (the old single room at 0, settings, jukebox and the life keep their places).
// ACTIVE SLOT            The slot you saved to / loaded last. A tiny block at SLOT_DIR remembers it. Options use it:
//                        SAVE MAP TO SLOT (the map editor's SAVE MAP also writes the active slot), BOOT LOADS PERSON.
//
// ---- THE FORMAT (made so a whole HOUSE can be added later without breaking any save) -------------------------------------
//  header (SLOT_HDR = 32 bytes)   0 'S' 1 'V' 2 version(1) 3 KIND 4 SPAN 5 has-mask 6..7 payload length 8..9 payload checksum
//                                 10 map width 11 map height 12..13 save counter 14..15 reserved (house: room count)
//                                 16..25 name 26..30 reserved 31 header checksum
//  KIND   0 = ROOM (one room).  1 = HOUSE (reserved: several rooms and how they connect, see "HOUSES" below).
//         2 = HOUSEHOLD (the Sims living with you and how they feel about each other: the house.h block from SRAM, byte for byte, as the
//         payload. One or two slots (SPAN). Loading one puts it back at SL_HH_OFF; it never becomes the active slot.)
//  SPAN   how many consecutive slots the save covers. A room is 1. A house spans several slots: its payload simply runs on
//         into the next slots' bytes, and the scan (slScan) skips the slots it covers.
//  payload   a list of chunks:  tag(1) length(2) data ...  and a 0 tag at the end. Readers SKIP tags they do not know, so new
//         chunk types can be added later. Tags: 'R' room, 'C' person, 'L' life, 'H' house plan (reserved).
//  'R' chunk  room index(1) format(2 = runs) then runs of  count, tile, floor, wallpaper (format 1: count, tile, floor<<4|wallpaper).  A ROOM slot has room index 0.
//  Every save is verified (checksum) before it is loaded, and the slot is invalidated while it is being written, so a power
//  cut in the middle of a save can only lose that one slot, never corrupt another.
//
// ---- HOUSES (not done yet: where to start) ----------------------------------------------------------------------------
//  A house = KIND 1, SPAN n, one 'R' chunk per room (room index 0..), plus an 'H' chunk: for each room its name, its position
//  in the house and its doors (which tile of which room leads to which tile of which room). Write houseSave()/houseLoad() below,
//  set SLOT_HOUSE_READY to 1, and the slot screen already lists, deletes and protects house slots.
//  The room editor edits one room at a time; a house load would copy room 0 into lifeMap and keep the rest until a door is used.
//
// Needs before it: the map (lifeMap/floorMap/wallMap, MW, MH, palIdx, palCol, flFlat), the creature (look, vox, dec, custom, buildLook,
// setColors), sims.h (simsPack/simsCheck/simsUnpack/simsSaveNow), SRAM_BASE, mapSave/mapScan, and the UI kit (box, menu, helpScreen, toast).
//
// ---- THE SRAM MAP (32 KB), layout 2 ------------------------------------------------------------------------------------
//     0      .. 4802    the room being played ("BM3", main.c)
//     4808   .. 4810    layout marker 'L' 'Y' 2 (slMigrate)
//     4816   .. 4824    TIMED RUN high score (TRN_OFF, timedrun.h)
//     4864   .. 4879    settings (SET_OFF)            4896 .. 4991   extended options (OPT_OFF)
//     4992   .. 5007    active slot (SLOT_DIR)        5008 .. 5023   life stage (AGE_OFF)
//     5014   .. 5023    skills (SK_OFF, skills.h)
//     5024   .. 5055    persona (PERS_OFF)            5056 .. 5135   jukebox (JB_OFF)
//     5136   .. 5199    the life (SIM_OFF)            5200 .. 5215   SAVE MEMORY TEST (SRAM_TEST)
//     5216   .. 8191    the household (SL_HH_OFF; SL_HH_LEN = 2976 bytes are reserved; it was 1024, then 2048)
//     8192   .. 32767   SLOT_N room slots of SLOT_SZ bytes (twelve; layout 1 had six, from 20480, and its small blocks in between)
// Layout 1 saves are carried over the first time the game starts (slMigrate): the old six slots keep their bytes and become slots 7 to 12.
#define SLOT_BASE  8192
#define SLOT_N0    12         // slots on 32 KB SRAM
#define SLOT_MAX   58         // slots on 128 KB flash (8192 .. 126975; the last 4 KB are save.h's scratch sector). 64 KB flash: 26
static int slN=SLOT_N0;       // how many this save chip holds (slInitN, at power on)
#define SLOT_N slN
#define SLOT_SZ    2048
#define SLOT_HDR   32
#define SLOT_NAME  10
#define SLOT_DIR   4992       // 'S' 'D' active-slot checksum   (4 bytes, 16 reserved)
#define SRAM_TEST  5200       // 16 spare bytes the SAVE TEST in the options writes to
#define SL_HH_OFF  5216       // the household block (house.h); SL_MIG_HH bytes are reserved for it
#define SL_MIG_HH  1024       // what the layout 1 -> 2 upgrade copies (old households fit in it)
#define SL_HH_LEN  2976       // what the household block may use now (5216..8191; nothing else lives up to SLOT_BASE)
#define SL_MIG_TAG 4808
#define SL_OLD_BASE 20480     // layout 1: the six slots started here
#define SL_OLD_N    6
#define SLOT_HOUSE_READY 1
_Static_assert(SLOT_BASE+SLOT_N0*SLOT_SZ<=32768,"the slots do not fit in 32 KB of SRAM");
_Static_assert(SLOT_BASE+SLOT_MAX*SLOT_SZ<=131072-4096,"the slots do not fit in 128 KB of flash");
_Static_assert(SL_OLD_BASE==SLOT_BASE+SL_OLD_N*SLOT_SZ,"the layout 1 slots must land on whole slots of the new bank");
_Static_assert(3+MSZ*3<=SL_MIG_TAG,"the room being played must end before the layout marker");
_Static_assert(SL_MIG_TAG+3<=SET_OFF&&SET_OFF+16<=OPT_OFF&&OPT_OFF+3+XO_N+1<=SLOT_DIR&&SLOT_DIR+4<=AGE_OFF&&AGE_OFF+5<=PERS_OFF,"the small SRAM blocks overlap (1)");
_Static_assert(PERS_OFF+PERS_LEN<=JB_OFF&&JB_OFF+5+JB_MAX<=SIM_OFF&&SIM_OFF+SIM_BLOCK<=SRAM_TEST&&SRAM_TEST+16<=SL_HH_OFF&&SL_HH_OFF+SL_HH_LEN<=SLOT_BASE&&SL_MIG_HH<=SL_HH_LEN,"the small SRAM blocks overlap (2)");
_Static_assert(NWALL<=255&&NFL<=255,"a run stores the floor and the wallpaper in a byte each");

enum { SLK_ROOM=0, SLK_HOUSE=1, SLK_HHOLD=2, SLK_TOWN=3, SLK_PLAYER=4 };   // SLK_PLAYER: a PLAYER SAVE FILE (savegame.h)
static u8 sgPid EWRAM_BSS, sgWant EWRAM_BSS, slHPid EWRAM_BSS;   // player in play (0 none) / a new player about to start / extra header byte for the next slHeader
static int sgSave(void); static int sgDeletePid(int pid); static void sgPickHome(void); static int sgList(int*l); static int sgHomeOf(int slot);   // savegame.h
enum { SLH_ROOM=1, SLH_PERSON=2, SLH_LIFE=4 };                 // what a slot holds / what to load
enum { SLE_OK=0, SLE_EMPTY=-1, SLE_BAD=-2, SLE_BIG=-3, SLE_HOUSE=-4, SLE_FMT=-5, SLE_SIZE=-6, SLE_NOPART=-7, SLE_NOSRAM=-8, SLE_NOROOM=-9, SLE_VISIT=-10 };
static int hhBlockLen(volatile u8*m,int avail); static void hhSave(void); static void hhLoad(void);   // house.h (included further down)
#define SLC_ROOM   'R'
#define SLC_PERSON 'C'
#define SLC_LIFE   'L'
#define SLC_HOUSE  'H'
#define SLC_STATS  'T'   // lifestats.h: the lifetime stats of this player (LS_N x 4 bytes); here because statscreen.h needs it before savegame.h
#define CNV (H*D*W)                                            // voxels in the creature (192)
#define SLOT_NSPR NSPR

// ---------- layout upgrade: layout 1 -> layout 2 (run once, first thing at power on) ----------
// Layout 1 kept the small blocks between 8192 and 20479. Layout 2 puts the slots there, so the small blocks are copied down to
// where main.c, sims.h, jukebox.h and house.h now expect them. Order matters, so a power cut at any point is safe:
//   1 copy every block (the old copies stay untouched, running again just copies again)   2 move the active slot number on by
//   six (the old slots are now slots 7 to 12)   3 write the marker   4 only then clear the old blocks.
// The six old slots at 20480.. are never touched: their bytes simply are slots 7 to 12 now.
typedef struct { u16 from, to, len; } SlMove;
#define SLO(s) (SLOT_BASE+(u32)(s)*SLOT_SZ)   // where slot s starts in the save memory
static void slInitN(void){ int n=(int)((svSlotEnd()-SLOT_BASE)/SLOT_SZ); slN=n>SLOT_MAX?SLOT_MAX:n; }
static void slMigrate(void){
    volatile u8*m=SRAM_BASE;
    if(m[SL_MIG_TAG]=='L'&&m[SL_MIG_TAG+1]=='Y'&&m[SL_MIG_TAG+2]==2) return;
    static const SlMove mv[]={ {8192,SET_OFF,16}, {8448,OPT_OFF,96}, {12352,SLOT_DIR,16}, {12416,AGE_OFF,16}, {12432,PERS_OFF,32},
                               {14336,JB_OFF,80}, {16384,SIM_OFF,64}, {18448,SL_HH_OFF,SL_MIG_HH} };
    for(unsigned k=0;k<sizeof(mv)/sizeof(mv[0]);k++) for(int i=0;i<mv[k].len;i++) svWr(mv[k].to+i,svRd(mv[k].from+i));
    volatile u8*d=m+SLOT_DIR;
    if(d[0]=='S'&&d[1]=='D'&&d[2]<SL_OLD_N&&d[3]==(u8)(0x5D+d[2])){ d[2]=(u8)(d[2]+SL_OLD_N); d[3]=(u8)(0x5D+d[2]); }
    else { for(int i=0;i<4;i++) d[i]=0; }
    m[SL_MIG_TAG]='L'; m[SL_MIG_TAG+1]='Y'; m[SL_MIG_TAG+2]=2;
    svErase(SLOT_BASE,SL_OLD_BASE-SLOT_BASE); svCommit();
}

// ---------- byte writer (a counting dry run when p is 0) and reader ----------
typedef struct { u32 off; int pos,cap,over; u32 s1,s2; } SlW;   // off: where in the save memory it writes (0 = only count)
static void slwInit(SlW*w,u32 off,int cap){ w->off=off; w->pos=0; w->cap=cap; w->over=0; w->s1=w->s2=0; }
static void slwPut(SlW*w,int b){
    if(w->pos>=w->cap){ w->over=1; return; }
    b&=255; if(w->off) svWr(w->off+(u32)w->pos,b); w->pos++;
    w->s1+=(u32)b; if(w->s1>=255) w->s1-=255; w->s2+=w->s1; if(w->s2>=255) w->s2-=255;   // Fletcher-16
}
static void slwPut16(SlW*w,int v){ slwPut(w,v&255); slwPut(w,(v>>8)&255); }
static int slwSum(const SlW*w){ return (int)(w->s1|(w->s2<<8)); }
typedef struct { volatile u8*p; int pos,len,bad; } SlR;
static int slrGet(SlR*r){ if(r->pos>=r->len){ r->bad=1; return 0; } return r->p[r->pos++]; }
static int slrGet16(SlR*r){ int a=slrGet(r); int b=slrGet(r); return a|(b<<8); }
static int slSumOf(volatile u8*p,int n){ u32 s1=0,s2=0; for(int i=0;i<n;i++){ s1+=p[i]; if(s1>=255) s1-=255; s2+=s1; if(s2>=255) s2-=255; } return (int)(s1|(s2<<8)); }

// ---------- chunks: ROOM ----------
// Room formats: 1 = runs of (count, tile, floor<<4|wallpaper); 2 = runs of (count, tile, floor, wallpaper);
// 3 = three planes one after the other, tiles then floors then wallpapers, each its own runs of (count, value) covering the whole
//     map. Floors and wallpapers change far less often than the furniture, so each plane packs on its own: about a quarter smaller
//     than format 2 on a furnished room (more rooms fit a slot). Formats 1 and 2 still load; new saves are format 3.
static int slSrcF=-1, slWantRoom=0, slRoomIdx=0;   // house saves: the floor slEncRoom writes (-1: the ground floor as it stands), the room index slDecRoom reads, the index written
static int slPlaneVal(int plane,int i){ return flPlaneAt(slSrcF>=0?slSrcF:0,plane,i); }   // (the floor you are on is read live, any other from flPool; -1 = the ground floor)
_Static_assert(FL_POOL>=4*SLOT_SZ-SLOT_HDR,"flPool (main.c) must hold every house that fits 4 save slots, or a saved house could fail to load: raise FL_POOL, or lower the 4-slot limit in houseSave on purpose");
static void slEncRoom(SlW*w){
    slwPut(w,slRoomIdx);   // room index: 0 = the one room of a ROOM slot, a house numbers its floors
    slwPut(w,3);
    for(int pl=0;pl<3;pl++){
        int n=0, cur=0;
        for(int i=0;i<MSZ;i++){
            int v=slPlaneVal(pl,i);
            if(n>0&&v==cur&&n<255){ n++; continue; }
            if(n){ slwPut(w,n); slwPut(w,cur); }
            n=1; cur=v;
        }
        if(n){ slwPut(w,n); slwPut(w,cur); }
    }
}
// one plane of format 3: runs until the whole map is covered. Returns 1 if fine. apply=1 writes it into the map.
static int slDecRoomPlane(SlR*c,int pl,int apply){
    int i=0;
    while(i<MSZ){
        int n=slrGet(c), v=slrGet(c); if(c->bad||n==0||i+n>MSZ) return 0;
        if(pl==0&&palIdx((char)v)<0) return 0;
        if(pl==1&&v>=NFL) return 0;
        if(pl==2&&v>=NWALL) return 0;
        if(apply) for(int k=0;k<n;k++){ int y=(i+k)/MW, x=(i+k)%MW; if(pl==0) lifeMap[y][x]=(char)v; else if(pl==1) floorMap[y][x]=(u8)v; else wallMap[y][x]=(u8)v; }
        i+=n;
    }
    return 1;
}
// returns 1 if fine (or a room index we do not use), 0 if broken. apply=1 writes the map.
static int slDecRoom(SlR*c,int apply){
    int idx=slrGet(c), fmt=slrGet(c); if(c->bad) return 0;
    if(idx!=slWantRoom) return 1;      // another floor of a house: not wanted now
    if(fmt==3){
        for(int pl=0;pl<3;pl++) if(!slDecRoomPlane(c,pl,apply)) return 0;
        if(apply) wDirty=1;
        return c->pos==c->len;
    }
    if(fmt!=1&&fmt!=2) return 0;
    int i=0;
    if(apply) wDirty=1;
    while(c->pos<c->len){
        int n=slrGet(c), t=slrGet(c), f=slrGet(c), p; if(c->bad||n==0) return 0;
        if(fmt==1){ p=f&15; f>>=4; } else { p=slrGet(c); if(c->bad) return 0; }
        if(palIdx((char)t)<0||f>=NFL||p>=NWALL) return 0;
        if(i+n>MSZ) return 0;
        if(apply) for(int k=0;k<n;k++){ int y=(i+k)/MW, x=(i+k)%MW; lifeMap[y][x]=(char)t; floorMap[y][x]=(u8)f; wallMap[y][x]=(u8)p; }
        i+=n;
    }
    return i==MSZ;
}
// ---------- chunks: PERSON ----------
static void slEncPlane(SlW*w,int plane){   // 0 = voxels, 1 = face sprite low byte, 2 = face sprite high byte; run-length packed
    const u8*v=&vox[0][0][0]; const u16*d=&dec[0][0][0]; int n=0; u8 cur=0;
    for(int i=0;i<CNV;i++){
        u8 b=plane==0?v[i]:plane==1?(u8)(d[i]&255):(u8)(d[i]>>8);
        if(n&&b==cur&&n<255){ n++; continue; }
        if(n){ slwPut(w,n); slwPut(w,cur); }
        n=1; cur=b;
    }
    if(n){ slwPut(w,n); slwPut(w,cur); }
}
static void slEncPerson(SlW*w){
    slwPut(w,14);   // format 14 adds neck width; format 13 adds jaw width, hand size and foot size; format 12 adds tail taper / fluff / wave / tip shade, horn tip and wing droop / shade; format 11 adds chest, belly, upper arm and forearm width and tail / horn shade; format 10 adds neck, hip / waist / shoulder / thigh / calf width and 11 more tail, horn, wing, antenna and ear sliders;                                            // format 9 (8 had no leg width, arm width, antenna, tail length / curl / thickness, horn size / spread / curve / height ear front-back / spread, head size, hand foot size, wing size or tail tip); format 8 (7 had no tone, brow / nose height, torso, arms or stance sliders); format 7 (6 had no claws, antennae or body paint); format 6 (5 had no brows, nose, cheeks, glasses, eye colour or body / face sliders); format 5 (4 had no hats, beards or clothes styles; 3 had no persona: it reads as the one already set; 2 had no sliders: they read as 0 = the middle; 1 had no life stage: those people are adults)
    for(int i=0;i<LK_N;i++) slwPut(w,look[i]);
    slwPut(w,stage); slwPut(w,ageDays);
    slwPut(w,pAsp); slwPut(w,pLtw); for(int i=0;i<TR_N;i++) slwPut(w,pTr[i]);   // persona: aspiration, lifetime want, personality
    slwPut(w,custom?1:0);                                   // hand-built blocks: only then the blocks are stored (else buildLook() remakes them)
    if(custom){ slEncPlane(w,0); slEncPlane(w,1); slEncPlane(w,2); }
}
static u8 slTmp[3][CNV] EWRAM_BSS;                          // decoded planes, checked before anything is applied
static int slDecPlane(SlR*c,int plane){
    int i=0;
    while(i<CNV){ int n=slrGet(c), v=slrGet(c); if(c->bad||n==0||i+n>CNV) return 0; for(int k=0;k<n;k++) slTmp[plane][i+k]=(u8)v; i+=n; }
    return 1;
}
static int slDecPerson(SlR*c,int apply){
    int fmt=slrGet(c); if(c->bad||fmt<1||fmt>14) return 0;
    u8 lk[LK_N]={0}; for(int i=0;i<(fmt>=14?LK_N:fmt>=13?LK_N13:fmt>=12?LK_N12:fmt>=11?LK_N11:fmt>=10?LK_N10:fmt>=9?LK_N9:fmt>=8?LK_N8:fmt>=7?LK_N7:fmt>=6?LK_N6:fmt>=5?LK_N5:fmt>=4?LK_N4:fmt>=3?LK_N3:LK_BASE);i++) lk[i]=(u8)slrGet(c);
    int stg=AG_ADULT, agd=0; if(fmt>=2){ stg=slrGet(c); agd=slrGet(c); }
    if(c->bad||stg>=AG_N) return 0;
    int pa=pAsp, pl=pLtw; u8 pt[TR_N]; for(int i=0;i<TR_N;i++) pt[i]=pTr[i];
    if(fmt>=4){ pa=slrGet(c); pl=slrGet(c); for(int i=0;i<TR_N;i++) pt[i]=(u8)slrGet(c); if(c->bad||!persValid(pa,pl,pt)) return 0; }
    int cu=slrGet(c); if(c->bad) return 0;
    if(lk[LK_TONE]>=9||lk[LK_EARSZ]>=9||lk[LK_EARLF]>=9||lk[LK_SHAPE]>=NSHAPE||lk[LK_SKIN]>=NSKIN||lk[LK_EYES]>=NEYE||lk[LK_MOUTH]>=NMOUTH||lk[LK_BROW]>=6||lk[LK_NOSE]>=6||lk[LK_CHEEK]>=5||lk[LK_GLASS]>=4||lk[LK_EYECOL]>=NSW||lk[LK_EARS]>=3||lk[LK_HSTYLE]>=NHAIR||lk[LK_TAIL]>=3||lk[LK_HORNS]>=3||lk[LK_BACK]>=3||lk[LK_HAT]>=6||lk[LK_HATCOL]>=6||lk[LK_BEARD]>=3||lk[LK_TOPSTY]>=5||lk[LK_BOTSTY]>=4||lk[LK_SHOE]>=6||lk[LK_HCOL]>=NSW||lk[LK_TOP]>=NSW||lk[LK_BOT]>=NSW) return 0;
    for(int i=LK_HEIGHT;i<=LK_MOUTHHT;i++) if(lk[i]>=9) return 0;
    for(int i=LK_HTONE;i<=LK_STANCE;i++) if(lk[i]>=9) return 0;
    if(lk[LK_BUTT]>=9||lk[LK_BUTTH]>=9||lk[LK_BUTTW]>=9||lk[LK_LEGW]>=9||lk[LK_ARMW]>=9||lk[LK_ANTLEN]>=9||lk[LK_ANTSPR]>=9||lk[LK_ANTTIP]>=9||lk[LK_TAILLEN]>=9||lk[LK_HORNSZ]>=9||lk[LK_TAILCURL]>=9||lk[LK_TAILTHK]>=9||lk[LK_HORNSPR]>=9||lk[LK_HORNCRV]>=9||lk[LK_HORNHT]>=9||lk[LK_EARFWD]>=9||lk[LK_EARSPR]>=9||lk[LK_HEADSZ]>=9||lk[LK_HANDFT]>=9||lk[LK_WINGSZ]>=9||lk[LK_TAILTIP]>=7) return 0;
    for(int i=LK_NECK;i<=LK_EARWID;i++) if(lk[i]>=9) return 0;
    for(int i=LK_CHESTW;i<=LK_HORNTONE;i++) if(lk[i]>=9) return 0;
    for(int i=LK_TAILTAPER;i<=LK_WINGTONE;i++) if(lk[i]>=9) return 0;
    for(int i=LK_JAWW;i<=LK_NECKW;i++) if(lk[i]>=9) return 0;
    if(lk[LK_CLAWS]>=4||lk[LK_ANTENNA]>=3||lk[LK_PATTERN]>=7||lk[LK_PATCOL]>=6) return 0;
    if(lk[LK_FEARS]>=5||lk[LK_MUZZLE]>=4||lk[LK_FTAIL]>=4) return 0;
    if(cu>1) return 0;
    if(cu){
        if(!slDecPlane(c,0)||!slDecPlane(c,1)||!slDecPlane(c,2)) return 0;
        for(int i=0;i<CNV;i++){
            int vx=slTmp[0][i], dc=slTmp[1][i]|(slTmp[2][i]<<8);
            if((vx&15)>8||(vx>>4)>11) return 0;                       // a colour slot and a shape that exist
            if(dc&&(decSpr(dc)<1||decSpr(dc)>SLOT_NSPR||(dc>>15))) return 0;  // a face sprite that exists
        }
    }
    if(apply){
        for(int i=0;i<LK_N;i++) look[i]=lk[i];
        pAsp=(u8)pa; pLtw=(u8)pl; for(int i=0;i<TR_N;i++) pTr[i]=pt[i]; persSave();
        stage=(u8)stg; ageDays=(u8)agd; fixLook(); ageSave();
        buildLook();                                                   // also sets sty[] and custom=0
        if(cu){ u8*v=&vox[0][0][0]; u16*d=&dec[0][0][0]; for(int i=0;i<CNV;i++){ v[i]=slTmp[0][i]; d[i]=(u16)(slTmp[1][i]|(slTmp[2][i]<<8)); } custom=1; }
        setColors();
    }
    return 1;
}
// ---------- chunks: LIFE (the life block straight from SRAM, SIM3 or the older SIM2) ----------
static void slEncLife(SlW*w){ int n=simsVer(SIM_SRAM)==2?SIM_BLOCK2:SIM_BLOCK; slwPut(w,1); for(int i=0;i<n;i++) slwPut(w,SIM_SRAM[i]); }   // SRAM may still hold an older SIM2 life
static int slDecLife(SlR*c,int apply){
    if(c->len<1+4||c->p[0]!=1) return 0;
    volatile unsigned char*b=(volatile unsigned char*)(c->p+1);
    int v=simsVer(b); if(c->len!=1+(v==3?SIM_BLOCK:SIM_BLOCK2)) return 0;   // SIM3 = 52 bytes, SIM2 = 24 bytes
    if(!simsCheck(b)) return 0;
    if(apply){ simsUnpack(b); fxGhostClear(); simsSaveNow(); }   /* a life that is loaded brings its own ghosts (savegame.h 'G') or none */
    return 1;
}
static void slChunk(SlW*w,int tag,void(*enc)(SlW*)){   // a chunk: its length first (measured by a dry run), then the data
    SlW t; slwInit(&t,0,1<<20); enc(&t);
    slwPut(w,tag); slwPut16(w,t.pos); enc(w);
}
static int slBuild(SlW*w,int mask){
    if(mask&SLH_ROOM) slChunk(w,SLC_ROOM,slEncRoom);
    if(mask&SLH_PERSON) slChunk(w,SLC_PERSON,slEncPerson);
    if(mask&SLH_LIFE) slChunk(w,SLC_LIFE,slEncLife);
    slwPut(w,0);   // end
    return w->pos;
}

// ---------- header, scan ----------
typedef struct { u8 ok,kind,span,has,mw,mh; u16 len,sum,seq; char name[SLOT_NAME+1]; u8 pid; } SlInfo;   // pid: which player a save file belongs to
static SlInfo slI[SLOT_MAX] EWRAM_BSS; static s8 slOwner[SLOT_MAX] EWRAM_BSS; static u8 slGood[SLOT_MAX] EWRAM_BSS;   // per slot: header, which head covers it (-1 none), payload checksum ok
#define SLB(s) svPtr(SLO(s))   // read pointer (save.h)
// A save of span slots must fit, and must not run over the 64 KB bank boundary of a 128 KB chip (read pointers stay in one bank)
static int slFits(int slot,int span){ return slot>=0&&span>=1&&slot+span<=SLOT_N&&(SLO(slot)>>16)==((SLO(slot+span)-1)>>16); }
static void slOpen(int slot,int span){ svErr=0; svErase(SLO(slot),(u32)span*SLOT_SZ); }   // erased = empty (reads 0xFF) until the header goes in last
static int slVerify(int slot);
static int slInfo(int slot,SlInfo*I){
    volatile u8*m=SLB(slot); I->ok=0;
    if(m[0]!='S'||m[1]!='V'||m[2]!=1) return 0;
    u8 hs=0x5B; for(int i=0;i<31;i++) hs=(u8)(hs+m[i]); if(m[31]!=hs) return 0;
    I->kind=m[3]; I->span=m[4]; I->has=m[5]; I->len=(u16)(m[6]|(m[7]<<8)); I->sum=(u16)(m[8]|(m[9]<<8)); I->mw=m[10]; I->mh=m[11]; I->seq=(u16)(m[12]|(m[13]<<8)); I->pid=m[28];
    int e=SLOT_NAME; for(int i=0;i<SLOT_NAME;i++){ char c=(char)m[16+i]; I->name[i]=((c>='A'&&c<='Z')||(c>='0'&&c<='9'))?c:' '; }
    while(e>0&&I->name[e-1]==' ') e--;
    I->name[e]=0;
    if(I->kind>SLK_PLAYER||!slFits(slot,I->span)||I->len>I->span*SLOT_SZ-SLOT_HDR) return 0;
    I->ok=1; return 1;
}
static void slScan(void){
    for(int i=0;i<SLOT_N;i++){ slOwner[i]=-1; slGood[i]=0; slI[i].ok=0; }
    for(int i=0;i<SLOT_N;){
        SlInfo*I=&slI[i];
        if(slInfo(i,I)){ slGood[i]=(u8)(slSumOf(SLB(i)+SLOT_HDR,I->len)==I->sum);
            for(int k=0;k<I->span;k++) slOwner[i+k]=(s8)i;
            i+=I->span; }
        else i++;
    }
}
static int slActive(void){   // the active slot, or -1
    volatile u8*m=SRAM_BASE+SLOT_DIR;
    if(m[0]!='S'||m[1]!='D'||m[2]>=SLOT_N||m[3]!=(u8)(0x5D+m[2])) return -1;
    SlInfo I; if(!slInfo(m[2],&I)) return -1;
    return m[2];
}
static void slSetActive(int s){ volatile u8*m=SRAM_BASE+SLOT_DIR; m[0]='S'; m[1]='D'; m[2]=(u8)s; m[3]=(u8)(0x5D+s); }
static void slHdrWrite(int slot,const u8*m){ u32 o=SLO(slot); for(int i=2;i<SLOT_HDR;i++) svWr(o+i,m[i]); svWr(o,m[0]); svWr(o+1,m[1]); }   // the magic goes in last
static void slHeader(int slot,int kind,int span,int has,int len,int sum,int seq,const char*name){
    u8 m[SLOT_HDR];
    for(int i=0;i<SLOT_HDR;i++) m[i]=0;
    m[2]=1; m[3]=(u8)kind; m[4]=(u8)span; m[5]=(u8)has; m[6]=(u8)(len&255); m[7]=(u8)(len>>8); m[8]=(u8)(sum&255); m[9]=(u8)(sum>>8);
    m[10]=MW; m[11]=MH; m[12]=(u8)(seq&255); m[13]=(u8)(seq>>8); m[28]=slHPid; slHPid=0;
    for(int i=0;i<SLOT_NAME;i++){ char c=name[i]; if(!c){ for(;i<SLOT_NAME;i++) m[16+i]=' '; break; } m[16+i]=(u8)c; }
    m[0]='S'; m[1]='V';                                   // (written last by slHdrWrite: until then the slot reads as empty)
    u8 hs=0x5B; for(int i=0;i<31;i++) hs=(u8)(hs+m[i]); m[31]=hs;
    slHdrWrite(slot,m);
}
static void slNameN(char*d,const char*pre,int n){ while(*pre) *d++=*pre++; if(n>=10) *d++=(char)('0'+n/10); *d++=(char)('0'+n%10); *d=0; }
static void slDefaultName(int slot,char*d){ slNameN(d,"ROOM ",slot+1); }

// ---------- save / load / delete / copy / rename ----------
static int slMaskOpt(void){ static const u8 t[3]={SLH_ROOM,SLH_ROOM|SLH_PERSON,SLH_ROOM|SLH_PERSON|SLH_LIFE}; return t[xo[XO_SLOTCONT]]; }
// name = 0: keep the name the slot has (or ROOM n for an empty slot). Returns SLE_OK or an error and leaves the slot as it was on SLE_BIG.
static int slSaveI(int slot,int mask,const char*name){
    SlInfo old; int had=slInfo(slot,&old); char nm[SLOT_NAME+1];
    if(name) { int i=0; for(;name[i]&&i<SLOT_NAME;i++) nm[i]=name[i]; nm[i]=0; }
    else if(had&&old.kind==SLK_ROOM){ for(int i=0;i<=SLOT_NAME;i++) nm[i]=old.name[i]; }
    else slDefaultName(slot,nm);
    if(had&&old.kind!=SLK_ROOM) return SLE_HOUSE;           // a house or a household slot is never overwritten by a room
    if((mask&SLH_LIFE)&&!simsCheck(SIM_SRAM)) mask&=~SLH_LIFE;       // no life has been played yet: nothing to store
    SlW d; slwInit(&d,0,1<<20); slBuild(&d,mask);
    if(d.pos>SLOT_SZ-SLOT_HDR) return SLE_BIG;                       // checked BEFORE touching the slot
    slOpen(slot,1);                                                  // empty while we write
    SlW w; slwInit(&w,SLO(slot)+SLOT_HDR,SLOT_SZ-SLOT_HDR); slBuild(&w,mask);
    slHeader(slot,SLK_ROOM,1,mask,w.pos,slwSum(&w),had?old.seq+1:1,nm);
    return slVerify(slot);
}
static int slSave(int slot,int mask,const char*name){   // flash writes are slow: a loading screen covers them
    ldShow("SAVING",0,2); int e=slSaveI(slot,mask,name); ldShow("SAVING",2,2); ldEnd(); return e;
}
static int slVerify(int slot){   // read it back: no save memory here?
    SlInfo chk; if(svErr||!slInfo(slot,&chk)||slSumOf(SLB(slot)+SLOT_HDR,chk.len)!=chk.sum) return SLE_NOSRAM;
    return SLE_OK;
}
static int slLoadedMask;   // what the last slLoad really changed
static int slParse(volatile u8*body,int len,int mask,int apply){
    SlR r={body,0,len,0};
    while(r.pos<len){
        int tag=slrGet(&r); if(tag==0) break;
        int cl=slrGet16(&r); if(r.bad||r.pos+cl>len) return SLE_FMT;
        SlR c={body+r.pos,0,cl,0}; r.pos+=cl;
        if(tag==SLC_ROOM&&(mask&SLH_ROOM)){ if(!slDecRoom(&c,apply)) return SLE_FMT; if(apply) slLoadedMask|=SLH_ROOM; }
        else if(tag==SLC_PERSON&&(mask&SLH_PERSON)){ if(!slDecPerson(&c,apply)) return SLE_FMT; if(apply) slLoadedMask|=SLH_PERSON; }
        else if(tag==SLC_LIFE&&(mask&SLH_LIFE)){ if(!slDecLife(&c,apply)) return SLE_FMT; if(apply) slLoadedMask|=SLH_LIFE; }
        // SLC_HOUSE and any tag we do not know: skipped on purpose (see the HOUSES note at the top)
    }
    return SLE_OK;
}
// ---------- houses: every floor as its own room chunk (index 0..FLR_N-1), then a small plan chunk (version, floor count) ----------
static int slHouseBuild(SlW*w){
    for(int f=0;f<FLR_N;f++){ slSrcF=f; slRoomIdx=f; slChunk(w,SLC_ROOM,slEncRoom); }
    slSrcF=-1; slRoomIdx=0;
    slwPut(w,SLC_HOUSE); slwPut16(w,2); slwPut(w,1); slwPut(w,FLR_N);
    slwPut(w,0); return w->pos;
}
static int houseLoad(int slot){   // all floors are checked first; only then is anything replaced. You end up on the ground floor
    SlInfo I; slLoadedMask=0; if(!slInfo(slot,&I)||I.kind!=SLK_HOUSE) return SLE_EMPTY;
    volatile u8*b=SLB(slot)+SLOT_HDR;
    if(slSumOf(b,I.len)!=I.sum) return SLE_BAD;
    if(I.mw!=MW||I.mh!=MH) return SLE_SIZE;
    for(int f=0;f<FLR_N;f++){ slWantRoom=f; int e=slParse(b,I.len,SLH_ROOM,0); if(e){ slWantRoom=0; return e; } }
    flEnsure(); flHome(); flClear();   // (every stored floor goes first: the new house always fits, whatever the old one took)
    for(int f=0;f<FLR_N;f++){ slWantRoom=f; flBlankLive(); slParse(b,I.len,SLH_ROOM,1); flStoreAs(f); }   // (a floor the save lacks stays empty)
    slWantRoom=0; flLoad(0); curFl=0;
    mapSave(); mapScan(); hhSlotsFree(); liveInvalidate(); camSnap=1;
    return SLE_OK;
}
static int houseSave(int slot,const char*name){   // needs a fresh slScan (slOwner). A house takes 3 or 4 slots in a row
    SlInfo old; int had=slInfo(slot,&old); if(had&&old.kind!=SLK_HOUSE) return SLE_HOUSE;
    flEnsure();   // (the floor you are on is read live: slPlaneVal)
    SlW d; slwInit(&d,0,1<<20); slHouseBuild(&d);
    int span=(SLOT_HDR+d.pos+SLOT_SZ-1)/SLOT_SZ; if(span>4) return SLE_BIG;
    if(!slFits(slot,span)) return SLE_NOROOM;
    for(int k=0;k<span;k++) if(slOwner[slot+k]>=0&&slOwner[slot+k]!=slot) return SLE_NOROOM;
    char nm[SLOT_NAME+1];
    if(name){ int i=0; for(;name[i]&&i<SLOT_NAME;i++) nm[i]=name[i]; nm[i]=0; }
    else if(had){ for(int i=0;i<=SLOT_NAME;i++) nm[i]=old.name[i]; }
    else slNameN(nm,"HOUSE ",slot+1);
    int oldspan=had?old.span:1;
    slOpen(slot,span);
    SlW w; slwInit(&w,SLO(slot)+SLOT_HDR,span*SLOT_SZ-SLOT_HDR); slHouseBuild(&w);
    if(w.over) return SLE_BIG;
    slHeader(slot,SLK_HOUSE,span,SLH_ROOM,w.pos,slwSum(&w),had?old.seq+1:1,nm);
    for(int k=span;k<oldspan&&slot+k<SLOT_N;k++){ svWr(SLO(slot+k),0); svWr(SLO(slot+k)+1,0); }
    return slVerify(slot);
}
static int slLoad(int slot,int mask){
    SlInfo I; slLoadedMask=0;
    if(!slInfo(slot,&I)) return SLE_EMPTY;
    if((I.kind==SLK_HOUSE||(mask&SLH_ROOM))&&nbBarred()) return SLE_VISIT;   // loading a room or a house over a community lot you are visiting would be building it
    if(I.kind!=SLK_ROOM) return (I.kind==SLK_HOUSE&&SLOT_HOUSE_READY)?houseLoad(slot):SLE_HOUSE;   // (a household slot loads with slLoadHH)
    if(slSumOf(SLB(slot)+SLOT_HDR,I.len)!=I.sum) return SLE_BAD;
    if(I.mw!=MW||I.mh!=MH) return SLE_SIZE;
    mask&=I.has; if(!mask) return SLE_NOPART;
    int e=slParse(SLB(slot)+SLOT_HDR,I.len,mask,0); if(e) return e;      // pass 1: check everything, change nothing
    if(mask&SLH_ROOM) flHome();                                         // (back to the ground floor first)
    e=slParse(SLB(slot)+SLOT_HDR,I.len,mask,1); if(e) return e;          // pass 2: apply
    if(slLoadedMask&SLH_ROOM){ flBlankUpper(); mapSave(); mapScan(); }   // a single room replaces the house: the upper floors are empty again                  // the loaded room becomes the current room
    slSetActive(slot);
    return SLE_OK;
}
// ---------- household slots: the Sims living with you, kept like a room slot (SAVE HOUSEHOLD / LOAD HOUSEHOLD in the slot's menu) ----------
// slSaveHH stores the household block that hhSave last wrote to SRAM (call hhSave first); slLoadHH puts a stored one back there (call hhLoad after).
#define SL_HHBLK (SRAM_BASE+SL_HH_OFF)
static int slSaveHHI(int slot,const char*name){   // name 0: keep the slot's name (or FAMILY n). Needs slScan to be fresh (slOwner).
    SlInfo old; int had=slInfo(slot,&old); if(had&&old.kind!=SLK_HHOLD) return SLE_HOUSE;
    volatile u8*b=SL_HHBLK; int len=hhBlockLen(b,SL_HH_LEN); if(!len) return SLE_BAD;
    int span=(SLOT_HDR+len+SLOT_SZ-1)/SLOT_SZ;
    if(!slFits(slot,span)) return SLE_NOROOM;
    for(int k=0;k<span;k++) if(slOwner[slot+k]>=0&&slOwner[slot+k]!=slot) return SLE_NOROOM;   // the slots it runs into must be free (or its own)
    char nm[SLOT_NAME+1];
    if(name){ int i=0; for(;name[i]&&i<SLOT_NAME;i++) nm[i]=name[i]; nm[i]=0; }
    else if(had){ for(int i=0;i<=SLOT_NAME;i++) nm[i]=old.name[i]; }
    else slNameN(nm,"FAMILY ",slot+1);
    int oldspan=had?old.span:1;
    slOpen(slot,span);                                               // empty while we write
    for(int i=0;i<len;i++) svWr(SLO(slot)+SLOT_HDR+(u32)i,b[i]);
    int sum=slSumOf(b,len);
    slHeader(slot,SLK_HHOLD,span,0,len,sum,had?old.seq+1:1,nm);
    for(int k=span;k<oldspan&&slot+k<SLOT_N;k++){ svWr(SLO(slot+k),0); svWr(SLO(slot+k)+1,0); }   // it shrank: free the slot it no longer needs
    return slVerify(slot);
}
static int slSaveHH(int slot,const char*name){
    ldShow("SAVING THE HOUSEHOLD",0,2); int e=slSaveHHI(slot,name); ldShow("SAVING THE HOUSEHOLD",2,2); ldEnd(); return e;
}
static int slLoadHH(int slot){   // checks the stored household first and only then replaces the one in SRAM
    SlInfo I; if(!slInfo(slot,&I)||I.kind!=SLK_HHOLD) return SLE_EMPTY;
    volatile u8*s=SLB(slot)+SLOT_HDR;
    if(slSumOf(s,I.len)!=I.sum) return SLE_BAD;
    int len=hhBlockLen(s,I.len); if(!len) return SLE_FMT;
    volatile u8*d=SL_HHBLK; for(int i=0;i<len;i++) d[i]=s[i];
    svCommit(); hhLoad();   // hhM and the relationships now match SRAM again (so the next SAVE HOUSEHOLD cannot write an old one)
    return SLE_OK;
}
static void slDelete(int slot){
    svWr(SLO(slot),0); svWr(SLO(slot)+1,0);                          // (on flash: bits only go to 0, no erase needed)
    volatile u8*d=SRAM_BASE+SLOT_DIR; if(d[0]=='S'&&d[2]==(u8)slot) d[0]=0;   // it was the active slot: there is none now
    svCommit();
}
static void slEraseAll(void){ ldShow("ERASING THE SLOTS",0,2); svErase(SLOT_BASE,(u32)SLOT_N*SLOT_SZ); volatile u8*d=SRAM_BASE+SLOT_DIR; for(int i=0;i<4;i++) d[i]=0; svCommit(); ldShow("ERASING THE SLOTS",2,2); ldEnd(); }
static int slCopy(int src,int dst){   // span 1 only; byte for byte, magic last
    SlInfo I; if(!slInfo(src,&I)) return SLE_EMPTY; if(I.span!=1) return SLE_HOUSE;
    int n=SLOT_HDR+I.len; u32 a=SLO(src), b=SLO(dst);
    slOpen(dst,1); for(int i=2;i<n;i++) svWr(b+(u32)i,svRd(a+(u32)i)); svWr(b,'S'); svWr(b+1,'V');
    return slVerify(dst);
}
static void slRename(int slot,const char*name){
    SlInfo I; if(!slInfo(slot,&I)) return; u8 m[SLOT_HDR]; for(int i=0;i<SLOT_HDR;i++) m[i]=svRd(SLO(slot)+(u32)i);
    for(int i=0;i<SLOT_NAME;i++){ char c=name[i]; if(!c){ for(;i<SLOT_NAME;i++) m[16+i]=' '; break; } m[16+i]=(u8)c; }
    u8 hs=0x5B; for(int i=0;i<31;i++) hs=(u8)(hs+m[i]); m[31]=hs;
    svErase(SLO(slot),SLOT_HDR); slHdrWrite(slot,m);   // (flash: the header is erased on its own; the rest of the sector is kept)
}
static int slotSyncActive(void){   // SAVE MAP TO SLOT: write the room into the active slot too
    int a=slActive(); if(a<0) return 0;
    return slSave(a,slMaskOpt(),0)==SLE_OK;
}
static void slotBoot(void){        // BOOT LOADS PERSON: the creature is not kept anywhere else, so bring it back from the active slot
    if(!xo[XO_SLOTBOOT]) return;
    int a=slActive(); if(a>=0) slLoad(a,SLH_PERSON);
}
static int slotUsedBytes(void){ int n=0; for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i) n+=SLOT_HDR+slI[i].len; return n; }
static const char* slErrMsg(int e){
    switch(e){ case SLE_BIG: return "TOO BIG FOR A SLOT"; case SLE_BAD: return "SLOT IS DAMAGED"; case SLE_EMPTY: return "SLOT IS EMPTY";
        case SLE_HOUSE: return "WRONG KIND OF SLOT"; case SLE_NOROOM: return "NOT ENOUGH FREE SLOTS"; case SLE_SIZE: return "MAP SIZE DOES NOT MATCH"; case SLE_NOPART: return "THAT PART IS NOT SAVED";
        case SLE_NOSRAM: return "SAVE NOT SUPPORTED HERE"; case SLE_VISIT: return "NO BUILDING WHILE VISITING"; default: return "COULD NOT READ SLOT"; }
}

// ---------- the screen ----------
static char* slCat(char*d,const char*s){ while(*s) *d++=*s++; *d=0; return d; }
static char* slNum(char*d,int n){ char t[8]; int k=0; if(n<=0) t[k++]='0'; while(n>0&&k<7){ t[k++]=(char)('0'+n%10); n/=10; } while(k>0) *d++=t[--k]; *d=0; return d; }
static char* slSize(char*d,int n){   // 812B  or  1.4K
    if(n<1000) { d=slNum(d,n); return slCat(d,"B"); }
    d=slNum(d,n/1024); *d++='.'; d=slNum(d,(n%1024)*10/1024); return slCat(d,"K");
}
static u16 bfsQ[MH*MW] EWRAM_BSS;   // (main.c: the search queue, free while a slot is previewed)
#define slTb ((u8*)bfsQ)            // the tile plane of the room being previewed (format 3)
static u16 slThumbCol(int t,int f){ if(t=='.') return shade(flFlat[f%NFL][0],10); int pi=palIdx((char)t); return pi>=0?palCol[pi]:0; }
static void slThumb(int slot,int x0,int y0){   // the room of a slot at 1 pixel per tile, decoded straight from SRAM (all room formats)
    rect(x0-1,y0-1,MW+2,MH+2,RGB(3,4,7)); rect(x0,y0,MW,MH,RGB(6,7,10));
    SlInfo*I=&slI[slot]; if(!I->ok||!slGood[slot]||(I->kind!=SLK_ROOM&&I->kind!=SLK_HOUSE)||!(I->has&SLH_ROOM)) return;
    SlR r={SLB(slot)+SLOT_HDR,0,I->len,0};
    while(r.pos<r.len){
        int tag=slrGet(&r); if(!tag) break; int cl=slrGet16(&r); if(r.bad) return;
        if(tag!=SLC_ROOM){ r.pos+=cl; continue; }
        SlR c={r.p+r.pos,0,cl,0}; int idx=slrGet(&c), fmt=slrGet(&c); if(idx!=0||c.bad) return;
        int i=0;
        if(fmt==3){   // tiles first (kept), then the floors drawn with the tiles on top
            while(i<MSZ){ int n=slrGet(&c), t=slrGet(&c); if(c.bad||n==0) return; for(int k=0;k<n&&i<MSZ;k++) slTb[i++]=(u8)t; }
            i=0;
            while(i<MSZ){ int n=slrGet(&c), f=slrGet(&c); if(c.bad||n==0) return; for(int k=0;k<n&&i<MSZ;k++,i++) px(x0+i%MW,y0+i/MW,slThumbCol(slTb[i],f)); }
            return;
        }
        if(fmt!=1&&fmt!=2) return;
        while(c.pos<c.len&&i<MSZ){
            int n=slrGet(&c), t=slrGet(&c), f=slrGet(&c); if(c.bad) return; if(fmt==2) slrGet(&c); else f>>=4;
            u16 col=slThumbCol(t,f);
            for(int k=0;k<n&&i<MSZ;k++,i++) px(x0+i%MW,y0+i/MW,col);
        }
        return;
    }
}
#define SL_ROWS 6
static int slLocked(int i){ int o=slOwner[i]; return o>=0&&(slI[o].kind==SLK_PLAYER||slI[o].kind==SLK_TOWN); }   // saves that SAVE GAME and the town keep for you: shown, never picked
static int slTop(int sel){ int t=sel-2; if(t>SLOT_N-SL_ROWS) t=SLOT_N-SL_ROWS; if(t<0) t=0; return t; }   // first slot shown: the cursor sits near the middle of the six rows
static void slDraw(int sel){
    fillCols(0,ROW_W,RGB(3,4,8));
    box(3,1,234,157); text(12,6,"BLUEPRINTS",GOLD,1);
    int act=slActive();
    text(100,6,act>=0?"ACTIVE SLOT":"NO ACTIVE SLOT",DIMC,1); if(act>=0){ char nb[4]; slNum(nb,act+1); text(160,6,nb,GOLD,1); }
    int top=slTop(sel);
    if(SLOT_N>SL_ROWS){ rect(152,18,2,SL_ROWS*15-3,RGB(5,6,10)); rect(152,18+top*(SL_ROWS*15-3)/SLOT_N,2,SL_ROWS*(SL_ROWS*15-3)/SLOT_N,DIMC); }   // where the window is in the list
    for(int i=top;i<top+SL_ROWS&&i<SLOT_N;i++){
        int y=18+(i-top)*15; SlInfo*I=&slI[i];
        if(i==sel){ rect(8,y-2,142,14,RGB(6,16,8)); rect(8,y-2,2,14,GOLD); }
        char nb[4]; slNum(nb,i+1); text(13,y+2,nb,i==sel?GOLD:DIMC,1);
        u16 nc=i==sel?WHITE:DIMC;
        if(slLocked(i)){ text(28,y,slI[slOwner[i]].kind==SLK_TOWN?"THE TOWN":"PLAYER SAVE",RGB(9,11,15),1); text(28,y+7,"KEPT BY SAVE GAME",RGB(7,9,13),1); }
        else if(slOwner[i]<0) text(28,y,"EMPTY",i==sel?RGB(20,23,26):RGB(9,11,15),1);
        else if(slOwner[i]!=i) text(28,y,"PART OF A BIG SAVE",RGB(14,16,22),1);
        else {
            text(28,y,I->name[0]?I->name:"NO NAME",slGood[i]?nc:RGB(30,10,8),1);
            char b[40]; char*e=b; *e=0;
            if(!slGood[i]) e=slCat(e,"DAMAGED");
            else if(I->kind==SLK_HOUSE) e=slCat(e,"HOUSE");
            else if(I->kind==SLK_TOWN) e=slCat(e,"NEIGHBORHOOD");
            else if(I->kind==SLK_PLAYER) e=slCat(e,"PLAYER SAVE");
            else if(I->kind==SLK_HHOLD){ e=slCat(e,"HOUSEHOLD "); e=slNum(e,SLB(i)[SLOT_HDR+2]+1); e=slCat(e," SIMS"); *e=0; }
            else { if(I->has&SLH_ROOM) e=slCat(e,"ROOM "); if(I->has&SLH_PERSON) e=slCat(e,"PERSON "); if(I->has&SLH_LIFE) e=slCat(e,"LIFE "); }
            if(I->kind!=SLK_HHOLD&&I->kind!=SLK_TOWN){ e=slCat(e," "); slSize(e,SLOT_HDR+I->len); }
            text(28,y+7,b,i==sel?RGB(22,25,28):RGB(11,13,18),1);
            if(act==i) text(116,y,"ACTIVE",GOLD,1);
        }
    }
    // right: the room of the slot under the cursor
    slThumb(sel,166,20);
    if(slOwner[sel]==sel&&slGood[sel]){
        SlInfo*I=&slI[sel]; char b[24]; char*e=slCat(b,"SAVED "); e=slNum(e,I->seq); slCat(e,I->seq==1?" TIME":" TIMES");
        text(158,64,b,DIMC,1);
        text(158,72,I->kind==SLK_HOUSE?"A HOUSE":I->kind==SLK_HHOLD?"A HOUSEHOLD":I->kind==SLK_TOWN?"THE TOWN":I->kind==SLK_PLAYER?"A PLAYER":"ONE ROOM",DIMC,1);
    } else if(slOwner[sel]<0) text(158,64,"A FREE SLOT",DIMC,1);
    // bottom: how full the slots are
    int used=slotUsedBytes(), tot=SLOT_N*SLOT_SZ, cnt=0; for(int i=0;i<SLOT_N;i++) if(slOwner[i]>=0) cnt++;
    char b[40]; char*e=slNum(b,cnt); e=slCat(e," OF "); e=slNum(e,SLOT_N); e=slCat(e," SLOTS USED   "); e=slSize(e,used); e=slCat(e," OF "); slSize(e,tot);
    text(12,112,b,DIMC,1); rect(12,121,150,4,RGB(8,10,14)); rect(12,121,used*150/tot,4,GOLD);
    text(12,132,"UP DOWN PICK A SLOT   A OPTIONS",WHITE,1);
    text(12,141,"B BACK",DIMC,1);
    { char cb[32]; slCat(slCat(cb,"SAVE CHIP  "),svName()); text(12,150,cb,RGB(12,14,16),1); }
}

static const char slChars[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static int slEditName(char*nm){   // nm holds SLOT_NAME letters/digits/spaces and a 0. Returns 1 if the player accepted it.
    char b[SLOT_NAME+1]; int cur=0; u16 prev=keyNow();
    { int i=0; for(;i<SLOT_NAME&&nm[i];i++) b[i]=nm[i]; for(;i<SLOT_NAME;i++) b[i]=' '; b[SLOT_NAME]=0; }
    const int NC=(int)sizeof(slChars)-1;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        int ci=0; for(int i=0;i<NC;i++) if(slChars[i]==b[cur]) ci=i;
        if(pr&K_UP) b[cur]=slChars[(ci+1)%NC];
        if(pr&K_DOWN) b[cur]=slChars[(ci+NC-1)%NC];
        if(pr&K_LEFT) cur=(cur+SLOT_NAME-1)%SLOT_NAME;
        if(pr&K_RIGHT) cur=(cur+1)%SLOT_NAME;
        if(pr&K_SEL) for(int i=0;i<SLOT_NAME;i++) b[i]=' ';
        if(pr&K_B) return 0;
        if(pr&(K_A|K_START)){ int e=SLOT_NAME; while(e>0&&b[e-1]==' ') e--; if(e==0) continue; for(int i=0;i<SLOT_NAME;i++) nm[i]=b[i]; nm[SLOT_NAME]=0; return 1; }
        box(20,44,200,66); text(28,50,"NAME THIS SLOT",GOLD,1);
        for(int i=0;i<SLOT_NAME;i++){ char s[2]={b[i],0}; int x=28+i*17; rect(x-1,66,15,18,i==cur?RGB(6,16,8):RGB(5,6,10)); if(b[i]!=' ') text(x+2,68,s,WHITE,2); if(i==cur) rect(x,83,13,2,GOLD); }
        text(28,92,"UP DOWN LETTER  LEFT RIGHT MOVE",DIMC,1);
        text(28,100,"A OK  B CANCEL  SELECT CLEARS",DIMC,1);
        present();
    }
}
static const char* const slYesNo[2]={"NO","YES"};
static char slLn[9][30] EWRAM_BSS; static const char* slLp[9];
static void slInfoScreen(int s){
    SlInfo*I=&slI[s]; int n=0; char*e;
    slLn[n][0]='>'; slCat(slLn[n]+1,I->name[0]?I->name:"NO NAME"); n++;
    e=slCat(slLn[n],"KIND   "); slCat(e,I->kind==SLK_HOUSE?"HOUSE":I->kind==SLK_HHOLD?"HOUSEHOLD":I->kind==SLK_TOWN?"NEIGHBORHOOD":I->kind==SLK_PLAYER?"PLAYER SAVE":"ROOM"); n++;
    e=slCat(slLn[n],"SIZE   "); e=slSize(e,SLOT_HDR+I->len); e=slCat(e," OF "); slSize(e,SLOT_SZ*I->span); n++;
    e=slCat(slLn[n],"SAVED  "); e=slNum(e,I->seq); slCat(e,I->seq==1?" TIME":" TIMES"); n++;
    e=slCat(slLn[n],"MAP    "); e=slNum(e,I->mw); e=slCat(e," X "); slNum(e,I->mh); n++;
    e=slCat(slLn[n],"HOLDS  "); if(I->kind==SLK_HHOLD){ e=slNum(e,SLB(s)[SLOT_HDR+2]+1); e=slCat(e," SIMS"); } if(I->has&SLH_ROOM) e=slCat(e,"ROOM "); if(I->has&SLH_PERSON) e=slCat(e,"PERSON "); if(I->has&SLH_LIFE) e=slCat(e,"LIFE "); n++;
    slCat(slLn[n],slGood[s]?"CHECK  OK":"CHECK  DAMAGED"); n++;
    for(int i=0;i<n;i++) slLp[i]=slLn[i];
    helpScreen("SLOT INFO",slLp,n);
}
enum { SA_SAVE, SA_LOADALL, SA_LOADROOM, SA_LOADPERSON, SA_RENAME, SA_COPY, SA_INFO, SA_DELETE, SA_SAVEHH, SA_LOADHH, SA_SAVEHOUSE, SA_DELPL };
// A on a slot: the list of things you can do with it. Returns 1 if a slot was loaded (the caller must restart play).
static int slActions(int s){
    int own=slOwner[s]; SlInfo*I=&slI[s];
    if(own>=0&&own!=s){ toast("PART OF A BIG SAVE"); return 0; }
    int occ=(own==s), good=occ&&slGood[s], house=occ&&I->kind==SLK_HOUSE, hh=occ&&I->kind==SLK_HHOLD, conf=xo[XO_SLOTCONF], changed=0;
    const char*it[9]; int id[9], n=0;
    if(!occ){ it[n]="SAVE HERE"; id[n++]=SA_SAVE; if(dbgOn){ it[n]="SAVE HOUSEHOLD"; id[n++]=SA_SAVEHH; } it[n]="SAVE HOUSE"; id[n++]=SA_SAVEHOUSE; }
    else if(I->kind==SLK_PLAYER){ it[n]="INFO"; id[n++]=SA_INFO; it[n]="DELETE PLAYER"; id[n++]=SA_DELPL; }   // (players are made and loaded on the PLAY screen)
    else if(hh){
        if(good&&dbgOn){ it[n]="LOAD HOUSEHOLD"; id[n++]=SA_LOADHH; }
        if(dbgOn){ it[n]="SAVE HOUSEHOLD"; id[n++]=SA_SAVEHH; }
        if(good&&I->span==1){ it[n]="RENAME"; id[n++]=SA_RENAME; it[n]="COPY TO FREE SLOT"; id[n++]=SA_COPY; }
        it[n]="INFO"; id[n++]=SA_INFO; it[n]="DELETE"; id[n++]=SA_DELETE;
    }
    else {
        if(good){ it[n]="LOAD ALL"; id[n++]=SA_LOADALL;
            if(!house&&(I->has&SLH_ROOM)&&(I->has&~SLH_ROOM)){ it[n]="LOAD ROOM ONLY"; id[n++]=SA_LOADROOM; }
            if(!house&&(I->has&SLH_PERSON)&&(I->has&~SLH_PERSON)){ it[n]="LOAD PERSON ONLY"; id[n++]=SA_LOADPERSON; } }
        if(!house){ it[n]="SAVE OVER IT"; id[n++]=SA_SAVE; } else { it[n]="SAVE HOUSE"; id[n++]=SA_SAVEHOUSE; }
        if(good){ it[n]="RENAME"; id[n++]=SA_RENAME; if(!house){ it[n]="COPY TO FREE SLOT"; id[n++]=SA_COPY; } }
        it[n]="INFO"; id[n++]=SA_INFO; it[n]="DELETE"; id[n++]=SA_DELETE;
    }
    int c=menu(occ?(I->name[0]?I->name:"NO NAME"):"EMPTY SLOT",it,n); if(c<0) return 0;
    switch(id[c]){
        case SA_SAVE:{
            if(occ&&conf&&menu("OVERWRITE THIS SLOT",slYesNo,2)!=1) break;
            int e=slSave(s,slMaskOpt(),0);
            if(e) toast(slErrMsg(e)); else { slSetActive(s); toast("SLOT SAVED"); }
        } break;
        case SA_SAVEHOUSE:{
            if(occ&&conf&&menu("OVERWRITE THIS SLOT",slYesNo,2)!=1) break;
            int e=houseSave(s,0);
            toast(e?slErrMsg(e):"HOUSE SAVED");
        } break;
        case SA_SAVEHH:{
            if(occ&&conf&&menu("OVERWRITE THIS SLOT",slYesNo,2)!=1) break;
            hhSave();   // the household as it is right now
            int e=slSaveHH(s,0);
            toast(e?slErrMsg(e):"HOUSEHOLD SAVED");
        } break;
        case SA_LOADHH:{
            if(conf&&menu("REPLACE YOUR HOUSEHOLD",slYesNo,2)!=1) break;
            int e=slLoadHH(s);
            if(e) toast(slErrMsg(e)); else { toast("HOUSEHOLD LOADED"); changed=1; }
        } break;
        case SA_LOADALL: case SA_LOADROOM: case SA_LOADPERSON:{
            if(conf&&menu("LOAD OVER WHAT YOU HAVE",slYesNo,2)!=1) break;
            int m=id[c]==SA_LOADALL?(SLH_ROOM|SLH_PERSON|SLH_LIFE):id[c]==SA_LOADROOM?SLH_ROOM:SLH_PERSON;
            int e=slLoad(s,m);
            if(e) toast(slErrMsg(e)); else { toast("LOADED"); changed=1; }
        } break;
        case SA_RENAME:{ char nm[SLOT_NAME+1]; for(int i=0;i<=SLOT_NAME;i++) nm[i]=I->name[i]; if(slEditName(nm)) slRename(s,nm); } break;
        case SA_COPY:{
            int d=-1; for(int i=0;i<SLOT_N;i++) if(slOwner[i]<0){ d=i; break; }
            if(d<0){ toast("NO FREE SLOT"); break; }
            int e=slCopy(s,d); static char cm[24] EWRAM_BSS; slNum(slCat(cm,"COPIED TO SLOT "),d+1); toast(e?slErrMsg(e):cm);
        } break;
        case SA_INFO: slInfoScreen(s); break;
        case SA_DELPL: if(!conf||menu("DELETE THIS PLAYER",slYesNo,2)==1){ sgDeletePid(I->pid); toast("PLAYER DELETED"); } break;
        case SA_DELETE: if(!conf||menu("DELETE THIS SLOT",slYesNo,2)==1){ slDelete(s); toast("SLOT DELETED"); } break;
    }
    return changed;
}
static const char* const slotHelp[]={
    ">Keep What You Make",
    "Slots are your storage. They hold the rooms",
    "and houses you build, the creatures you make",
    "and the households you gather, so you can come",
    "back to any of them.",
    ">Rooms and Houses",
    "A room slot keeps a single floor, and a house slot",
    "keeps all three. Loading a room makes it the one",
    "you are playing in.",
    ">People and Households",
    "A person slot keeps your creature with its",
    "life, cash and clock. A household slot keeps the",
    "Sims who live with you, along with their",
    "friendships and family ties.",
    ">Saving Your Life",
    "Save Game keeps your life safe. Each player has a",
    "save file of their own, while the town is shared.",
    ">How Much Fits",
    "A cartridge with flash memory holds 58 slots.",
    "Without it, there are 12."};
static int slotScreen(void){   // returns 1 if something was loaded
    int sel=0, dirty=1, changed=0; u16 prev=keyNow();
    slScan();
    { int a=slActive(); if(a>=0) sel=a; }   // open on the slot you used last
    for(int t=0;t<SLOT_N&&slLocked(sel);t++) sel=(sel+1)%SLOT_N;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&K_DOWN){ for(int t=0;t<SLOT_N;t++){ sel=(sel+1)%SLOT_N; if(!slLocked(sel)) break; } dirty=1; }
        if(pr&K_UP){ for(int t=0;t<SLOT_N;t++){ sel=(sel+SLOT_N-1)%SLOT_N; if(!slLocked(sel)) break; } dirty=1; }
        if(pr&(K_B|K_START)) break;
        if(pr&K_A){ changed|=slActions(sel); slScan(); prev=keyNow(); dirty=1; }
        if(dirty){ slDraw(sel); uiPresent(); dirty=0; } else vsync();
    }
    while((~REG_KEYINPUT)&0x3FF) vsync();
    return changed;
}
