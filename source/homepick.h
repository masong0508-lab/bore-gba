// homepick.h - PICK YOUR HOME: after a NEW PLAYER finishes the creator, they choose where to live (like the Sims 2 neighborhood: land or a house, cheap ones).
//   Lists the free HOME lots of the town you can pay for with your starting cash (land = a bare lot, house = a lot somebody built on). The price is the lot's
//   value (neighborhood.h). A pays and moves in, B keeps the starter lot you were given. Lots another player calls home, or a household lives on, are left out.
// Needs: neighborhood.h (nbT, nbGo, nbMoney, nbWho, nbSave), savegame.h (sgList, sgHomeOf, sgPid), sims.h (simMoney, simsSaveNow).

static void homePick(void){
    if(!nbOk||nbT.home>=NB_LOTS) return;
    int l[SLOT_MAX], n=sgList(l); u8 used[NB_LOTS]; for(int i=0;i<NB_LOTS;i++) used[i]=0;
    for(int i=0;i<n;i++) if(slI[l[i]].pid!=sgPid){ int h=sgHomeOf(l[i]); if(h<NB_LOTS) used[h]=1; }   // (another player's home)
    static char lb[12][40] EWRAM_BSS; const char* it[12]; int lot[12], m=0;
    for(int li=0;li<NB_LOTS&&m<12;li++){
        const NbLot*L=&nbT.lot[li];
        if(!L->on||L->kind!=LKIND_RES||li==nbT.home||used[li]||nbWho(li,0)||nbPrice(L)>simMoney) continue;
        char*e=slCat(lb[m],L->name); e=slCat(e,L->slot>=0?"  HOUSE  ":"  LAND  "); nbMoney(e,nbPrice(L));
        it[m]=lb[m]; lot[m]=li; m++;
    }
    if(!m){ toast("NO HOME YOU CAN AFFORD"); return; }
    char t[40]; { char*e=slCat(t,"PICK A HOME  "); nbMoney(e,simMoney); }
    int c=menu(t,it,m); if(c<0) return;   // B: stay on the starter lot
    int li=lot[c], price=nbPrice(&nbT.lot[li]), old=nbT.home;
    nbT.home=(u8)li;
    if(!nbGo(li)){ nbT.home=(u8)old; nbSave(); toast(nbErr); return; }
    simMoneyAdd(-(money_t)price); simsSaveNow(); nbSave(); toast("WELCOME HOME");
}
