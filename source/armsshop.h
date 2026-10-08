// armsshop.h - WEAPONS (module 3: the ARMS SHOP). A community lot of the town (type CT_ARMS) with a counter ('g'). Stand next to the counter and press R:
// the shop menu sells every weapon once (BAT, KNIFE, TASER, PISTOL, UZI, ROCKET) and ammo, for simoleons. Teens and up only.
// THE LOT   armsLotGet builds it on free land once per town (Town.pad[1] bit 0: if you turn it into something else it does not come back),
//           or you can make any community lot into one (the lot menu's WHAT KIND OF PLACE). armsBuild lays it out (called by nbTemplate).
// THE COUNTER  drawn in code (no sprite, no RAM), 6 px high like a bench, so bullets fly over it and Sims cannot walk through.
static void armsBuild(int x0,int y0,int x1,int y1){   // nbTemplate: a shop room in the lot, a counter across it, you arrive outside the door
    int cx=(x0+x1)/2, rx0=x0+1, ry0=y0+1, rx1=x1-1, ry1=y1-3;
    gBox(x0,y0,x1,y1,3); gRoom(rx0,ry0,rx1,ry1,2,NWP+57); gPut(cx,ry1,'D');
    for(int x=cx-2;x<=cx+2;x++) gPut(x,ry0+3,'g');
    gPut(rx0+1,ry0+1,'Z'); gPut(rx1-1,ry0+1,'Z'); gPut(rx0+1,ry1-1,'Z'); gPut(rx1-1,ry1-1,'Z');
}
static int armsLotGet(void){   // the town's arms shop, made once if there is none (-1: nothing made)
    if(!nbOk||(nbT.pad[1]&1)) return -1;
    nbT.pad[1]|=1;
    for(int i=0;i<NB_LOTS;i++){ const NbLot*L=&nbT.lot[i]; if(L->on&&L->kind==LKIND_COMM&&L->type==CT_ARMS){ nbSave(); return i; } }
    int fi=-1; for(int i=0;i<NB_LOTS&&fi<0;i++) if(!nbT.lot[i].on) fi=i;
    if(fi>=0) for(int sz=4;sz>=3;sz--) for(int y=0;y+sz<=NB_H;y++) for(int x=0;x+sz<=NB_W;x++){
        int ok=1; for(int yy=y;yy<y+sz&&ok;yy++) for(int xx=x;xx<x+sz;xx++) if(nbAt(xx,yy)>=0||NB_GR(nbT.cell[yy][xx])==NT_WATER){ ok=0; break; }
        if(!ok) continue;
        nbLotAdd(fi,"ARMS",x,y,sz,sz,LKIND_COMM,CT_ARMS);
        for(int yy=y;yy<y+sz;yy++) for(int xx=x;xx<x+sz;xx++) nbT.cell[yy][xx]=(u8)NT_PLAZA;
        nbSave(); return fi; }
    for(int i=0;i<NB_LOTS;i++){ NbLot*L=&nbT.lot[i];   // no free land: a lot nobody built on and nobody lives on
        if(L->on&&L->kind==LKIND_RES&&i!=nbT.home&&i!=nbT.cur&&L->slot<0&&!nbLives(i)){
            L->kind=LKIND_COMM; L->type=CT_ARMS; const char*nm="ARMS"; int k=0; for(;nm[k]&&k<NB_NAME;k++) L->name[k]=nm[k]; L->name[k]=0; nbSave(); return i; } }
    nbSave(); return -1;
}
static void drawCounter(int sx,int sy,int t){   // the shop counter: a dark wood block with a gun on it, and a blinking red light
    rect(sx-8,sy-9,17,8,RGB(9,5,3)); rect(sx-8,sy-10,17,2,RGB(20,14,7)); rect(sx-8,sy-2,17,1,RGB(4,2,1));
    rect(sx-6,sy-13,7,2,RGB(6,6,8)); rect(sx-6,sy-11,2,3,RGB(6,6,8)); rect(sx-4,sy-14,2,1,RGB(14,14,16));
    if((t>>2)&1) rect(sx+3,sy-14,2,2,RGB(31,6,4));
}
static int armsNear(void){ int tx=(int)(lfx>>8), ty=(int)(lfy>>8);
    for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){ int x=tx+dx, y=ty+dy; if(x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]=='g') return 1; }
    return 0; }
static int armsUse(void){   // R next to the counter (weapons.h wpUseSpot)
    if(!armsNear()) return 0;
    if(stage<AG_TEEN){ lnote="NOT FOR KIDS"; lnoteT=50; return 1; }
    static char lb[9][28] EWRAM_BSS, tt[24] EWRAM_BSS; const char*it[9];
    for(;;){
        char*e=simCat(tt,"ARMS SHOP  $"); simCatN(e,simMoney);
        for(int w=0;w<WP_N;w++){ char*q=simCat(lb[w],wpT[w].nm); q=simCat(q,"  "); if(wpOwn>>w&1) simCat(q,"OWNED"); else { *q++='$'; simCatN(q,wpT[w].price); } it[w]=lb[w]; }
        { char*q=simCat(lb[6],"12 BULLETS  $40"); (void)q; it[6]=lb[6]; q=simCat(lb[7],"3 MISSILES  $150"); (void)q; it[7]=lb[7]; it[8]="LEAVE"; }
        int c=menu(tt,it,9); if(c<0||c==8) break;
        int price=c<WP_N?wpT[c].price:c==6?40:150;
        if(c<WP_N&&(wpOwn>>c&1)) toast("YOU HAVE ONE");
        else if(c==6&&wpBul>=WP_BMAX) toast("POCKETS FULL");
        else if(c==7&&wpMis>=WP_MMAX) toast("POCKETS FULL");
        else if(simMoney<price) toast("NOT ENOUGH CASH");
        else { simMoney-=price;
            if(c<WP_N) wpGive(c,c==WP_PISTOL||c==WP_UZI?12:0,c==WP_ROCKET?2:0);
            else { if(c==6) wpBul=(u8)(wpBul+12>WP_BMAX?WP_BMAX:wpBul+12); else wpMis=(u8)(wpMis+3>WP_MMAX?WP_MMAX:wpMis+3); wpSave(); }   // ammo alone never makes a weapon
            simsSaveNow(); toast("SOLD"); }
    }
    liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
    return 1;
}
