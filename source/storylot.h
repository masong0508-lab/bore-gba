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
    {"FIXER UP",1,0,SL_KIT|SL_BOX},   // MOVING IN
    {"RAMP PAD",1,0,SL_SOFA|SL_RAIL},   // DROP IN
    {"TACO FLAT",1,0,SL_KIT|SL_SOFA|SL_PHONE},   // TACO TUESDAY
    {"THE STOOP",2,1,SL_KIT|SL_SOFA|SL_PHONE},   // ICEBREAKER
    {"LOVE SHACK",1,0,SL_KIT|SL_SOFA},   // SPARKS
    {"THE OFFICE",1,0,SL_KIT|SL_SOFA|SL_SHELF|SL_PHONE},   // NINE TO FIVE
    {"PET HOUSE",2,1,SL_KIT|SL_SOFA|SL_SHELF},   // GOOD BOY
    {"THE DOJO",1,0,SL_SOFA|SL_SHELF},   // FIGHT NIGHT
    {"OPEN DOORS",3,1,SL_KIT|SL_SOFA|SL_PHONE},   // OPEN HOUSE
    {"THE GARDEN",1,0,SL_KIT|SL_SOFA|SL_SHELF},   // GREEN THUMB
};
_Static_assert(sizeof(stLots)/sizeof(stLots[0])==STY_N,"stLots needs one row per story (the STY_ enum order)");
static const u8 stLotStyle[STY_N]={0,HS_FAMILY,HS_MODERN,HS_FAMILY,HS_LOFT,HS_FAMILY,HS_FAMILY,HS_COTTAGE,HS_MODERN,HS_COTTAGE,HS_FAMILY,HS_MANSION,HS_COTTAGE,HS_LOFT,HS_MODERN,HS_FAMILY,HS_COTTAGE,HS_MODERN,HS_FAMILY,HS_LOFT,HS_FAMILY,HS_COTTAGE};   // the kind of house each story gets (housegen.h); the plan, papers and furniture are new every time

// ---- SET HOUSE: Missy's mansion (TV SHOW & TELL) ----
// A purpose-built plan instead of a generated one: the great room (a bar, a fireplace, two sofas and the TV, the trophy cases, the clock) and the kitchen with a dining
// table across the front, and along the back the master suite, her bathroom, the STUDIO (vanities, a mannequin, the awards, a keyboard) and a room for MAMESY (a lot of lots
// are small: the studio goes first). A low garden fence round the lot with an open gateway (SECURITY GATES lock it, chapter 3), a fountain on the path, topiaries and beds.
static int sCh(const char*nm){ for(int i=0;i<CAT_N;i++){ const char*a=catTab[i].nm,*b=nm; while(*a&&*a==*b){ a++; b++; } if(!*a&&!*b) return catTab[i].ch; } return 0; }
static int sPut(int x0,int y0,int x1,int y1,int side,const char*nm){   // a named catalog piece against a wall of the room (side 1: the back wall only), where the house can still be walked
    int ch=sCh(nm), lk=hgLookNm(nm); if(!ch) return 0;
    u16 cand[96]; int n=0;
    for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){ int edge=side==1?y==y0:(x==x0||x==x1||y==y0||y==y1);
        if(edge&&lifeMap[y][x]=='.'&&!hgNearDoor(x,y)&&n<96) cand[n++]=(u16)(y*MW+x); }
    while(n>0){ int k=hgRnd(n), p=cand[k]; cand[k]=cand[--n]; int x=p%MW, y=p/MW;
        lifeMap[y][x]=(char)ch; wallMap[y][x]=(u8)lk; if(hgOk()) return 1; lifeMap[y][x]='.'; wallMap[y][x]=0; }
    return 0;
}
static int sAt(int x,int y,const char*nm){ if(x<0||y<0||x>=MW||y>=MH||lifeMap[y][x]!='.') return 0; int ch=sCh(nm); if(!ch) return 0;
    lifeMap[y][x]=(char)ch; wallMap[y][x]=(u8)hgLookNm(nm); if(hgOk()) return 1; lifeMap[y][x]='.'; wallMap[y][x]=0; return 0; }
static void stSetHouse(int x0,int y0,int x1,int y1){
    hgR=0x5E7C0DE5u; hgRnd(1);
    int lw=x1-x0+1, lh=y1-y0+1, wd=lw-2, ht=lh-4; if(wd>22) wd=22; if(ht>15) ht=15; if(wd<14) wd=14; if(ht<11) ht=11;
    int hx0=x0+(lw-wd)/2, hx1=hx0+wd-1, hy1=y1-3, hy0=hy1-ht+1; if(hy0<y0+1){ hy0=y0+1; ht=hy1-hy0+1; }
    for(int y=hy0;y<=hy1;y++)for(int x=hx0;x<=hx1;x++) if(lifeMap[y][x]!='P') lifeMap[y][x]='.';
    int py=hy0+6;   // the wall between the back rooms and the front
    // the back rooms: 0 master, 1 bath, 2 studio, 3 Mamesy's room (the studio only when the house is wide enough)
    u8 kd[4]; int nr=0, w[4]; kd[nr]=0; w[nr++]=6; kd[nr]=1; w[nr++]=4; if(wd>=21){ kd[nr]=2; w[nr++]=5; } kd[nr]=3; w[nr++]=5;
    { int need=0; for(int i=0;i<nr;i++) need+=w[i]; int extra=wd-1-need;
      while(extra<0){ if(w[0]>5){ w[0]--; extra++; } else if(w[1]>3){ w[1]--; extra++; } else if(w[nr-1]>4){ w[nr-1]--; extra++; } else break; }
      if(extra>0){ w[0]+=extra/2; w[nr-1]+=extra-extra/2; } }
    int xs[6]; xs[0]=hx0; for(int i=0;i<nr;i++) xs[i+1]=xs[i]+w[i]; xs[nr]=hx1;
    int kw=wd>=18?7:6, kx=hx1-kw;   // the kitchen is on the right, the great room on the left
    // walls and floors: red tile and velvet in the great room (the show-off room), carpet and quiet papers in the bedrooms
    gRoom(hx0,hy0,hx1,hy1,4,NWP+41);
    for(int i=0;i<nr;i++){ static const u8 fl[4]={1,6,4,2}, wp[4]={56,57,24,66}; gRoom(xs[i],hy0,xs[i+1],py,fl[kd[i]],NWP+wp[kd[i]]); }
    gRoom(kx,py,hx1,hy1,3,NWP+48);
    for(int i=0;i<nr;i++){ int a=xs[i]+1, b=xs[i+1]-1, x=a+(b-a)/2; if(x==kx) x--; lifeMap[py][x]='D'; }
    { int y=py+2; lifeMap[y][kx]='D'; }
    { int fx=(hx0+kx)/2; lifeMap[hy1][fx]='D'; hgFX=fx; hgFY=hy1; }
    hgX0=hx0; hgY0=hy0; hgX1=hx1; hgY1=hy1+1;
    hgWindows(hx0,hy0,hx1,hy1,'e');
    // the back rooms
    for(int i=0;i<nr;i++){ int rx0=xs[i]+1, rx1=xs[i+1]-1, ry0=hy0+1, ry1=py-1;
        switch(kd[i]){
        case 0: sPut(rx0,ry0,rx1,ry1,1,"KING BED"); sPut(rx0,ry0,rx1,ry1,0,"WHITE WARDROBE"); sPut(rx0,ry0,rx1,ry1,0,"BLACK DRESSER"); sPut(rx0,ry0,rx1,ry1,0,"TABLE LAMP"); sPut(rx0,ry0,rx1,ry1,0,"FERN"); break;
        case 1: sPut(rx0,ry0,rx1,ry1,1,"GOLD TOILET"); sPut(rx0,ry0,rx1,ry1,1,"CLAWFOOT TUB"); if(!sPut(rx0,ry0,rx1,ry1,0,"GLASS SHOWER")) sPut(rx0,ry0,rx1,ry1,0,"BATHROOM SINK"); sPut(rx0,ry0,rx1,ry1,0,"POTTED PALM"); break;
        case 2: sPut(rx0,ry0,rx1,ry1,1,"VANITY"); sPut(rx0,ry0,rx1,ry1,1,"VANITY"); sPut(rx0,ry0,rx1,ry1,0,"TROPHY CASE"); sPut(rx0,ry0,rx1,ry1,0,"MANNEQUIN"); sPut(rx0,ry0,rx1,ry1,0,"WHITE WARDROBE"); sPut(rx0,ry0,rx1,ry1,0,"SYNTHESIZER"); sPut(rx0,ry0,rx1,ry1,0,"BRASS LAMP"); break;
        default: sPut(rx0,ry0,rx1,ry1,1,"TWIN BED GREEN"); sPut(rx0,ry0,rx1,ry1,0,"PINE DRESSER"); sPut(rx0,ry0,rx1,ry1,0,"GUITAR STAND"); sPut(rx0,ry0,rx1,ry1,0,"TABLE LAMP"); sPut(rx0,ry0,rx1,ry1,0,"FERN"); break; } }
    if(wd<21){ /* no studio room: the vanity and the awards go in the great room */ }
    // the kitchen and the dining table
    { int rx0=kx+1, rx1=hx1-1, ry0=py+1, ry1=hy1-1;
      sPut(rx0,ry0,rx1,ry1,0,"STEEL FRIDGE"); sPut(rx0,ry0,rx1,ry1,0,"STOVE"); sPut(rx0,ry0,rx1,ry1,0,"KITCHEN SINK"); sPut(rx0,ry0,rx1,ry1,0,"MARBLE COUNTER"); sPut(rx0,ry0,rx1,ry1,0,"MARBLE COUNTER"); sPut(rx0,ry0,rx1,ry1,0,"KETTLE STAND");
      hgTable(rx0,ry0,rx1,ry1,4); }
    // the great room
    { int rx0=hx0+1, rx1=kx-1, ry0=py+1, ry1=hy1-1;
      sPut(rx0,ry0,rx1,ry1,0,"VELVET SOFA"); sPut(rx0,ry0,rx1,ry1,0,"BIG TV"); sPut(rx0,ry0,rx1,ry1,0,"LEATHER SOFA");
      hgPut(rx0,ry0,rx1,ry1,1,'o');   // the fireplace
      sPut(rx0,ry0,rx1,ry1,0,"BAR COUNTER"); sPut(rx0,ry0,rx1,ry1,0,"BAR STOOL"); sPut(rx0,ry0,rx1,ry1,0,"BAR STOOL"); sPut(rx0,ry0,rx1,ry1,0,"BAR STOOL"); sPut(rx0,ry0,rx1,ry1,0,"WINE FRIDGE");
      sPut(rx0,ry0,rx1,ry1,0,"TROPHY CASE"); sPut(rx0,ry0,rx1,ry1,0,"TROPHY CASE"); sPut(rx0,ry0,rx1,ry1,0,"GRANDFATHER CLOCK"); sPut(rx0,ry0,rx1,ry1,0,"MARBLE BUST");
      if(wd<21){ sPut(rx0,ry0,rx1,ry1,0,"VANITY"); sPut(rx0,ry0,rx1,ry1,0,"SYNTHESIZER"); }
      sPut(rx0,ry0,rx1,ry1,0,"BIG SPEAKERS"); sPut(rx0,ry0,rx1,ry1,0,"POTTED PALM"); sPut(rx0,ry0,rx1,ry1,0,"POTTED PALM"); sPut(rx0,ry0,rx1,ry1,0,"FICUS"); sPut(rx0,ry0,rx1,ry1,0,"CHANDELIER"); sPut(rx0,ry0,rx1,ry1,0,"PORTRAIT");
      { int cx=(rx0+rx1)/2, cy=(ry0+ry1)/2; if(lifeMap[cy][cx]=='.') lifeMap[cy][cx]='r'; sAt(cx,cy+1,"GLASS TABLE"); } }
    // the yard: a low garden fence with the gateway at the front, a fountain by the path, topiaries and beds
    { int gx=(x0+x1)/2;
      for(int x=x0;x<=x1;x++){ if(lifeMap[y0][x]=='.') lifeMap[y0][x]='w'; if((x<gx-2||x>gx+2)&&lifeMap[y1][x]=='.') lifeMap[y1][x]='w'; }
      for(int y=y0;y<=y1;y++){ if(lifeMap[y][x0]=='.') lifeMap[y][x0]='w'; if(lifeMap[y][x1]=='.') lifeMap[y][x1]='w'; }
      for(int x=x0;x<=x1;x++){ wallMap[y0][x]=13; wallMap[y1][x]=13; } for(int y=y0;y<=y1;y++){ wallMap[y][x0]=13; wallMap[y][x1]=13; }
      int fy=y1-1, fxs=hgFX<gx?gx+3:gx-3; sAt(fxs,fy,"FOUNTAIN"); sAt(x0+2,fy,"TOPIARY BALL"); sAt(x1-2,fy,"TOPIARY BALL"); sAt(x0+1,fy-1,"ROSE BUSH"); sAt(x1-1,fy-1,"ROSE BUSH");
      sAt(hx0-1,hy1+1,"TOPIARY CONE"); sAt(hx1+1,hy1+1,"TOPIARY CONE"); sAt(hx0+2,hy1+1,"RED FLOWER BED"); sAt(hx1-2,hy1+1,"YELLOW FLOWER BED");
      for(int x=hx0+1;x<hx1;x+=3) sAt(x,y0+1,x&1?"TULIP BED":"BOXWOOD"); }
}
static void stBuildHouse(int s,int x0,int y0,int x1,int y1){   // the story's furnished house on the live map, inside the lot rectangle (the street side is at the bottom): its own kind, a new plan each time
    const StLot*q=&stLots[s]; u32 seed=((u32)rnd8()<<8|(u32)rnd8())^(u32)(s*977);
    if(s==STY_TVSHOW){ stSetHouse(x0,y0,x1,y1); return; }   // (Missy's mansion has its own plan)
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
