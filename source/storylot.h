// storylot.h - STORY LOTS (story step 3): every story starts in its own pre-made, furnished house instead of an empty lot.
// NEW GAME > STORY MODE calls storyHome(story) right after storySetup. The house is 14 x 12 tiles and fits any lot (a lot is at least 16 x 16 tiles):
//   top row, left to right: bedroom 1, bathroom, bedroom 2.   Below: the open living room with the kitchen corner on the left. The front door faces the street.
// Which lot: (1) the lot you call home when nothing is built on it (a new player's fresh lot), else (2) a free house lot nobody lives on and no other player calls home,
//   else (3) a new lot on free land. A home you built is never touched: it is stored as it is when you move to the new lot. No lot at all: the story starts as before.
// Needs: story.h (STY_ stories), neighborhood.h (nbT, nbGo, nbTemplate, nbSave, nbFree, nbLotAdd, nbRect, nbWho, nbValueLive, nbBounds), slots.h (sgPid, sgList, sgHomeOf, slI),
//        main.c (gRoom, gBox, gPut, gLine, flBlankUpper, mapSave, mapScan, hhSlotsFree, liveInvalidate, camSnap, wDirty, toast).

enum { SL_KIT=1, SL_SOFA=2, SL_SHELF=4, SL_PHONE=8, SL_RAIL=16, SL_BOX=32 };   // extras: coffee maker / sofa and TV / bookshelf / phone / skate rail and manual pad in the yard / moving crates
typedef struct { const char* name; u8 beds, sep, flags; } StLot;   // lot name (10 letters at most), beds (1-3), sep 1 = each bed in its own room first, flags
static const StLot stLots[STY_N]={
    {"",0,0,0},
    {"FLATSHARE",2,1,SL_KIT|SL_SOFA|SL_PHONE},         // ROOMMATES: two bedrooms, a shared kitchen and sofa
    {"LOVE NEST",2,0,SL_KIT|SL_SOFA|SL_SHELF},         // NEWLYWEDS: one bedroom for two, a spare room
    {"FAMILY ST",2,1,SL_KIT|SL_SOFA|SL_SHELF},         // SINGLE PARENT: a room each for you and the kid
    {"SKATE PAD",1,0,SL_SOFA|SL_RAIL},                 // SKATE LIFE: a rail and a manual pad out front
    {"FULL HOUSE",2,1,SL_KIT|SL_SOFA|SL_PHONE},        // HOUSEFULL
    {"THE CREW",3,1,SL_KIT|SL_SOFA|SL_PHONE},          // BEST FRIENDS: three beds
    {"BARE BONES",1,0,SL_BOX},                         // RAGS TO RICHES: a bed, a fridge and the plumbing, nothing else
    {"CLIMBER HQ",1,0,SL_KIT|SL_SOFA|SL_SHELF|SL_PHONE}, // CAREER CLIMBER: a desk-job flat with a phone and books
    {"NEW DIGS",2,1,SL_SOFA|SL_BOX},                   // NEW IN TOWN: still half unpacked
    {"2ND CHANCE",2,1,SL_KIT|SL_SOFA},                 // SECOND CHANCE
    {"SET HOUSE",2,1,SL_KIT|SL_SOFA|SL_SHELF|SL_PHONE},  // TV SHOW & TELL: a fashionable mansion (and a room for Mamesy)
};
_Static_assert(sizeof(stLots)/sizeof(stLots[0])==STY_N,"stLots needs one row per story (the STY_ enum order)");
static const u8 stLotStyle[STY_N]={0,HS_FAMILY,HS_MODERN,HS_FAMILY,HS_LOFT,HS_FAMILY,HS_FAMILY,HS_COTTAGE,HS_MODERN,HS_COTTAGE,HS_FAMILY,HS_MANSION};   // the kind of house each story gets (housegen.h); the plan, papers and furniture are new every time

static void stBuildHouse(int s,int x0,int y0,int x1,int y1){   // the story's furnished house on the live map, inside the lot rectangle (the street side is at the bottom): its own kind, a new plan each time
    const StLot*q=&stLots[s]; u32 seed=((u32)rnd8()<<8|(u32)rnd8())^(u32)(s*977);
    hgHouse(seed,stLotStyle[s],q->beds,q->sep,q->flags|(s==STY_RAGS?0x80:0),x0,y0,x1,y1);   // (RAGS TO RICHES: a bed, a fridge and the plumbing, nothing else)
}
static int stLotBare(int li){   // nothing is built on this lot of the live map (grass, planters, you and your board)
    int x0,y0,x1,y1; nbRect(&nbT.lot[li],&x0,&y0,&x1,&y1);
    for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++){ char c=lifeMap[y][x]; if(c!='.'&&c!='P'&&c!='B'&&c!='w'&&c!='Z') return 0; }
    return 1;
}
static void stStarterHouse(int li,int style){   // neighborhood.h MOVE IN: the lot's own house (housegen.h, the lot's seed: the house the panel promised), beds for who is moving in
    NbLot*L=&nbT.lot[li]; int x0,y0,x1,y1; nbRect(L,&x0,&y0,&x1,&y1);
    int beds=hhN+1>3?3:hhN+1;
    wDirty=1; hgHouse(nbLotSeed(li),style,beds,1,SL_KIT|SL_SOFA|SL_SHELF|SL_PHONE,x0,y0,x1,y1);
    flBlankUpper(); mapSave(); mapScan(); hhSlotsFree(); liveInvalidate(); camSnap=1; L->floors=1; nbValueLive(li); nbSave();
}
static void storyHome(int s){   // the story's own house becomes your home (and the live map). Does nothing when the town has no lot for it
    if(!nbOk||s<1||s>=STY_N||nbT.home>=NB_LOTS||!nbT.lot[nbT.home].on) return;
    int li=-1, fresh=0;
    if(nbT.cur==nbT.home&&nbT.lot[nbT.home].w>=4&&nbT.lot[nbT.home].h>=4&&stLotBare(nbT.home)){ li=nbT.home; fresh=1; }   // (1) your lot is still bare: the house goes right here
    if(li<0){
        int l[SLOT_MAX], n=sgList(l); u8 used[NB_LOTS]; for(int i=0;i<NB_LOTS;i++) used[i]=0;
        for(int i=0;i<n;i++) if(slI[l[i]].pid!=sgPid){ int h=sgHomeOf(l[i]); if(h<NB_LOTS) used[h]=1; }   // (another player's home)
        for(int k=0;k<NB_LOTS&&li<0;k++){ const NbLot*L=&nbT.lot[k];   // (2) a free house lot nobody built on or lives on
            if(L->on&&L->kind==LKIND_RES&&L->w>=4&&L->h>=4&&k!=nbT.home&&k!=nbT.cur&&L->slot<0&&!used[k]&&!nbWho(k,0)) li=k; }
        if(li<0){   // (3) a new lot on free land
            int i=0; while(i<NB_LOTS&&nbT.lot[i].on) i++;
            if(i<NB_LOTS) for(int sz=6;sz>=4&&li<0;sz--) for(int y=0;y+sz<=NB_H&&li<0;y++) for(int x=0;x+sz<=NB_W&&li<0;x++) if(nbFree(x,y,sz,sz,-1)){
                char nm[SLOT_NAME+1]; slNameN(nm,"HOME ",i+1); nbLotAdd(i,nm,x,y,sz,sz,LKIND_RES,0);
                for(int q=y;q<y+sz;q++) for(int p=x;p<x+sz;p++) nbT.cell[q][p]&=7;   // (what stood there is cleared)
                li=i; }
        }
        if(li<0){ toast("NO FREE LOT FOR THE STORY HOME"); return; }
        if(!nbGo(li)){ toast(nbErr); return; }   // the lot you leave is stored as it is
    }
    NbLot*L=&nbT.lot[li]; int x0,y0,x1,y1; nbRect(L,&x0,&y0,&x1,&y1);
    if(fresh) nbTemplate(li);   // (a clean start: the grass, the street and the spawn point)
    wDirty=1; stBuildHouse(s,x0,y0,x1,y1);
    flBlankUpper(); mapSave(); mapScan(); hhSlotsFree(); liveInvalidate(); camSnap=1; L->floors=1; nbValueLive(li);
    { const char*nm=stLots[s].name; int k=0; for(;nm[k]&&k<NB_NAME;k++) L->name[k]=nm[k]; L->name[k]=0; }
    nbT.home=(u8)li; nbBounds(); nbSave();
}
