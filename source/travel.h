// travel.h - GETTING ABOUT: CALL A CAB on the phone (The Sims 2's taxi) and the story journal's GO THERE take you to another lot of the town
// without leaving the game. The trip is the prison's own lot change (prison.h prLifeSwitch / prTransfer, prGo code 3): the lot you leave is
// stored, the new one is loaded and play carries on there with the same Sim, needs and mood.
//   trvFind(ct)   a community lot of kind ct, or -1. A skate park or a park is also found as a PARK + SKATE lot (it is both).
//   trvPlace(ct)  the same, but a town without one gets one (on free land, or a lot nobody built on and nobody lives on): a story never
//                 sends you to a place that is not there. The prison and the arms shop are made by their own code (prLotGet, armsLotGet).
//   trvGo(lot)    asks for the trip (1 = on the way: the main loop does it at the end of this step). Not from the prison while you are
//                 doing time, not from a test play, not without a town.
// Needs: neighborhood.h (nbT, nbOk, nbAt, nbLotAdd, nbSave, nbLives, ctNm), prison.h (prGo, prIn, prLotGet), armsshop.h (armsLotGet, later: declared).
static u8 trvLot=255;    // where the cab goes (prGo code 3)
static int armsLotGet(void);   // armsshop.h
static int trvIs(const NbLot*L,int ct){ return L->on&&L->kind==LKIND_COMM&&(L->type==ct||((ct==CT_SKATE||ct==CT_PARK)&&L->type==CT_BOTH)); }
static int trvFind(int ct){
    if(!nbOk) return -1;
    if(nbT.cur<NB_LOTS&&trvIs(&nbT.lot[nbT.cur],ct)) return nbT.cur;   // (the one you are on first)
    for(int i=0;i<NB_LOTS;i++) if(trvIs(&nbT.lot[i],ct)) return i;
    return -1;
}
static int trvHere(int ct){ return nbOk&&nbT.cur<NB_LOTS&&trvIs(&nbT.lot[nbT.cur],ct); }   // are you on a place of that kind right now?
static int trvPlace(int ct){
    int i=trvFind(ct); if(i>=0||!nbOk) return i;
    if(ct==CT_PRISON) return prLotGet();
    if(ct==CT_ARMS){ armsLotGet(); return trvFind(ct); }
    int fi=-1; for(int k=0;k<NB_LOTS&&fi<0;k++) if(!nbT.lot[k].on) fi=k;
    if(fi>=0) for(int sz=6;sz>=4;sz--) for(int y=0;y+sz<=NB_H;y++) for(int x=0;x+sz<=NB_W;x++){
        int ok=1; for(int yy=y;yy<y+sz&&ok;yy++) for(int xx=x;xx<x+sz;xx++) if(nbAt(xx,yy)>=0||NB_GR(nbT.cell[yy][xx])==NT_WATER){ ok=0; break; }
        if(!ok) continue;
        nbLotAdd(fi,ctNm[ct],x,y,sz,sz,LKIND_COMM,ct);
        for(int yy=y;yy<y+sz;yy++) for(int xx=x;xx<x+sz;xx++) nbT.cell[yy][xx]=(u8)NT_PLAZA;
        nbSave(); return fi; }
    for(int k=0;k<NB_LOTS;k++){ NbLot*L=&nbT.lot[k];   // no free land: a lot nobody built on and nobody lives on becomes the place
        if(L->on&&L->kind==LKIND_RES&&k!=nbT.home&&k!=nbT.cur&&L->slot<0&&!nbLives(k)){
            L->kind=LKIND_COMM; L->type=(u8)ct; const char*nm=ctNm[ct]; int q=0; for(;nm[q]&&q<NB_NAME;q++) L->name[q]=nm[q]; L->name[q]=0; nbSave(); return k; } }
    return -1;
}
static int trvGo(int lot){
    if(!nbOk||lot<0||lot>=NB_LOTS||!nbT.lot[lot].on){ toast("NOWHERE TO GO"); return 0; }
    if(prIn()){ toast("NOT WHILE YOU ARE DOING TIME"); return 0; }
    if(lot==nbT.cur){ toast("YOU ARE ALREADY HERE"); return 0; }
    trvLot=(u8)lot; prGo=3; return 1;
}
static int trvHas(const char*s,const char*w){   // does the name already say the kind? (the last word of the kind in it: PARK, LOUNGE, TOWN ...)
    const char*k=w; for(const char*p=w;*p;p++) if(*p==' ') k=p+1;
    for(;*s;s++){ int i=0; while(k[i]&&s[i]==k[i]) i++; if(!k[i]) return 1; } return 0; }
static void trvMenu(void){   // the phone: CALL A CAB  GO SOMEWHERE (home first, then every public place of the town)
    if(!nbOk){ toast("NO TOWN TO GO ABOUT IN"); return; }
    const char* it[NB_LOTS+1]; int lot[NB_LOTS+1], n=0; char lb[NB_LOTS+1][28];
    if(nbT.home<NB_LOTS&&nbT.home!=nbT.cur){ char*e=slCat(lb[n],"HOME  "); slCat(e,nbT.lot[nbT.home].name); it[n]=lb[n]; lot[n++]=nbT.home; }
    for(int i=0;i<NB_LOTS;i++){ const NbLot*L=&nbT.lot[i]; if(!L->on||L->kind!=LKIND_COMM||i==nbT.cur) continue;
        if(L->type==CT_PRISON&&!prIn()) continue;   // (nobody takes a cab to the prison)
        char*e=slCat(lb[n],L->name); if(L->type<CT_N&&!trvHas(L->name,ctNm[L->type])){ e=slCat(e,"  "); slCat(e,ctNm[L->type]); } it[n]=lb[n]; lot[n++]=i; }   // ("MAIN SQUARE  PLAZA", but not "SKATE PARK  SKATE PARK")
    if(!n){ toast("NO PUBLIC PLACES IN THIS TOWN YET"); return; }
    int c=menu("CALL A CAB  WHERE TO",it,n); if(c<0) return;
    if(trvGo(lot[c])){ sfxPlay(SFX_CASH); toast("THE CAB IS HERE"); }
}
