// neighborhood.h - THE NEIGHBORHOOD: a town of lots to live in, visit and build on (The Sims 1 neighbourhood screen, with the later games'
// town editing: paint the land, lay roads, plant trees, place and remove lots, seasons and the time of day).
//
// THE TOWN   a 24 x 24 grid of cells, each a terrain (grass, dirt, sand, water, plaza, road) and an optional decoration (tree, pine, bush,
//            flowers, rock, lamp, bench, fountain).
// LOTS       up to 16 rectangles of 4 to 10 cells a side. A cell is 4 x 4 tiles of the room map, so lots run from 16 x 16 to 40 x 40 tiles.
//            RESIDENTIAL (a house to live in; one of them is your HOME) or COMMUNITY (park, skate park, plaza, lounge, old town).
// THE LIVE MAP  the room you play and build always belongs to one lot (nbT.cur). Going to another lot first stores this one in a HOUSE slot
//            (all three floors, named after the lot; slots.h finds a free run), then loads the other lot's slot, or builds its starting layout.
//            The room builder keeps its cursor inside the lot (edX0..edY1); its RESET rebuilds the lot's starting layout.
// MONEY      MOVE IN buys the lot (land + everything built on it) and sells your old home for its value, from the life's cash (sims.h).
// SAVED      in a TOWN slot (slots.h, kind 3), found by its kind; written when you leave the screen and after every lot change.
// HOOK       nbDrawLotModel() draws a lot's building as a small icon. A later version can draw the real house there (every floor of its slot).
//
// Needs before it: the map and floors (lifeMap, floorMap, wallMap, mapGen, gRoom, gBox, gPut, gLine, mapSave, mapScan, flHome, flBlankUpper,
// flEnsure, flPlaneAt, curFl), slots.h, lifeMode, mapEditor, the UI kit (box, menu, toast, helpScreen, text, rect, px, line, disc, present,
// keyNow), sims.h (simsDefaults, simsLoad, simsSaveNow, simMoney).
#define NB_W 24
#define NB_H 24
#define NB_LOTS 16
#define NB_NAME 10
enum { NT_GRASS, NT_DIRT, NT_SAND, NT_WATER, NT_PLAZA, NT_ROAD, NT_N };
enum { DC_NONE, DC_TREE, DC_PINE, DC_BUSH, DC_FLOWER, DC_ROCK, DC_LAMP, DC_BENCH, DC_FOUNTAIN, DC_N };
enum { LKIND_RES, LKIND_COMM };
enum { CT_PARK, CT_SKATE, CT_PLAZA, CT_LOUNGE, CT_OLDTOWN, CT_BOTH, CT_PRISON, CT_ARMS, CT_N };   // CT_BOTH: a park and a skate park in one (what a community flag and a skate flag together make)
static const char* const ntNm[NT_N]={"GRASS","DIRT","SAND","WATER","PLAZA","ROAD"};
static const char* const dcNm[DC_N]={"CLEAR","TREE","PINE","BUSH","FLOWERS","ROCK","LAMP","BENCH","FOUNTAIN"};
static const char* const ctNm[CT_N]={"PARK","SKATE PARK","PLAZA","LOUNGE","OLD TOWN","PARK + SKATE","PRISON","ARMS SHOP"};
static const char* const seasNm[4]={"SPRING","SUMMER","FALL","WINTER"};
static const char* const todNm[3]={"DAY","DUSK","NIGHT"};
typedef struct { u8 on,x,y,w,h,kind,type; s8 slot; char name[NB_NAME+1]; u8 floors; u16 value; } NbLot;   // value: what it sells for, in units of NB_UNIT simoleons
typedef struct { char tag[4]; char name[NB_NAME+1]; u8 season,tod,home,cur,zoom,pad[3]; u8 cell[NB_H][NB_W]; NbLot lot[NB_LOTS]; u8 roof[NB_LOTS][5]; } Town;   // roof (APPENDED, older towns have none: zeros = the old automatic roof): per lot [0] style (low nibble: PYRAMID FLAT HIP SHED) + colour (high nibble: 0 auto, 1-8 palette, 9 custom), [1] height 0 auto / 1-15, [2] overhang 0-7, [3..4] custom RGB555
#define NB_TOWN_V1 ((unsigned)__builtin_offsetof(Town,roof))   // the size of a town saved before roofs existed: still loads, its roofs are all automatic
static Town nbT EWRAM_BSS;
#define NB_UNIT 40   // a lot's saved value is in units of this many simoleons (a u16 then reaches 2.6 million; an old town's numbers just scale up)
static int nbPrice(const NbLot*L){ return (int)L->value*NB_UNIT; }   // what the lot costs / sells for
static int nbWho(int li,char*nm); static int nbLives(int li); static int hhPlayAt(int li); static int hhNewAt(int li);   // households.h: who lives on a lot, playing them, new Sims
static u8 nbOk;                  // nbT holds a town
static int nbTS=-1;              // the slot nbT was loaded from (or -1: a new town, it gets a free slot)
static Town nbTmp EWRAM_BSS;     // another town, read for the chooser's thumbnails
#define NB_ACT pad[0]            // 1 = the live room belongs to this town (its lot cur)
#define NB_GR(c) ((c)&7)
#define NB_DC(c) ((c)>>3)

// ---------- lots and the room map ----------
static void nbRect(const NbLot*L,int*x0,int*y0,int*x1,int*y1){ int tw=L->w*4, th=L->h*4; *x0=(MW-tw)/2; *y0=(MH-th)/2; *x1=*x0+tw-1; *y1=*y0+th-1; }
static void nbBounds(void){   // the room builder's cursor stays on the live lot
    edX0=0; edY0=0; edX1=MW-1; edY1=MH-1;
    if(nbOk&&nbT.cur<NB_LOTS&&nbT.lot[nbT.cur].on) nbRect(&nbT.lot[nbT.cur],&edX0,&edY0,&edX1,&edY1);
}
static u8 nbEditPass;   // 1 only while the town view's BUILD opens a lot: the one way into a community lot's room builder
static int nbBarred(void){   // 1 = the live room is a COMMUNITY lot and you got there by visiting it. Like the Sims, a community lot is built from the neighborhood view, never while you are standing in it:
    // the builder, the pause menu's EDIT MAP, BUILD ROOM in the main menu, the creator's BUILD button and loading a blueprint over it all say no
    return nbOk&&!nbEditPass&&nbT.cur<NB_LOTS&&nbT.lot[nbT.cur].on&&nbT.lot[nbT.cur].kind==LKIND_COMM;
}
static int nbFlagOk(void){ return nbOk&&nbEditPass&&nbT.cur<NB_LOTS&&nbT.lot[nbT.cur].on&&nbT.cur!=nbT.home&&!nbLives(nbT.cur); }   // flags can only be placed while the town view's BUILD has a free lot open (never in your home, never in normal play)
static int nbFlagsOn(void){ return nbOk&&nbT.cur<NB_LOTS&&nbT.lot[nbT.cur].on&&nbT.lot[nbT.cur].kind==LKIND_COMM; }   // flags only do anything on a community lot
static int nbAt(int cx,int cy){ for(int i=0;i<NB_LOTS;i++){ const NbLot*L=&nbT.lot[i]; if(L->on&&cx>=L->x&&cy>=L->y&&cx<L->x+L->w&&cy<L->y+L->h) return i; } return -1; }
static int nbItemValue(char c){   // what a tile of c is worth in simoleons: the catalog price (main.c palPrice); anything that is not in the catalog counts a little
    if(c=='.'||c=='P') return 0;
    int i=palIdx(c); return i<0?50:(int)palPrice[i];
}
static void nbValueLive(int j){   // land (20 simoleons a tile) + everything built on every floor of the live map
    NbLot*L=&nbT.lot[j]; u32 v=(u32)L->w*L->h*8*NB_UNIT; int fl=1;
    flEnsure();
    for(int f=0;f<FLR_N;f++){ int any=0; for(int i=0;i<MSZ;i++){ char c=(char)flPlaneAt(f,0,i); v+=(u32)nbItemValue(c); if(f&&c!='.'&&c!='w') any=1; } if(any) fl=f+1; }
    v=(v+NB_UNIT/2)/NB_UNIT; L->value=(u16)(v>65535?65535:v); L->floors=(u8)fl;
}
static const char* nbErr;
static void prisonBuild(int x0,int y0,int x1,int y1);   // prison.h
static void armsBuild(int x0,int y0,int x1,int y1); static int armsLotGet(void);   // prison.h: the compound of a PRISON lot
static void nbTemplate(int i){   // a lot's starting layout: the land, the street in front, and what its kind brings
    NbLot*L=&nbT.lot[i]; flEnsure(); flHome();
    if(L->kind==LKIND_COMM&&L->type==CT_OLDTOWN) mapGen();
    else {
        int x0,y0,x1,y1; nbRect(L,&x0,&y0,&x1,&y1); wDirty=1;
        for(int y=0;y<MH;y++){ for(int x=0;x<MW;x++){ int e=(x==0||y==0||x==MW-1||y==MH-1), in=x>=x0&&x<=x1&&y>=y0&&y<=y1, ring=!in&&x>=x0-1&&x<=x1+1&&y>=y0-1&&y<=y1+1;
            lifeMap[y][x]=e?'w':'.'; wallMap[y][x]=(u8)(e?13:0); floorMap[y][x]=(u8)(ring?7:11);   // grass all round, a pavement ring at the lot line
            if(!in&&!ring&&!e&&y>y1+1&&y<=y1+3) floorMap[y][x]=12;                                   // the street in front
            if(!in&&!ring&&!e&&((x*7+y*13)%23)==0&&!(y>y1&&y<=y1+3)) lifeMap[y][x]='Z'; }               // planters on the verges
          lifeMap[y][MW]=0; }
        int cx=(x0+x1)/2;
        if(L->kind==LKIND_COMM){
            if(L->type==CT_PARK){ for(int y=y0+2;y<=y1-2;y+=3)for(int x=x0+2;x<=x1-2;x+=4) gPut(x,y,(x/4+y/3)&1?'Z':'N'); gPut(cx-1,(y0+y1)/2,'Y'); gPut(cx+2,(y0+y1)/2,'K'); gPut(x0+1,y0+1,'a'); }   // (a community flag: visitors come and go there)
            else if(L->type==CT_SKATE||L->type==CT_BOTH){ gBox(x0,y0,x1,y1,12); gLine(x0+2,y0+2,x1-2,y0+2,'=',0); gPut(x0+2,y1-3,'1'); gPut(x1-2,y1-3,'3');
                for(int x=x0+3;x<=x1-3;x++) gPut(x,y0+1,'5');
                gPut(cx,(y0+y1)/2,'X'); gPut(cx+1,(y0+y1)/2,'X'); gPut(cx,(y0+y1)/2+1,'9'); gPut(x0+1,(y0+y1)/2,'L'); gPut(x1-1,(y0+y1)/2,'M'); gPut(x0+1,y0+1,'k');   // (a skate flag: a skater starts there)
                if(L->type==CT_BOTH){ gPut(x1-1,y0+1,'a'); for(int x=x0+3;x<=x1-3;x+=5){ gFree(x,y1-1,(x/5)&1?'Z':'N'); } gFree(cx-3,y1-3,'K'); gFree(x1-3,y1-4,'Y'); } }   // + a community flag, benches, planters and a picnic table
            else if(L->type==CT_PLAZA){ gBox(x0,y0,x1,y1,3); for(int x=x0+1;x<=x1-1;x+=3){ gPut(x,y0+1,'Z'); gPut(x,y1-1,'Z'); } gPut(cx-2,(y0+y1)/2,'N'); gPut(cx+2,(y0+y1)/2,'N'); gPut(cx,(y0+y1)/2-2,'K'); }
            else if(L->type==CT_LOUNGE){ int rx0=x0+1, ry0=y0+1, rx1=x1-1, ry1=y1-3; gRoom(rx0,ry0,rx1,ry1,2,NWP+57); gPut(cx,ry1,'D');
                gPut(rx0+1,ry0+1,'V'); gPut(rx1-1,ry0+1,'V'); gPut(rx0+2,ry0+2,'U'); gPut(rx1-2,ry0+2,'U'); gPut(cx,ry0+2,'G'); gPut(rx0+1,ry1-1,'C'); gPut(rx1-1,ry1-1,'C'); }
            else if(L->type==CT_PRISON) prisonBuild(x0,y0,x1,y1);
            else if(L->type==CT_ARMS) armsBuild(x0,y0,x1,y1);   // armsshop.h
        }
        if(L->kind==LKIND_COMM){   // sound pack: a RADIO / SOUND SYSTEM on the community lots (only onto empty floor; R next to one tunes a station)
            int my=(y0+y1)/2;
            if(L->type==CT_LOUNGE){ gFree(cx-1,y0+2,'A'); gFree(cx+1,y0+2,'R'); }
            else if(L->type==CT_PLAZA||L->type==CT_PARK) gFree(cx,my+1,'R');
            else if(L->type==CT_SKATE||L->type==CT_BOTH) gFree(x0+1,y1-2,'R');
        }
        gPut(cx,y1-1,'P'); if(lifeMap[y1-1][cx+1]=='.') gPut(cx+1,y1-1,'B');   // you arrive at the front, your board beside you
    }
    flBlankUpper(); mapSave(); mapScan(); hhSlotsFree(); liveInvalidate(); camSnap=1;
    L->floors=1; nbValueLive(i);
}
static void nbFlagSync(int j){   // the flags on the live lot j (main.c, FLAGS) say what kind of place it is. No flags: nothing changes
    NbLot*L=&nbT.lot[j]; int f=0, conv=0;
    for(int y=0;y<MH;y++) for(int x=0;x<MW;x++){ char c=lifeMap[y][x]; if(c=='a') f|=1; else if(c=='k') f|=2; }
    if(!f) return;
    if(L->kind==LKIND_RES){   // a free lot becomes a community lot; a home or a lot where a household lives stays one (the flags are only decoration there)
        if(j==nbT.home||nbLives(j)){ toast("FLAGS NEED A FREE LOT"); return; }
        L->kind=LKIND_COMM; L->type=CT_PARK; conv=1; }
    int t=f==3?CT_BOTH:f==2?CT_SKATE:(L->type==CT_SKATE||L->type==CT_BOTH)?CT_PARK:L->type;   // (community flags alone: a skate park turns into a park, any other kind stays)
    if(t==L->type&&!conv) return;
    L->type=(u8)t; char q[24]; slCat(slCat(q,"NOW  "),ctNm[t]); toast(q);
}
static int nbStore(int j){   // the live map into lot j's house slot (keeps the slot it had when it still fits, else finds a free run)
    NbLot*L=&nbT.lot[j]; nbValueLive(j); slScan();
    if(L->slot>=0){
        if(slOwner[L->slot]==L->slot&&slI[L->slot].kind==SLK_HOUSE){ int e=houseSave(L->slot,L->name); if(e==SLE_OK) return 0; if(e!=SLE_NOROOM) return e; slDelete(L->slot); slScan(); }
        L->slot=-1;
    }
    for(int s=0;s<SLOT_N;s++) if(slOwner[s]<0){ int e=houseSave(s,L->name); if(e==SLE_OK){ L->slot=(s8)s; return 0; } if(e!=SLE_NOROOM) return e; }
    return SLE_NOROOM;
}
static int nbSave(void);
static int nbGo(int i){   // make lot i the live map. 1 = done (nbErr says why not)
    if(nbT.cur==i) return 1;
    box(60,64,120,24); text(76,72,"MOVING...",WHITE,1); present();
    ldShow("SAVING THE LOT YOU LEAVE",0,3);
    if(nbT.cur<NB_LOTS&&nbT.lot[nbT.cur].on){ int e=nbStore(nbT.cur); if(e){ nbErr=e==SLE_NOROOM?"NO FREE SLOTS FOR THIS LOT":slErrMsg(e); ldEnd(); return 0; } }
    ldShow("OPENING THE NEW LOT",1,3);
    NbLot*L=&nbT.lot[i]; int ok=0;
    if(L->slot>=0){ slScan(); if(slOwner[L->slot]==L->slot&&slI[L->slot].kind==SLK_HOUSE) ok=(houseLoad(L->slot)==SLE_OK); if(!ok) L->slot=-1; }
    if(!ok) nbTemplate(i);
    ldShow("SAVING THE TOWN",2,3);
    nbT.cur=(u8)i; nbBounds(); nbSave(); twKeep=0; ldEnd(); return 1;   // (another lot: other passers-by)   // (ldEnd: the loading screen is over, the music may come back)
}

// ---------- the town on the save chip ----------
static int nbTownList(int*l,int max){ slScan(); int n=0; for(int i=0;i<SLOT_N&&n<max;i++) if(slOwner[i]==i&&slI[i].kind==SLK_TOWN&&slGood[i]&&(slI[i].len==sizeof(Town)||slI[i].len==NB_TOWN_V1)) l[n++]=i; return n; }
static int nbRead(int s,Town*t){   // a town from its slot (1 = ok)
    volatile u8*b=SLB(s)+SLOT_HDR; u8*d=(u8*)t; unsigned n=slI[s].len; if(n>sizeof(Town)) n=sizeof(Town);
    for(unsigned i=0;i<sizeof(Town);i++) d[i]=i<n?b[i]:0;   // (an older, shorter town: the roofs after its end are zero = automatic)
    if(t->tag[0]!='T'||t->tag[1]!='W'||t->tag[2]!='N'||t->tag[3]!='1') return 0;
    t->name[NB_NAME]=0; for(int i=0;i<NB_LOTS;i++) t->lot[i].name[NB_NAME]=0;
    return 1;
}
static int nbLoad(void){   // the town the live room belongs to (or the first one)
    int l[SLOT_MAX], n=nbTownList(l,SLOT_MAX), pick=-1;
    for(int i=0;i<n&&pick<0;i++){ if(nbRead(l[i],&nbTmp)&&nbTmp.NB_ACT) pick=l[i]; }
    if(pick<0&&n) pick=l[0];
    if(pick<0||!nbRead(pick,&nbT)) return 0;
    nbT.NB_ACT=1; nbTS=pick; return 1;
}
static int nbSave(void){
    slScan(); int s=nbTS; SlInfo old; int had=s>=0&&slOwner[s]==s&&slInfo(s,&old)&&old.kind==SLK_TOWN;
    if(!had){ s=-1; for(int i=0;i<SLOT_N;i++) if(slOwner[i]<0){ s=i; break; } }
    if(s<0) return SLE_NOROOM;
    slOpen(s,1); const u8*d=(const u8*)&nbT; for(unsigned i=0;i<sizeof(Town);i++) svWr(SLO(s)+SLOT_HDR+i,d[i]);
    slHeader(s,SLK_TOWN,1,0,(int)sizeof(Town),slSumOf((volatile u8*)d,(int)sizeof(Town)),had?old.seq+1:1,nbT.name);
    nbTS=s; return slVerify(s);
}
_Static_assert(sizeof(Town)<=SLOT_SZ-SLOT_HDR,"the town must fit one slot");

// ---------- a new town ----------
static void nbLotAdd(int i,const char*nm,int x,int y,int w,int h,int kind,int type){
    NbLot*L=&nbT.lot[i]; L->on=1; L->x=(u8)x; L->y=(u8)y; L->w=(u8)w; L->h=(u8)h; L->kind=(u8)kind; L->type=(u8)type; L->slot=-1; L->floors=1; for(int q=0;q<5;q++) nbT.roof[i][q]=0;
    int k=0; for(;nm[k]&&k<NB_NAME;k++) L->name[k]=nm[k]; L->name[k]=0; L->value=(u16)(w*h*8);
}
enum { NS_SUBURB, NS_DESERT, NS_LAKE, NS_EMPTY, NS_N };
static const char* const nsNm[NS_N]={"GREEN SUBURB","DESERT TOWN","LAKESIDE","EMPTY LAND"};
static const char* const nsTown[NS_N]={"BOREVILLE","MESA FLATS","PINE COVE","NEW TOWN"};
static void nbGen(int style,const char*name){   // a new town in nbT (not saved). Its live lot is none yet (cur 255): going there builds the home lot
    u8*z=(u8*)&nbT; for(unsigned i=0;i<sizeof(Town);i++) z[i]=0;
    nbT.tag[0]='T'; nbT.tag[1]='W'; nbT.tag[2]='N'; nbT.tag[3]='1';
    { int k=0; for(;name[k]&&k<NB_NAME;k++) nbT.name[k]=name[k]; }
    nbT.season=(u8)(style==NS_LAKE?2:1); nbT.home=0; nbT.cur=255;
    for(int i=0;i<NB_LOTS;i++) nbT.lot[i].slot=-1;
    int rx=11, ry=11, base=style==NS_DESERT?NT_SAND:NT_GRASS;
    if(style==NS_DESERT){ rx=8; ry=15; } else if(style==NS_LAKE){ rx=5; ry=13; } else if(style==NS_EMPTY){ rx=-1; ry=12; }
    for(int y=0;y<NB_H;y++)for(int x=0;x<NB_W;x++) nbT.cell[y][x]=(u8)((x==rx||y==ry)?NT_ROAD:base);
    if(style==NS_SUBURB){
        nbLotAdd(0,"YOUR PLACE",0,0,10,10,LKIND_RES,0); nbLotAdd(1,"MAPLE 2",12,0,4,4,LKIND_RES,0); nbLotAdd(2,"MAPLE 4",17,0,6,6,LKIND_RES,0);
        nbLotAdd(3,"OAK 1",12,5,5,5,LKIND_RES,0); nbLotAdd(4,"OAK 3",18,7,5,4,LKIND_RES,0); nbLotAdd(5,"TOWN PARK",0,12,6,6,LKIND_COMM,CT_PARK);
        nbLotAdd(6,"CHILL SPOT",7,12,4,4,LKIND_COMM,CT_LOUNGE); nbLotAdd(7,"MAIN SQUARE",7,17,4,4,LKIND_COMM,CT_PLAZA);
        nbLotAdd(8,"SKATE PARK",0,19,7,5,LKIND_COMM,CT_SKATE); nbLotAdd(9,"OLD TOWN",13,13,10,10,LKIND_COMM,CT_OLDTOWN);
        for(int y=21;y<NB_H;y++)for(int x=8;x<11;x++) if(x+y>=30) nbT.cell[y][x]=NT_WATER;   // a pond
    } else if(style==NS_DESERT){
        nbLotAdd(0,"YOUR PLACE",1,1,6,6,LKIND_RES,0); nbLotAdd(1,"MESA 1",10,1,5,5,LKIND_RES,0); nbLotAdd(2,"MESA 3",16,1,4,4,LKIND_RES,0);
        nbLotAdd(3,"MESA 5",20,1,4,6,LKIND_RES,0); nbLotAdd(4,"CACTUS 2",10,8,6,5,LKIND_RES,0); nbLotAdd(5,"DRY CREEK",0,8,7,6,LKIND_COMM,CT_SKATE);
        nbLotAdd(6,"OASIS",17,9,5,5,LKIND_COMM,CT_LOUNGE); nbLotAdd(7,"OLD MINE",10,16,10,8,LKIND_COMM,CT_OLDTOWN); nbLotAdd(8,"TOWN PLAZA",1,17,6,6,LKIND_COMM,CT_PLAZA);
        for(int y=17;y<21;y++)for(int x=21;x<24;x++) nbT.cell[y][x]=NT_WATER;   // the oasis spring
        for(int y=0;y<NB_H;y++)for(int x=0;x<NB_W;x++) if(nbT.cell[y][x]==NT_SAND&&((x*7+y*3)%11)==0) nbT.cell[y][x]=NT_DIRT;
    } else if(style==NS_LAKE){
        nbLotAdd(0,"YOUR PLACE",0,0,5,5,LKIND_RES,0); nbLotAdd(1,"SHORE 1",6,0,6,4,LKIND_RES,0); nbLotAdd(2,"SHORE 3",6,6,5,5,LKIND_RES,0);
        nbLotAdd(3,"BIRCH 2",0,6,4,6,LKIND_RES,0); nbLotAdd(4,"HARBOR PARK",0,15,5,6,LKIND_COMM,CT_PARK); nbLotAdd(5,"BOARDWALK",14,15,6,4,LKIND_COMM,CT_PLAZA);
        nbLotAdd(6,"CABIN",20,15,4,4,LKIND_COMM,CT_LOUNGE); nbLotAdd(7,"OLD DOCKS",6,17,8,5,LKIND_COMM,CT_OLDTOWN); nbLotAdd(8,"RIDGE SKATE",14,20,6,4,LKIND_COMM,CT_SKATE);
        for(int y=0;y<NB_H;y++)for(int x=0;x<NB_W;x++) if(nbAt(x,y)<0&&nbT.cell[y][x]!=NT_ROAD&&(x-17)*(x-17)+(y-6)*(y-6)*3/2<34) nbT.cell[y][x]=NT_WATER;   // the lake
    } else nbLotAdd(0,"YOUR PLACE",2,3,6,6,LKIND_RES,0);
    u32 r=0x2F6E2B1u+(u32)style*7919u; int dens=style==NS_EMPTY?25:style==NS_DESERT?45:70;
    for(int y=0;y<NB_H;y++)for(int x=0;x<NB_W;x++){
        r=r*1103515245u+12345u; int v=(int)(r>>16)&255;
        if(nbAt(x,y)>=0) continue;
        u8 t=nbT.cell[y][x];
        if(t==NT_ROAD){ if(((x==rx&&(y%4)==2)||(y==ry&&(x%4)==2))&&!(x==rx&&y==ry)) nbT.cell[y][x]=(u8)(NT_PLAZA|(DC_LAMP<<3)); continue; }   // street lamps
        if(t==NT_WATER){ continue; }
        if(v<dens){ int d;
            if(style==NS_DESERT) d=v<12?DC_TREE:v<30?DC_ROCK:DC_BUSH;
            else if(style==NS_LAKE) d=v<38?DC_PINE:v<52?DC_TREE:v<62?DC_ROCK:DC_BUSH;
            else d=v<30?DC_TREE:v<45?DC_PINE:v<58?DC_BUSH:DC_FLOWER;
            nbT.cell[y][x]|=(u8)(d<<3); }
    }
    if(style==NS_SUBURB){ nbT.cell[22][7]=(u8)(NT_SAND); nbT.cell[23][7]=(u8)(NT_SAND|(DC_ROCK<<3)); nbT.cell[20][9]=(u8)(NT_GRASS|(DC_BENCH<<3)); nbT.cell[10][4]=(u8)(NT_PLAZA|(DC_FOUNTAIN<<3)); }
}
// ---------- drawing ----------
static int nbHw, nbHh, nbOx, nbOy, nbK;   // half width / height of a cell, where cell (0,0) is, the object scale
static u16 nbLit(u16 c,int d){ int r=(c&31)+d, g=((c>>5)&31)+d, b=((c>>10)&31)+d; r=r<0?0:r>31?31:r; g=g<0?0:g>31?31:g; b=b<0?0:b>31?31:b; return RGB(r,g,b); }
static u16 nbTint(u16 c){
    int r=c&31, g=(c>>5)&31, b=(c>>10)&31;
    if(nbT.tod==1){ r=r*29/32+1; g=g*22/32; b=b*18/32; }
    else if(nbT.tod==2){ r=r*10/32; g=g*11/32; b=b*13/32+3; }
    else if(nbT.tod==3){ r=r*24/32+5; g=g*21/32+3; b=b*25/32+6; }   // dawn (the main menu only): a pink, misty light
    if(r>31)r=31;
    if(b>31)b=31;
    return RGB(r,g,b);
}
static u16 nbGround(int t,int cx,int cy){
    int alt=(cx+cy)&1; u16 c;
    switch(t){
        case NT_GRASS: { static const u16 gs[4][2]={{RGB(10,21,8),RGB(9,20,7)},{RGB(8,18,5),RGB(7,17,5)},{RGB(17,16,6),RGB(16,14,5)},{RGB(27,28,30),RGB(25,26,29)}}; c=gs[nbT.season][alt]; break; }
        case NT_DIRT: c=alt?RGB(16,11,6):RGB(15,10,6); break;
        case NT_SAND: c=alt?RGB(27,24,15):RGB(26,23,14); break;
        case NT_WATER: c=nbT.season==3?RGB(21,25,29):(alt?RGB(5,12,24):RGB(6,14,25)); break;
        case NT_PLAZA: c=alt?RGB(19,19,18):RGB(17,17,16); break;
        default: c=RGB(7,7,8); break;
    }
    return nbTint(c);
}
static void nbDia(int sx,int sy,u16 c){   // a ground diamond, top vertex at (sx, sy)
    for(int r=0;r<2*nbHh;r++){ int d=r<nbHh?r:2*nbHh-1-r, w=(d+1)*nbHw/nbHh; rect(sx-w,sy+r,2*w,1,c); }
}
static void nbPos(int cx,int cy,int*sx,int*sy){ *sx=nbOx+(cx-cy)*nbHw; *sy=nbOy+(cx+cy)*nbHh; }
static void nbOutline(int x,int y,int w,int h,u16 c){   // the iso outline of a block of cells
    int ax,ay,bx,by,cx_,cy_,dx,dy; nbPos(x,y,&ax,&ay); nbPos(x+w,y,&bx,&by); nbPos(x+w,y+h,&cx_,&cy_); nbPos(x,y+h,&dx,&dy);
    line(ax,ay,bx,by,c); line(bx,by,cx_,cy_,c); line(cx_,cy_,dx,dy,c); line(dx,dy,ax,ay,c);
}
static void nbDecor(int d,int x,int y){   // x, y = the middle of the cell
    int k=nbK; u16 trunk=nbTint(RGB(12,7,3));
    switch(d){
        case DC_TREE: { rect(x,y-3*k,k,3*k,trunk);
            if(nbT.season==3){ line(x,y-3*k,x-2*k,y-6*k,trunk); line(x,y-3*k,x+2*k,y-6*k,trunk); px(x-2*k,y-6*k,WHITE); px(x+2*k,y-6*k,WHITE); break; }
            static const u16 cn[4]={RGB(12,24,10),RGB(5,16,4),RGB(25,12,3),0}; disc(x,y-5*k,2*k+(k>1),nbTint(cn[nbT.season]));
            if(nbT.season==0){ px(x-k,y-6*k,nbTint(RGB(31,20,24))); px(x+k,y-5*k,nbTint(RGB(31,20,24))); } else if(nbT.season==2) px(x+k,y-6*k,nbTint(RGB(28,20,4))); break; }
        case DC_PINE: { u16 c=nbTint(RGB(4,13,7)); for(int i=0;i<6*k;i++) rect(x-i/3,y-7*k+i,2*(i/3)+1,1,c); rect(x,y-k,k,k,trunk); if(nbT.season==3){ px(x,y-7*k,WHITE); rect(x-k,y-4*k,2*k+1,1,WHITE); } break; }
        case DC_BUSH: disc(x,y-k,k+1,nbTint(nbT.season==3?RGB(24,26,28):nbT.season==2?RGB(20,14,4):RGB(6,18,6))); break;
        case DC_FLOWER: if(nbT.season==3) break; px(x-k,y,nbTint(RGB(31,8,10))); px(x+k,y-k/2,nbTint(RGB(31,28,6))); px(x,y+k/2,nbTint(RGB(28,12,30))); px(x+2*k,y+k/2,nbTint(RGB(31,31,31))); break;
        case DC_ROCK: rect(x-k,y-k,3*k,k+1,nbTint(RGB(15,15,16))); rect(x,y-2*k+1,k+1,k,nbTint(RGB(20,20,21))); break;
        case DC_LAMP: rect(x,y-7*k,1,7*k,nbTint(RGB(9,9,10))); if(nbT.tod){ disc(x,y-7*k,k+1,RGB(31,29,14)); } else px(x,y-7*k,RGB(24,24,20)); break;
        case DC_BENCH: rect(x-2*k,y-k,4*k,k,nbTint(RGB(16,9,4))); px(x-2*k,y,nbTint(RGB(6,6,6))); px(x+2*k-1,y,nbTint(RGB(6,6,6))); break;
        case DC_FOUNTAIN: rect(x-2*k,y-k,4*k+1,k+1,nbTint(RGB(22,22,22))); rect(x-k,y-k,2*k+1,1,nbTint(nbT.season==3?RGB(24,27,30):RGB(10,20,30))); rect(x,y-3*k,1,2*k,nbTint(RGB(26,29,31))); break;
    }
}
static void nbIsoBox(int bx,int by,int hw2,int wh,int roofH,u16 wl,u16 wr,u16 rl,u16 rr){   // a house: square footprint, walls and a pyramid roof
    for(int x=bx-hw2;x<=bx+hw2;x++){
        int dx=x<bx?bx-x:x-bx, yb=by+(hw2-dx)/2, yt=yb-wh, ya=by-wh-roofH+roofH*dx/(hw2?hw2:1);
        if(wh>0) rect(x,yt,1,wh,x<bx?wl:wr);
        if(roofH>0&&yt>ya) rect(x,ya,1,yt-ya,x<bx?rl:rr);
    }
}
// ---- ROOFS: every house roof is described by 5 bytes (Town.roof), all zero = the old automatic pyramid ----
static const u16 nbRoofAuto[6]={RGB(20,6,5),RGB(9,9,12),RGB(14,9,5),RGB(6,13,9),RGB(22,10,6),RGB(12,7,14)};   // by lot number, as before
static const u16 nbRoofPal[8]={RGB(20,6,5),RGB(9,9,12),RGB(14,9,5),RGB(6,13,9),RGB(24,12,6),RGB(12,7,14),RGB(6,10,20),RGB(24,21,8)};
static const char* const nbRoofStNm[4]={"PYRAMID","FLAT","HIP","SHED"};
static const char* const nbRoofColNm[10]={"AUTO","RED","SLATE","BROWN","GREEN","CLAY","PURPLE","BLUE","THATCH","CUSTOM"};
static u16 nbRoofBase(int i){ const u8*R=nbT.roof[i]; int c=R[0]>>4; if(c==0||c>9) return nbRoofAuto[i%6]; if(c==9) return (u16)((R[3]|(R[4]<<8))&0x7FFF); return nbRoofPal[c-1]; }
// A roof over a box: bx,by = the front corner, hw2 = half width of the walls, wh = wall height, st = style, rh = roof height, ov = overhang (all in pixels)
static void nbRoofDraw(int bx,int by,int hw2,int wh,int st,int rh,int ov,u16 rl,u16 rr){
    int hr=hw2+ov; if(hr<1) hr=1;
    for(int x=bx-hr;x<=bx+hr;x++){
        int dx=x<bx?bx-x:x-bx, yt=by+(hr-dx)/2-wh, ya; u16 c=x<bx?rl:rr;
        if(st==1){   // FLAT: a lid with a rim
            ya=by-wh-rh-(hr-dx)/2; if(yt-rh>ya) rect(x,ya,1,yt-rh-ya,nbLit(c,2)); rect(x,yt-rh,1,rh,c); continue; }
        if(st==2){ int h=rh*(hr-dx)*2/hr; if(h>rh) h=rh; ya=by-wh-h; }   // HIP: a flat ridge, sloping ends
        else if(st==3) ya=by-wh-(hr-dx)/2-rh*(bx+hr-x)/(2*hr);          // SHED: one slope, high on the left
        else ya=by-wh-rh+rh*dx/hr;                                       // PYRAMID
        if(yt>ya) rect(x,ya,1,yt-ya,c);
    }
}
static void nbDrawHouse(int i,int sx,int sy){   // a lot's house: walls, windows, door and its roof (also the ROOF editor's preview)
    const NbLot*L=&nbT.lot[i]; int k=nbK, s=(L->w<L->h?L->w:L->h);
    static const u16 wallC[6]={RGB(28,26,20),RGB(18,23,28),RGB(29,22,18),RGB(20,25,18),RGB(26,26,26),RGB(27,24,14)};
    int hw2=s*nbHw/2, wh=(3+3*L->floors)*k, c=i%6; u16 w=nbTint(wallC[c]);
    const u8*R=nbT.roof[i]; int st=R[0]&15; if(st>3) st=0;
    int rh=R[1]?R[1]*k:(st==1?2*k:hw2/2+2*k), ov=(R[2]&7)*k; u16 base=nbRoofBase(i);
    nbIsoBox(sx,sy,hw2,wh,0,nbTint(nbLit(wallC[c],-5)),w,0,0);
    nbRoofDraw(sx,sy,hw2,wh,st,rh,ov,nbTint(nbLit(base,-3)),nbTint(base));
    u16 win=nbT.tod==2?RGB(31,28,12):nbTint(RGB(12,18,24));
    for(int f=0;f<L->floors;f++){ int y=sy+hw2/4-(3+3*f)*k-k; px(sx+hw2/3,y,win); px(sx+2*hw2/3,y-k,win); px(sx-hw2/3,y,win); px(sx-2*hw2/3,y-k,win); }
    rect(sx+hw2/2,sy+hw2/4-2*k,k,2*k,nbTint(RGB(10,6,3)));   // the door
}
static void nbDrawLotModel(int i,int sx,int sy){   // HOOK: a lot's building (a small icon now; the real house can be drawn here later)
    const NbLot*L=&nbT.lot[i]; int k=nbK, s=(L->w<L->h?L->w:L->h);
    if(L->kind==LKIND_RES){
        if(L->slot<0&&nbT.cur!=i&&!nbLives(i)){   // FOR SALE (a lot where a household lives shows their house)
            rect(sx,sy-6*k,1,6*k,nbTint(RGB(14,9,4))); rect(sx-2*k,sy-7*k,4*k+1,3*k,nbTint(WHITE)); rect(sx-2*k,sy-7*k,4*k+1,1,nbTint(RGB(28,4,4))); px(sx,sy-6*k,nbTint(RGB(28,4,4)));
            return; }
        nbDrawHouse(i,sx,sy);
        return;
    }
    switch(L->type){
        case CT_PARK: { int sk=nbK; nbK=k+1; nbDecor(DC_TREE,sx-k*s/2,sy); nbDecor(DC_TREE,sx+k*s/2,sy-k); nbK=sk; nbDecor(DC_BENCH,sx,sy+k); break; }
        case CT_BOTH: { int sk=nbK; nbK=k+1; nbDecor(DC_TREE,sx+k*s/2,sy-k); nbK=sk; for(int i2=0;i2<3*k;i2++){ int h=(3*k-i2)*(3*k-i2)/(3*k); rect(sx-3*k+i2-k,sy-h,1,h+1,nbTint(RGB(20,20,22))); rect(sx+k-i2,sy-h,1,h+1,nbTint(RGB(17,17,19))); } rect(sx-4*k,sy,4*k,1,nbTint(RGB(28,10,6))); break; }
        case CT_SKATE: for(int i2=0;i2<4*k;i2++){ int h=(4*k-i2)*(4*k-i2)/(4*k); rect(sx-3*k+i2,sy-h,1,h+1,nbTint(RGB(20,20,22))); rect(sx+3*k-i2,sy-h,1,h+1,nbTint(RGB(17,17,19))); } rect(sx-3*k,sy,6*k,1,nbTint(RGB(28,10,6))); break;
        case CT_PLAZA: { int sk=nbK; nbK=k+1; nbDecor(DC_FOUNTAIN,sx,sy); nbK=sk; break; }
        case CT_ARMS: { int hw2=s*nbHw/2; nbIsoBox(sx,sy,hw2,3*k,hw2/2,nbTint(RGB(18,6,5)),nbTint(RGB(24,9,7)),nbTint(RGB(10,3,3)),nbTint(RGB(14,5,4))); rect(sx-hw2/3,sy-4*k,2*hw2/3,k,nbTint(RGB(31,26,6))); break; }   // a red shop with a gold sign
        case CT_PRISON: { int hw2=s*nbHw/2; nbIsoBox(sx,sy,hw2,4*k,hw2/2,nbTint(RGB(13,13,14)),nbTint(RGB(18,18,19)),nbTint(RGB(8,8,9)),nbTint(RGB(11,11,12))); rect(sx-hw2/2,sy-5*k,hw2,k,nbTint(RGB(25,6,5))); rect(sx+hw2/3,sy-9*k,2*k,5*k,nbTint(RGB(10,10,11))); break; }   // a grey block with a red stripe and a watch tower
        case CT_LOUNGE: { int hw2=s*nbHw/2; nbIsoBox(sx,sy,hw2,5*k,hw2/3,nbTint(RGB(12,6,16)),nbTint(RGB(17,9,22)),nbTint(RGB(6,4,8)),nbTint(RGB(9,6,12))); rect(sx-k,sy-6*k,2*k+1,k,nbT.tod?RGB(31,8,26):nbTint(RGB(24,8,20))); break; }
        default: { int hw2=s*nbHw/3; nbIsoBox(sx,sy,hw2,7*k,k,nbTint(RGB(14,14,15)),nbTint(RGB(18,18,19)),nbTint(RGB(9,9,10)),nbTint(RGB(12,12,13)));   // old town: a factory with a chimney
            rect(sx+hw2/2,sy-12*k,2*k,6*k,nbTint(RGB(16,7,5))); px(sx+hw2/2,sy-13*k,nbTint(RGB(24,24,24))); px(sx+hw2/2+k,sy-14*k,nbTint(RGB(22,22,22))); break; }
    }
}
static void nbSky(void){   // the main menu's sky: a gradient for the time of day, stars at night
    static const u16 top[4]={RGB(14,17,28),RGB(13,8,18),RGB(1,1,5),RGB(13,11,21)}, bot[4]={RGB(24,24,30),RGB(31,19,11),RGB(4,6,13),RGB(29,20,22)};
    int t=nbT.tod&3; for(int y=0;y<SH;y++) rect(0,y,SW,1,s3Mix(top[t],bot[t],y,SH));
    if(t==2){ u32 r=0x1234567u; for(int i=0;i<60;i++){ r=r*1103515245u+12345u; px((int)((r>>8)%SW),(int)((r>>20)%SH),(r&0x100)?RGB(24,24,28):RGB(14,14,20)); } }
}
// tool: -1 = the whole town small (a backdrop), -2 = the main menu's backdrop (close up around ccx, ccy, the whole screen), 0.. = the town screen's tools
static void nbDrawTown(int ccx,int ccy,int tool,int ghostW,int ghostH,int ghostOk){
    int bd=tool==-2;
    if(nbT.zoom&&tool!=-1){ nbHw=8; nbHh=4; nbK=2; nbOx=120-(ccx-ccy)*nbHw; nbOy=62-(ccx+ccy)*nbHh+(bd?14:0); }
    else { nbHw=4; nbHh=2; nbK=1; nbOx=120; nbOy=20; }
    u16 sky=nbT.tod==2?RGB(1,1,4):nbT.tod==1?RGB(10,6,8):RGB(4,7,12);
    int yT=bd?-2*nbHh:4, yB=bd?SH:130, yB2=bd?SH+48:140;
    if(bd){ clipAll(); nbSky(); } else { clipAll(); fillCols(0,ROW_W,sky); clipSet(0,12,SW,122); }
    for(int s=0;s<NB_W+NB_H-1;s++) for(int cx=(s<NB_H?0:s-NB_H+1);cx<=s&&cx<NB_W;cx++){   // the ground, back to front
        int cy=s-cx, sx, sy; nbPos(cx,cy,&sx,&sy); if(sx<-nbHw||sx>SW+nbHw||sy<yT||sy>yB) continue;
        int li=nbAt(cx,cy); u8 c=nbT.cell[cy][cx];
        u16 g; if(li>=0){ const NbLot*L=&nbT.lot[li]; g=L->kind==LKIND_COMM&&L->type!=CT_PARK?nbTint(((cx+cy)&1)?RGB(20,20,19):RGB(18,18,17)):nbLit(nbGround(NT_GRASS,cx,cy),1); }   // a lot: mown lawn, or paving
        else g=nbGround(NB_GR(c),cx,cy);
        nbDia(sx,sy,g);
        if(li<0&&NB_GR(c)==NT_ROAD&&nbT.zoom){   // lane marks
            int h=(cx>0&&NB_GR(nbT.cell[cy][cx-1])==NT_ROAD)||(cx<NB_W-1&&NB_GR(nbT.cell[cy][cx+1])==NT_ROAD), v=(cy>0&&NB_GR(nbT.cell[cy-1][cx])==NT_ROAD)||(cy<NB_H-1&&NB_GR(nbT.cell[cy+1][cx])==NT_ROAD);
            u16 m=nbTint(RGB(26,24,10)); if(h&&!v) line(sx-2,sy+nbHh-1,sx+2,sy+nbHh+1,m); else if(v&&!h) line(sx+2,sy+nbHh-1,sx-2,sy+nbHh+1,m); }
        if(li<0&&NB_GR(c)==NT_WATER&&nbT.season!=3&&((cx*5+cy*3)&3)==0) rect(sx-1,sy+nbHh,3,1,nbTint(RGB(14,22,30)));
    }
    for(int i=0;i<NB_LOTS;i++) if(nbT.lot[i].on){ const NbLot*L=&nbT.lot[i]; nbOutline(L->x,L->y,L->w,L->h,nbTint(i==nbT.home?GOLD:RGB(25,25,24))); }
    for(int s=0;s<NB_W+NB_H-1;s++) for(int cx=(s<NB_H?0:s-NB_H+1);cx<=s&&cx<NB_W;cx++){   // what stands on it, back to front
        int cy=s-cx, sx, sy; nbPos(cx,cy,&sx,&sy); if(sx<-24||sx>SW+24||sy<-8||sy>yB2) continue;
        int li=nbAt(cx,cy);
        if(li<0){ int d=NB_DC(nbT.cell[cy][cx]); if(d) nbDecor(d,sx,sy+nbHh); continue; }
        const NbLot*L=&nbT.lot[li];
        if(cx==L->x+L->w-1&&cy==L->y+L->h-1){   // the lot's front corner: its building, drawn at the lot's middle
            int mx,my; nbPos(L->x,L->y,&mx,&my); my+=(L->w+L->h)*nbHh/2; mx+=(L->w-L->h)*nbHw/2;
            nbDrawLotModel(li,mx,my);
            if(li==nbT.home){ rect(mx-1,my-(16+3*L->floors)*nbK,3,3,GOLD); px(mx,my-(17+3*L->floors)*nbK,WHITE); }   // home: a gold marker over it
        }
    }
    if(tool<0){ clipAll(); return; }   // (the chooser's backdrop: no cursor)
    int sx,sy; nbPos(ccx,ccy,&sx,&sy);   // the cursor
    if(tool==4) nbOutline(ccx,ccy,ghostW,ghostH,ghostOk?RGB(8,30,8):RGB(31,6,6));
    else { int li=nbAt(ccx,ccy); if(tool==0&&li>=0){ const NbLot*L=&nbT.lot[li]; nbOutline(L->x,L->y,L->w,L->h,GOLD); nbOutline(L->x,L->y,L->w,L->h,(uiTicks&16)?WHITE:GOLD); } }
    line(sx,sy,sx+nbHw,sy+nbHh,WHITE); line(sx+nbHw,sy+nbHh,sx,sy+2*nbHh,WHITE); line(sx,sy+2*nbHh,sx-nbHw,sy+nbHh,WHITE); line(sx-nbHw,sy+nbHh,sx,sy,WHITE);
    clipAll();
}
static char* nbMoney(char*d,money_t v){   // §1,234
    *d++=(char)0xC2; *d++=(char)0xA7; return simCatMoney(d,v,1);
}
static const char* const nbTools[5]={"LOTS","PAINT","ROADS","DECOR","NEW LOT"};
static const u8 nbSizes[6][2]={{4,4},{5,5},{6,6},{8,8},{10,10},{8,5}};
static const char* const nbSizeNm[6]={"16 X 16","20 X 20","24 X 24","32 X 32","40 X 40","32 X 20"};
static void nbPanel(int ccx,int ccy,int tool,int sub){
    rect(0,0,SW,12,PANEL); text(4,3,nbT.name,GOLD,1);
    { char b[24]; char*e=slCat(b,seasNm[nbT.season]); e=slCat(e," "); slCat(e,todNm[nbT.tod]); text(SW-4-tw(b,1),3,b,DIMC,1); }
    rect(0,122,SW,38,PANEL);
    for(int t=0,x=4;t<5;t++){ int w=tw(nbTools[t],1)+6; if(t==tool) rect(x-2,124,w,9,RGB(6,16,8)); text(x+1,125,nbTools[t],t==tool?WHITE:DIMC,1); x+=w+2; }
    int li=nbAt(ccx,ccy); char b[40]; char*e;
    if(tool==0&&li>=0){
        const NbLot*L=&nbT.lot[li]; text(4,135,L->name,WHITE,1);
        e=slCat(b,L->kind==LKIND_RES?"HOME LOT ":ctNm[L->type]); e=slCat(e,"  "); e=slNum(e,L->w*4); e=slCat(e," X "); slNum(e,L->h*4); text(84,135,b,DIMC,1);
        if(li==nbT.home) e=slCat(b,"YOUR HOME  "); else if(L->kind==LKIND_COMM) e=slCat(b,"COMMUNITY  "); else if(nbWho(li,0)){ char f[24]; f[0]=0; nbWho(li,f); e=slCat(b,f); e=slCat(e,"  "); } else if(L->slot<0&&nbT.cur!=li) e=slCat(b,"FREE LOT  "); else { e=slCat(b,"HOUSE "); e=slNum(e,L->floors); e=slCat(e,L->floors>1?" FLOORS  ":" FLOOR  "); }
        nbMoney(e,nbPrice(L)); text(4,145,b,GOLD,1);
        text(4,153,nbT.cur==li?"A  LOT MENU   YOU ARE HERE":"A  LOT MENU",RGB(12,14,16),1);
        return;
    }
    if(tool==1){ e=slCat(b,"PAINT "); slCat(e,ntNm[sub]); }
    else if(tool==2) slCat(b,"A LAYS ROAD  PAINT REMOVES IT");
    else if(tool==3){ e=slCat(b,"PLANT "); slCat(e,dcNm[sub]); }
    else if(tool==4){ e=slCat(b,"NEW LOT "); slCat(e,nbSizeNm[sub]); }
    else { e=slCat(b,ntNm[NB_GR(nbT.cell[ccy][ccx])]); if(NB_DC(nbT.cell[ccy][ccx])){ e=slCat(e,"  "); slCat(e,dcNm[NB_DC(nbT.cell[ccy][ccx])]); } }
    text(4,135,b,WHITE,1);
    text(4,145,tool==0?"L R TOOLS   START TOWN MENU":tool==2?"L R TOOLS   HOLD A TO DRAW":"SELECT CHANGES IT   A PLACES",DIMC,1);
    text(4,153,tool?"B BACK TO THE LOTS TOOL":"B LEAVES THE NEIGHBORHOOD",RGB(12,14,16),1);
}

// ---------- actions ----------
static int nbFree(int x,int y,int w,int h,int skip){   // can a lot go there?
    if(x<0||y<0||x+w>NB_W||y+h>NB_H) return 0;
    for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++){ int l=nbAt(i,j); if((l>=0&&l!=skip)||NB_GR(nbT.cell[j][i])==NT_ROAD||NB_GR(nbT.cell[j][i])==NT_WATER) return 0; }
    return 1;
}
static int nbCash(money_t*have){ simsDefaults(); if(!simsLoad()){ *have=-1; return 0; } *have=simMoney; return 1; }   // the life's cash (0 = no life yet)
// ---- ROOF EDITOR (lot menu > ROOF): pick a style and colour, set the height and overhang, or mix your own colour. A saves, B cancels ----
static char* nbItoa(char*e,int v){ if(v>=10) *e++=(char)('0'+v/10); *e++=(char)('0'+v%10); *e=0; return e; }
static int nbRoofEdit(int li){   // 1 = saved
    NbLot*L=&nbT.lot[li]; u8*R=nbT.roof[li]; u8 old[5]; for(int q=0;q<5;q++) old[q]=R[q];
    int sel=0, shw=nbHw, shh=nbHh, sk=nbK, dirty=1; u16 prev=keyNow();
    static const char* const lab[7]={"STYLE","COLOR","HEIGHT","OVERHANG","RED","GREEN","BLUE"};
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        int cc=R[0]>>4, rows=cc==9?7:4; if(sel>=rows) sel=rows-1;
        int d=(pr&K_RIGHT)?1:(pr&K_LEFT)?-1:0, big=(pr&K_R)?4:(pr&K_L)?-4:0;
        if(pr&K_UP){ sel=(sel+rows-1)%rows; dirty=1; }
        if(pr&K_DOWN){ sel=(sel+1)%rows; dirty=1; }
        if(pr&K_B||pr&K_START){ for(int q=0;q<5;q++) R[q]=old[q]; nbHw=shw; nbHh=shh; nbK=sk; return 0; }
        if(pr&K_A){ nbHw=shw; nbHh=shh; nbK=sk; nbSave(); toast("ROOF SAVED"); return 1; }
        if(d||big){
            dirty=1; int st=R[0]&15;
            if(sel==0){ st=(st+d+4)%4; R[0]=(u8)((R[0]&0xF0)|st); }
            else if(sel==1){ u16 cur=nbRoofBase(li); cc=(cc+d+10)%10; R[0]=(u8)((cc<<4)|(R[0]&15)); if(cc==9&&!(R[3]|R[4])){ R[3]=(u8)(cur&255); R[4]=(u8)(cur>>8); } }
            else if(sel==2) R[1]=(u8)((R[1]+d+16)%16);
            else if(sel==3) R[2]=(u8)((R[2]+d+8)%8);
            else { int sh=5*(sel-4); u16 c=(u16)((R[3]|(R[4]<<8))&0x7FFF); int v=(c>>sh)&31; v+=d+big; if(v<0) v=0; if(v>31) v=31; c=(u16)((c&~(31<<sh))|(v<<sh)); R[3]=(u8)(c&255); R[4]=(u8)(c>>8); }
        }
        if(!dirty){ vsync(); continue; }
        dirty=0; objHideAll();
        nbSky(); rect(0,92,SW,68,nbTint(RGB(8,18,6)));
        nbHw=8; nbHh=4; nbK=2; nbDrawHouse(li,120,74);
        rect(0,102,SW,58,RGB(3,4,7)); rect(0,102,SW,1,GOLD);
        { char t[24]; char*e=t; const char*h="ROOF  "; while(*h) *e++=*h++; for(int q=0;L->name[q]&&q<NB_NAME;q++) *e++=L->name[q]; *e=0; text(6,106,t,GOLD,1); }
        for(int r=0;r<rows;r++){
            int x=r<4?6:128, y=r<4?116+(r)*9:116+(r-4)*9; char v[16]; v[0]=0; int st=R[0]&15, c9=R[0]>>4;
            if(r==0){ const char*q=nbRoofStNm[st>3?0:st]; int z=0; for(;q[z];z++) v[z]=q[z]; v[z]=0; }
            else if(r==1){ const char*q=nbRoofColNm[c9>9?0:c9]; int z=0; for(;q[z];z++) v[z]=q[z]; v[z]=0; }
            else if(r==2){ if(R[1]) nbItoa(v,R[1]); else { const char*q="AUTO"; int z=0; for(;q[z];z++) v[z]=q[z]; v[z]=0; } }
            else if(r==3) nbItoa(v,R[2]&7);
            else { u16 c=(u16)((R[3]|(R[4]<<8))&0x7FFF); nbItoa(v,(c>>(5*(r-4)))&31); }
            if(r==sel) rect(x-2,y-1,r<4?118:104,9,RGB(6,18,10));
            text(x,y,lab[r],r==sel?GOLD:WHITE,1); text(x+(r<4?64:44),y,v,r==sel?WHITE:RGB(22,25,28),1);
        }
        if(rows==7) rect(214,116,20,20,nbRoofBase(li));
        text(6,152,"UP DOWN PICK  LEFT RIGHT SET  A SAVE  B BACK",RGB(12,14,16),1);
        present();
    }
}
static int nbLotMenu(int li){   // returns 1 when the screen should close (play started and asked for the main menu)
    NbLot*L=&nbT.lot[li]; const char*it[10]; int id[10], n=0;
    enum { A_PLAY, A_BUILD, A_MOVE, A_RENAME, A_TYPE, A_BULL, A_DEL, A_HH, A_NEWHH, A_ROOF, A_CONV };
    char who[24]; who[0]=0; int lives=L->kind==LKIND_RES&&nbWho(li,who);
    static char ph[32] EWRAM_BSS; if(lives){ char*e=slCat(ph,"PLAY "); slCat(e,who); it[n]=ph; id[n++]=A_HH; }   // (The Sims 2: play the household that lives there)
    else if(L->kind==LKIND_RES&&li!=nbT.home){ it[n]="NEW HOUSEHOLD HERE"; id[n++]=A_NEWHH; }
    it[n]=L->kind==LKIND_COMM?"VISIT":li==nbT.home?"PLAY":"PLAY HERE"; id[n++]=A_PLAY;
    it[n]="BUILD"; id[n++]=A_BUILD;
    if(L->kind==LKIND_RES){ it[n]="ROOF"; id[n++]=A_ROOF; }   // size, shape and colour your own roof
    if(L->kind==LKIND_RES&&li!=nbT.home&&!lives){ it[n]="MOVE IN"; id[n++]=A_MOVE; }
    it[n]="RENAME"; id[n++]=A_RENAME;
    if(L->kind==LKIND_COMM){ it[n]="CHANGE TYPE"; id[n++]=A_TYPE; }
    if(li!=nbT.home&&!lives){ it[n]=L->kind==LKIND_COMM?"MAKE RESIDENTIAL":"MAKE COMMUNITY"; id[n++]=A_CONV; }   // any lot can be a home or a public place (not yours, and not one a household lives on)
    if(L->slot>=0||nbT.cur==li){ it[n]="BULLDOZE"; id[n++]=A_BULL; }
    it[n]="DELETE LOT"; id[n++]=A_DEL;
    int c=menu(L->name,it,n); if(c<0) return 0;
    switch(id[c]){
        case A_HH: case A_NEWHH:   // another household: yours waits in the bank, theirs (or new Sims, made in the creator) plays this lot
            if(!(id[c]==A_HH?hhPlayAt(li):hhNewAt(li))) return 0;
            if(id[c]==A_NEWHH){ creatureEditor(); if(gToMenu) return 1; }
            else { nbPlaying=1; lifeMode(0); nbPlaying=0; if(gToMenu) return 1; }
            nbValueLive(li); nbStore(li); nbSave(); menuMusSync(); return 0;
        case A_PLAY: case A_BUILD:
            if(!nbGo(li)){ toast(nbErr); return 0; }
            if(id[c]==A_PLAY){ nbPlaying=1; lifeMode(0); nbPlaying=0; if(gToMenu) return 1; }
            else { nbEditPass=1; vpFull(); mapEditor(); nbEditPass=0; }   // (the only way to build a community lot)
            if(id[c]==A_BUILD) nbFlagSync(li);   // (flags placed while building set what kind of place the lot is)
            nbValueLive(li); nbStore(li); nbSave(); menuMusSync(); return 0;
        case A_MOVE: {
            money_t have; int price=nbPrice(L), sale=nbT.home<NB_LOTS&&nbT.lot[nbT.home].on?nbPrice(&nbT.lot[nbT.home]):0, net=price-sale;
            if(nbCash(&have)){
                char q[40]; char*e=slCat(q,net>=0?"PAY ":"GET "); nbMoney(e,net>=0?net:-net);
                const char*yn[2]={"NO","YES"}; if(menu(q,yn,2)!=1) return 0;
                if(net>have){ toast("NOT ENOUGH SIMOLEONS"); return 0; }
                simMoneyAdd(-(money_t)net); simsSaveNow();
            }
            nbT.home=(u8)li; nbSave(); toast("WELCOME HOME"); return 0; }
        case A_CONV: {
            int toComm=L->kind==LKIND_RES;
            if(nbLives(li)){ toast("A HOUSEHOLD LIVES HERE"); return 0; }
            int t=0; if(toComm){ t=menu("WHAT KIND OF PLACE",ctNm,CT_N); if(t<0) return 0; }
            else { const char*yn[2]={"NO","YES"}; if(menu("MAKE IT A HOME LOT",yn,2)!=1) return 0; }
            L->kind=(u8)(toComm?LKIND_COMM:LKIND_RES); L->type=(u8)(toComm?t:0);
            if(L->slot<0&&nbT.cur==li) nbTemplate(li);   // (empty and live: it gets the starting layout of its new kind; a lot with a layout keeps what is built)
            nbSave(); toast(toComm?"NOW A COMMUNITY LOT":"NOW A HOME LOT"); return 0; }
        case A_ROOF: nbRoofEdit(li); return 0;
        case A_RENAME: { char nm[SLOT_NAME+1]; for(int i=0;i<=NB_NAME;i++) nm[i]=L->name[i]; if(slEditName(nm)){ for(int i=0;i<=NB_NAME;i++) L->name[i]=nm[i]; if(L->slot>=0) slRename(L->slot,nm); nbSave(); } return 0; }
        case A_TYPE: { int t=menu("WHAT KIND OF PLACE",ctNm,CT_N); if(t<0||t==L->type) return 0; L->type=(u8)t; if(L->slot<0&&nbT.cur==li) nbTemplate(li); nbSave(); return 0; }
        case A_BULL: {
            const char*yn[2]={"NO","YES"}; if(menu("TEAR IT ALL DOWN",yn,2)!=1) return 0;
            if(L->slot>=0){ slDelete(L->slot); L->slot=-1; }
            if(nbT.cur==li) nbTemplate(li); else L->value=(u16)(L->w*L->h*8);
            nbSave(); toast("BULLDOZED"); return 0; }
        case A_DEL: {
            if(li==nbT.home){ toast("YOU LIVE HERE"); return 0; }
            if(nbLives(li)){ toast("A HOUSEHOLD LIVES HERE"); return 0; }
            if(li==nbT.cur){ toast("GO TO ANOTHER LOT FIRST"); return 0; }
            const char*yn[2]={"NO","YES"}; if(menu("DELETE THIS LOT",yn,2)!=1) return 0;
            if(L->slot>=0) slDelete(L->slot);
            L->on=0; L->slot=-1; nbSave(); toast("LOT DELETED"); return 0; }
    }
    return 0;
}
static const char* const nbHelp[]={
    ">The Neighborhood",
    "A town is a map of lots, and each lot is a place to",
    "live, visit or build. You can explore the town you are",
    "in or make a new one of your own.",
    ">Shaping the Land",
    "Paint ground, lay roads, plant trees and",
    "decorations, and add new lots. A lot can be a",
    "home, a park, a skate park, a plaza, a lounge or",
    "part of the old town.",
    ">Moving In",
    "Choose a lot to play on, build on or move into.",
    "Moving in buys the lot and sells your old home. A lot",
    "is priced by its land and everything built on it.",
    ">Community Lots",
    "Parks, plazas and the like are places to visit rather",
    "than build in. To change one, work on it from this",
    "town view. Make Community and Make Residential",
    "switch a lot between the two.",
    ">The Town Menu",
    "From here you can zoom, change the season and time",
    "of day, and rename the town.",
    ">Handy Controls",
    "A on a lot offers Play, Build, Move In and Rename. L",
    "and R change tool, and SELECT picks its kind.",
    "START opens the town menu."};
static int nbNewLot(int x,int y,int w,int h){
    int i=0; while(i<NB_LOTS&&nbT.lot[i].on) i++;
    if(i>=NB_LOTS){ toast("THE TOWN IS FULL"); return 0; }
    static const char* const kinds[6]={"HOME LOT","PARK","SKATE PARK","PLAZA","LOUNGE","OLD TOWN"};
    int k=menu("WHAT GOES HERE",kinds,6); if(k<0) return 0;
    char nm[SLOT_NAME+1]; if(k==0) slNameN(nm,"LOT ",i+1); else { const char*s=kinds[k]; int j=0; for(;s[j]&&j<NB_NAME;j++) nm[j]=s[j]; nm[j]=0; }
    nbLotAdd(i,nm,x,y,w,h,k?LKIND_COMM:LKIND_RES,k?k-1:0);
    for(int j=y;j<y+h;j++)for(int q=x;q<x+w;q++) nbT.cell[j][q]&=7;   // (what stood there is cleared)
    nbSave(); toast("LOT PLACED"); return 1;
}
static void nbTownMenu(int*quit){
    static const char* const it[6]={"ZOOM","SEASON","TIME OF DAY","RENAME TOWN","ALL NEIGHBORHOODS","LEAVE"};
    int c=menu("TOWN",it,6);
    if(c==0) nbT.zoom^=1;
    else if(c==1) nbT.season=(u8)((nbT.season+1)&3);
    else if(c==2) nbT.tod=(u8)((nbT.tod+1)%3);
    else if(c==3){ char nm[SLOT_NAME+1]; for(int i=0;i<=NB_NAME;i++) nm[i]=nbT.name[i]; if(slEditName(nm)) for(int i=0;i<=NB_NAME;i++) nbT.name[i]=nm[i]; }
    else if(c==4||c==5) *quit=1;
}
static void neighborhoodScreen(void){
    nbBounds();
    armsLotGet();   // armsshop.h: the town gets its arms shop once
    int ccx=nbT.lot[nbT.home].on?nbT.lot[nbT.home].x+nbT.lot[nbT.home].w/2:5, ccy=nbT.lot[nbT.home].on?nbT.lot[nbT.home].y+nbT.lot[nbT.home].h/2:5;
    int tool=0, sub[5]={0,0,0,1,2}, hold[4]={0}, dirty=1, quit=0, played=0; u16 prev=keyNow();
    static const u16 dirK[4]={K_RIGHT,K_LEFT,K_UP,K_DOWN};
    while(!quit){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        int tr[4], fast=0; for(int i=0;i<4;i++){ hold[i]=(k&dirK[i])?hold[i]+1:0; tr[i]=(hold[i]==1)||hold[i]>3; if(hold[i]>12) fast=1; }   // (a pass is a whole redraw: held, the cursor steps every pass, and two cells after a while)
        int ux=tr[0]-tr[1], uy=tr[3]-tr[2];
        if(ux||uy){ int dx=ux+uy, dy=uy-ux; dx=(dx>0)-(dx<0); dy=(dy>0)-(dy<0); ccx+=dx<<fast; ccy+=dy<<fast;   // screen-relative, like the room builder
            if(ccx<0)ccx=0;
            if(ccy<0)ccy=0;
            if(ccx>=NB_W)ccx=NB_W-1;
            if(ccy>=NB_H)ccy=NB_H-1;
            dirty=1; }
        if(pr&K_R){ tool=(tool+1)%5; dirty=1; } if(pr&K_L){ tool=(tool+4)%5; dirty=1; }
        if(pr&K_SEL){ int n=tool==1?NT_ROAD:tool==3?DC_N:tool==4?6:1; sub[tool]=(sub[tool]+1)%n; dirty=1; }
        int gw=nbSizes[sub[4]][0], gh=nbSizes[sub[4]][1], gok=nbFree(ccx,ccy,gw,gh,-1);
        int onLot=nbAt(ccx,ccy)>=0;
        if((k&K_A)&&(tool==1||tool==2||tool==3)&&(pr&K_A||ux||uy)){   // painting: hold A and move
            if(onLot){ if(pr&K_A) toast("THAT IS A LOT"); }
            else { u8*c=&nbT.cell[ccy][ccx];
                if(tool==1) *c=(u8)((*c&~7)|sub[1]);
                else if(tool==2) *c=NT_ROAD;
                else *c=(u8)((*c&7)|(sub[3]<<3));
                if(NB_GR(*c)==NT_WATER||NB_GR(*c)==NT_ROAD){ int d=NB_DC(*c); if(d!=DC_LAMP&&d!=DC_FOUNTAIN&&d) *c&=7; }
                played=1; }
            dirty=1;
        }
        if(pr&K_B){ if(tool){ tool=0; dirty=1; } else quit=1; }
        if(pr&K_A){
            if(tool==0){ int li=nbAt(ccx,ccy); if(li>=0){ if(nbLotMenu(li)){ nbSave(); return; } }
                         else { tool=4; } prev=keyNow(); dirty=1; }
            else if(tool==4){ if(!gok) toast("NO ROOM THERE"); else if(nbNewLot(ccx,ccy,gw,gh)) tool=0; prev=keyNow(); dirty=1; }
        }
        if(pr&K_START){ nbTownMenu(&quit); prev=keyNow(); dirty=1; played=1; }
        if(dirty||(uiTicks&15)==0){ nbDrawTown(ccx,ccy,tool,gw,gh,gok); nbPanel(ccx,ccy,tool,sub[tool]); present(); dirty=0; } else vsync();
        uiTicks++; menuMusTick();
    }
    if(played) nbSave();
    while((~REG_KEYINPUT)&0x3FF) vsync();
}
// ---------- CHOOSE A NEIGHBORHOOD (the screen before the town: one thumbnail per town, made new from a style) ----------
static u16 nbThumbCol(const Town*t,int cx,int cy){
    for(int i=0;i<NB_LOTS;i++){ const NbLot*L=&t->lot[i]; if(!L->on||cx<L->x||cy<L->y||cx>=L->x+L->w||cy>=L->y+L->h) continue;
        int mx=cx-L->x, my=cy-L->y, mid=mx>=L->w/2-1&&mx<=L->w/2&&my>=L->h/2-1&&my<=L->h/2;
        if(L->kind==LKIND_COMM) return mid?RGB(12,12,14):RGB(19,19,18);
        if(mid&&(L->slot>=0||(t->NB_ACT&&t->cur==i))) return RGB(22,6,5);   // a house: its roof
        return mid&&L->slot<0?RGB(29,29,29):t->season==3?RGB(28,29,31):RGB(11,22,8); }
    u8 c=t->cell[cy][cx]; int g=NB_GR(c), d=NB_DC(c);
    if(d==DC_TREE||d==DC_PINE||d==DC_BUSH) return t->season==2?RGB(22,12,4):t->season==3?RGB(24,25,27):RGB(4,13,4);
    switch(g){ case NT_ROAD: return RGB(7,7,8); case NT_WATER: return t->season==3?RGB(22,26,30):RGB(6,13,25); case NT_SAND: return RGB(27,24,15);
        case NT_DIRT: return RGB(16,11,6); case NT_PLAZA: return RGB(19,19,18);
        default: { static const u16 gs[4]={RGB(10,21,8),RGB(8,18,5),RGB(17,16,6),RGB(27,28,30)}; return gs[t->season&3]; } }
}
static void nbThumb(const Town*t,int x0,int y0,int w,int h){   // the whole town, iso, w x h pixels
    rect(x0,y0,w,h,RGB(4,6,12));
    for(int py=0;py<h;py++)for(int px_=0;px_<w;px_++){
        int X2=(2*px_-w)*NB_W*2/w, Y2=py*NB_W*4/h;   // twice (cx-cy) and twice (cx+cy)
        int cx=(X2+Y2)/4, cy=(Y2-X2)/4; if(X2+Y2<0||Y2-X2<0||cx>=NB_W||cy>=NB_H) continue;
        px(x0+px_,y0+py,nbThumbCol(t,cx,cy)); }
}
static int nbSwitch(int s){   // make the town in slot s the one you live in (your lot there becomes the live room). 1 = done
    if(nbTS==s&&nbT.NB_ACT) return 1;
    if(nbLoad()){ if(nbT.cur<NB_LOTS&&nbT.lot[nbT.cur].on){ box(60,64,120,24); text(76,72,"PACKING UP...",WHITE,1); present();
            ldShow("PACKING UP YOUR LOT",0,3);
            int e=nbStore(nbT.cur); if(e){ nbErr=e==SLE_NOROOM?"NO FREE SLOTS FOR YOUR LOT":slErrMsg(e); ldEnd(); return 0; } }
        nbT.NB_ACT=0; nbSave(); }
    if(!nbRead(s,&nbT)){ nbErr="THAT TOWN IS DAMAGED"; nbLoad(); ldEnd(); return 0; }
    nbTS=s; int want=nbT.cur<NB_LOTS&&nbT.lot[nbT.cur].on?nbT.cur:nbT.home; nbT.cur=255; nbT.NB_ACT=1;
    if(!nbGo(want)){ nbSave(); ldEnd(); return 0; }
    return 1;
}
static int nbFirstTowns(int*l){   // the towns saved (their slots into l). The first time: three towns to start with (one on a 32 KB SRAM chip)
    int n=nbTownList(l,SLOT_MAX);
    if(!n){
        nbGen(NS_SUBURB,nsTown[NS_SUBURB]); nbT.cur=0; nbT.NB_ACT=1; nbTS=-1; nbSave();   // (the room you have is YOUR PLACE in BOREVILLE)
        if(SLOT_N>=26){ int keep=nbTS; for(int st=NS_DESERT;st<=NS_LAKE;st++){ nbGen(st,nsTown[st]); nbTS=-1; nbSave(); } nbTS=keep; }
        n=nbTownList(l,SLOT_MAX);
    }
    return n;
}
static int nbResetLot(void){ if(!nbOk||nbT.cur>=NB_LOTS||!nbT.lot[nbT.cur].on) return 0; nbTemplate(nbT.cur); return 1; }   // the room builder's RESET on a lot
static void nbBoot(void){ nbOk=nbLoad(); nbBounds(); }
