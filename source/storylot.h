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
    {"SET HOUSE",1,0,SL_KIT|SL_SOFA|SL_SHELF|SL_PHONE},  // TV SHOW & TELL
};
_Static_assert(sizeof(stLots)/sizeof(stLots[0])==STY_N,"stLots needs one row per story (the STY_ enum order)");

static void stBuildHouse(int s,int x0,int y0,int x1,int y1){   // the furnished house on the live map, inside the lot rectangle x0..x1 y0..y1 (the street side is at the bottom)
    const StLot*q=&stLots[s]; int cx=(x0+x1)/2, hx=cx-6, hy=y1-14;
    gRoom(hx,hy,hx+13,hy+11,1,NWP+61); gBox(hx+1,hy+6,hx+4,hy+10,3);   // the house: carpet and peach walls, lino under the kitchen corner
    gRoom(hx,hy,hx+5,hy+5,1,NWP+61);        // bedroom 1
    gRoom(hx+5,hy,hx+9,hy+5,5,NWP+57);      // bathroom (pink tile)
    gRoom(hx+9,hy,hx+13,hy+5,1,NWP+61);     // bedroom 2
    gPut(hx+3,hy+5,'D'); gPut(hx+7,hy+5,'D'); gPut(hx+11,hy+5,'D'); gPut(cx,hy+11,'D');   // three inner doors and the front door
    static const u8 bpSep[3][2]={{1,1},{10,1},{4,1}}, bpShr[3][2]={{1,1},{4,1},{10,1}};   // where the 1st, 2nd and 3rd bed go
    for(int i=0;i<q->beds&&i<3;i++){ const u8*p=q->sep?bpSep[i]:bpShr[i]; gPut(hx+p[0],hy+p[1],'S'); }
    gPut(hx+6,hy+1,'T'); gPut(hx+8,hy+1,'H');   // toilet and shower
    gPut(hx+1,hy+10,'F'); if(q->flags&SL_KIT) gPut(hx+2,hy+10,'c');   // fridge (and coffee maker)
    if(q->flags&SL_SOFA){ gPut(hx+7,hy+8,'C'); gPut(hx+7,hy+10,'v'); }   // sofa and TV, one tile apart
    if(q->flags&SL_SHELF) gPut(hx+12,hy+7,'b');
    if(q->flags&SL_PHONE) gPut(hx+12,hy+10,'I');
    if(q->flags&SL_BOX){ gPut(hx+11,hy+9,'#'); gPut(hx+12,hy+9,'#'); gPut(hx+11,hy+10,'#'); }   // moving crates
    if(q->flags&SL_RAIL){ gLine(x0+1,y1-1,x0+4,y1-1,'=',0); gPut(x1-3,y1-1,'M'); gPut(x1-2,y1-1,'M'); }   // the yard: a rail and a manual pad (your spot, 'P', is at the middle)
}
static int stLotBare(int li){   // nothing is built on this lot of the live map (grass, planters, you and your board)
    int x0,y0,x1,y1; nbRect(&nbT.lot[li],&x0,&y0,&x1,&y1);
    for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++){ char c=lifeMap[y][x]; if(c!='.'&&c!='P'&&c!='B'&&c!='w'&&c!='Z') return 0; }
    return 1;
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
