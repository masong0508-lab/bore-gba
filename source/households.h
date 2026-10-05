// households.h - THE TOWN'S HOUSEHOLDS (The Sims 2): who lives on the town's lots, the HOUSEHOLD BANK (the households you are not playing
// right now), playing another household, starting a new one on a free lot, the VISITORS who come by from them, and the PHONE.
//
// WHO LIVES WHERE  Your household lives on your home lot. Every other residential lot holds, in this order: a household from the bank
//                 (one you played and left there, or a Sim who moved out), else one of the pre-made families (nbFamOf: two lots in three, given
//                 out in lot order from a start that depends on the town's name, so a lot always has the same family), else nobody: a FREE LOT
//                 where NEW HOUSEHOLD HERE starts new Sims.
// THE BANK        Save memory kept apart from the room slots, for other households (bkInit, at power on): the top 16 slots of 128 KB flash
//                 (8 households), 8 of 64 KB flash (4), none on 32 KB SRAM. It only takes slots no save is using, so nothing is ever moved or
//                 lost; the room slot screen simply ends below it. A record (BK_SZ bytes) is either a whole household you played (kind 1:
//                 your look and, byte for byte, the age, persona, life and household blocks of save memory, so it comes back exactly as it was
//                 left) or one Sim who moved out (kind 2: look, age, aspiration, traits and names; they start a new life when you play them).
// VISITORS        house.h's twTick walks them in, lets them stay a while and walks them off; nbVisitor picks who (a Sim of another household).
// THE PHONE       pause menu > PHONE: invite someone over, order pizza (it turns out to be DiGiorno) or Chinese food, and (with the debug
//                 code, like every other change to who lives in the house) move a Sim out to a free lot of the town.
// Needs before it: neighborhood.h, slots.h, house.h, sims.h, the persona / age blocks (main.c), lifeMode, creatureEditor, the UI kit.

// ---------- the bank ----------
#define BK_SZ  4096
#define BK_HDR 40   // 0 'H' 1 'B' (written last) 2 kind 3 lot 4..5 town key 6..7 payload length 8..9 checksum 10..21 last name 22..33 first name 34 stage
                    // 35 how many look bytes the payload starts with (LK_N; anything else: a record written before GENDER, LK_N10)
static int bkN;     // households the bank holds
static u32 bkOff(int i){ return SLOT_BASE+(u32)(slN+2*i)*SLOT_SZ; }   // right after the room slots
static void bkInit(void){   // at power on, after slInitN: the top slots that no save uses become the bank (slN ends below it)
    int total=slN, want=total>=58?16:total>=26?8:0; bkN=0; if(!want) return;
    slScan(); int top=0; for(int s=0;s<total;s++) if(slOwner[s]>=0) top=s+1;   // (a save up there keeps its slot: the bank is smaller)
    int first=total-want; if(first<top) first=top; if((total-first)&1) first++;
    bkN=(total-first)/2; slN=first;
}
static int bkOk(int i){ u32 o=bkOff(i); return svRd(o)=='H'&&svRd(o+1)=='B'&&svRd(o+2)>=1&&svRd(o+2)<=2; }
static int bkFind(u16 key,int lot){ for(int i=0;i<bkN;i++){ u32 o=bkOff(i); if(bkOk(i)&&(u16)(svRd(o+4)|svRd(o+5)<<8)==key&&svRd(o+3)==lot) return i; } return -1; }
static int bkFree(void){ for(int i=0;i<bkN;i++) if(!bkOk(i)) return i; return -1; }
static void bkDel(int i){ svErase(bkOff(i),BK_SZ); }
static void bkName(int i,int first,char*d){ u32 o=bkOff(i)+(first?22:10); int k=0; for(;k<HH_NM-1;k++){ char c=(char)svRd(o+k); if(!c) break; d[k]=c; } d[k]=0; }
static u16 nbKey(const Town*t){ u16 h=0x5A3C; for(int i=0;i<NB_NAME&&t->name[i];i++) h=(u16)(h*31+(u8)t->name[i]); return h; }   // a town by its name
// writing a record: the header's fields, then the payload byte by byte (the checksum is counted on the way), the magic last
static u32 bkW; static u16 bkSum;
static void bkPut8(int v){ svWr(bkW++,v&255); bkSum=(u16)(bkSum+(v&255)); }
static void bkHead(int i,int kind,int lot,u16 key,const char*last,const char*first,int stage){
    u32 o=bkOff(i), n=bkW-o-BK_HDR;
    svWr(o+2,kind); svWr(o+3,lot); svWr(o+4,key&255); svWr(o+5,key>>8); svWr(o+6,(int)(n&255)); svWr(o+7,(int)(n>>8)); svWr(o+8,bkSum&255); svWr(o+9,bkSum>>8);
    for(int k=0;k<HH_NM;k++){ svWr(o+10+k,k<HH_NM-1?last[k]:0); if(!last[k]) break; }
    for(int k=0;k<HH_NM;k++){ svWr(o+22+k,k<HH_NM-1?first[k]:0); if(!first[k]) break; }
    svWr(o+34,stage); svWr(o+35,LK_N); svWr(o,'H'); svWr(o+1,'B');   // byte 35: how many looks the record holds (older records have another value there: LK_N10)
}
static int bkNl(int v){ return v==LK_N?LK_N:v==LK_N12?LK_N12:v==LK_N11?LK_N11:LK_N10; }   // looks a bank record holds, from its byte 35
static void bkSex(u8*lk,int nl){ if(nl<LK_N) lk[LK_SEX]=sexGuess(lk); else if(lk[LK_SEX]>=SX_N) lk[LK_SEX]=SX_NB; }   // (a record written before GENDER)
static int bkCheck(int i){ u32 o=bkOff(i); if(!bkOk(i)) return 0; int n=svRd(o+6)|svRd(o+7)<<8; if(n>BK_SZ-BK_HDR) return 0;
    u16 s=0; for(int k=0;k<n;k++) s=(u16)(s+svRd(o+BK_HDR+k)); return s==(u16)(svRd(o+8)|svRd(o+9)<<8); }
static int bkPutMine(int i,u16 key,int lot){   // the household you play (kind 1) into record i. 1 = done
    simsSaveNow(); hhSave(); ageSave(); persSave();
    volatile u8*hh=SRAM_BASE+HH_OFF; int hl=hhBlockLen(hh,SL_HH_LEN); if(!hl) return 0;
    if(BK_HDR+LK_N+5+PERS_LEN+SIM_BLOCK+8+2+hl>BK_SZ) return 0;
    svErr=0; svErase(bkOff(i),BK_SZ); bkW=bkOff(i)+BK_HDR; bkSum=0;
    for(int k=0;k<LK_N;k++) bkPut8(look[k]);
    for(int k=0;k<5;k++) bkPut8(SRAM_BASE[AGE_OFF+k]);
    for(int k=0;k<PERS_LEN;k++) bkPut8(SRAM_BASE[PERS_OFF+k]);
    for(int k=0;k<SIM_BLOCK;k++) bkPut8(SIM_SRAM[k]);
    for(int k=0;k<8;k++) bkPut8(SRAM_BASE[STORY_OFF+k]);   // (their story, story.h)
    bkPut8(hl); bkPut8(hl>>8); for(int k=0;k<hl;k++) bkPut8(hh[k]);
    bkHead(i,1,lot,key,hhPLast,hhPName,stage);
    return !svErr;
}
static int bkPutSim(int i,u16 key,int lot,const HhSim*s){   // one Sim who moved out (kind 2)
    svErr=0; svErase(bkOff(i),BK_SZ); bkW=bkOff(i)+BK_HDR; bkSum=0;
    for(int k=0;k<LK_N;k++) bkPut8(s->look[k]);
    bkPut8(s->asp); bkPut8(s->ltw); for(int k=0;k<TR_N;k++) bkPut8(s->tr[k]);
    bkHead(i,2,lot,key,s->last,s->name,s->stage);
    return !svErr;
}
static void hhRelClear(void){ for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ relD[a][b]=relL[a][b]=0; relF[a][b]=0; } }
static void hhAfterSwitch(void){   // a different household is the one you play: everything baked or picked for the old one goes
    twKeep=0; sprKey=0; for(int m=0;m<HH_MAX;m++) hhKey[m]=0; hhSlotsFree();
    custom=0; fixLook(); buildLook(); setColors(); ageSave(); persSave(); hhSave(); simsSaveNow();
}
static int bkTake(int i){   // record i becomes the household you play (it leaves the bank). 1 = done
    if(!bkCheck(i)) return 0;
    u32 p=bkOff(i)+BK_HDR; int kind=svRd(bkOff(i)+2);
    { int nl=bkNl(svRd(bkOff(i)+35)); for(int k=0;k<LK_N;k++) look[k]=k<nl?svRd(p++):0; bkSex(look,nl); }
    if(kind==1){
        for(int k=0;k<5;k++) SRAM_BASE[AGE_OFF+k]=svRd(p++);
        for(int k=0;k<PERS_LEN;k++) SRAM_BASE[PERS_OFF+k]=svRd(p++);
        for(int k=0;k<SIM_BLOCK;k++) SIM_SRAM[k]=svRd(p++);
        for(int k=0;k<8;k++) SRAM_BASE[STORY_OFF+k]=svRd(p++);
        int hl=svRd(p)|svRd(p+1)<<8; p+=2; if(hl>SL_HH_LEN) return 0;
        for(int k=0;k<hl;k++) SRAM_BASE[HH_OFF+k]=svRd(p++);
        ageLoad(); persLoad(); hhLoad();
    } else {   // one Sim: a new life, a household of one
        pAsp=svRd(p++); pLtw=svRd(p++); for(int k=0;k<TR_N;k++) pTr[k]=svRd(p++);
        stage=svRd(bkOff(i)+34); bkName(i,1,hhPName); bkName(i,0,hhPLast);
        simsNewLife(); hhN=0; hhRelClear(); stOff();
    }
    bkDel(i); hhAfterSwitch(); return 1;
}
static void stOff(void);   // story.h
static void hhFresh(int f){   // pre-made family f moves in for the first time: a new life, you are its first Sim, the rest live with you
    simsNewLife(); moodReset(); hhN=0; hhRelClear(); stOff();
    if(hhMoveIn(&hhFams[f])>0){ hhSwap(&hhM[0]); hhRemove(0); }
    hhAfterSwitch();
}

// ---------- who lives where ----------
static int nbFamOf(const Town*t,int li){   // the pre-made family on lot li, -1 = none (your home, community lots, the free lots)
    int base=nbKey(t)%HH_NFAM, c=0;
    for(int i=0;i<NB_LOTS;i++){ const NbLot*L=&t->lot[i]; if(!L->on||L->kind!=LKIND_RES) continue;
        if(i==li) return (i==t->home||c%3==2)?-1:(base+c-c/3)%HH_NFAM;
        c++; }
    return -1;
}
static void famLast(const HhFam*F,char*d){ const char*f=F->fam; if(f[0]=='T'&&f[1]=='H'&&f[2]=='E'&&f[3]==' ') f+=4; int k=0; while(f[k]&&k<HH_NM-1){ d[k]=f[k]; k++; } if(k>1&&d[k-1]=='S') k--; d[k]=0; }   // THE STACKS -> STACK
static int nbWho(int li,char*nm){   // who lives on lot li of the town you are in: 1 = a household from the bank, 2 = a pre-made family, 0 = nobody (nm = their family name)
    if(!nbOk||li<0||li>=NB_LOTS||li==nbT.home) return 0;
    int b=bkFind(nbKey(&nbT),li);
    if(b>=0){ if(nm){ char l[HH_NM]; bkName(b,0,l); char*e=slCat(nm,"THE "); e=slCat(e,l[0]?l:"NEIGHBOR"); if(l[0]) slCat(e,"S"); } return 1; }
    int f=nbFamOf(&nbT,li); if(f<0) return 0;
    { char l[HH_NM]; famLast(&hhFams[f],l); int k=0; while(l[k]&&l[k]==hhPLast[k]) k++; if(!l[k]&&!hhPLast[k]) return 0; }   // that family is the one you play: they live with you, not here
    if(nm) slCat(nm,hhFams[f].fam);
    return 2;
}
static int nbLives(int li){ return nbWho(li,0)!=0; }
static void nbSimFrom(int li,HhSim*s,char*from);
static int nbVisitor(HhSim*s,char*from,int skip){   // a Sim of another household of the town into s (look, stage, names, traits), the lot it lives on into from.
    int cand[NB_LOTS], n=0;                        // Returns who it is (skip that next time: two visitors are never from one household), -2 = nobody
    if(nbOk) for(int li=0;li<NB_LOTS;li++) if(nbLives(li)&&li!=skip) cand[n++]=li;
    from[0]=0;
    if(!n){   // no neighbors (no town yet, or nobody else lives in it): someone from a pre-made family, from down the street
        int f=rnd8()%HH_NFAM; const HhFam*F=&hhFams[f]; const HhPre*p=&F->m[rnd8()%F->n];
        hhNew(s,p); famLast(F,s->last); return 100+f; }
    int li=cand[rnd8()%n]; nbSimFrom(li,s,from); return li;
}
static void nbSimFrom(int li,HhSim*s,char*from){   // a Sim of the household on lot li (someone lives there) into s; the lot's name into from
    { int k=0; for(;nbT.lot[li].name[k]&&k<11;k++) from[k]=nbT.lot[li].name[k]; from[k]=0; }
    int b=bkFind(nbKey(&nbT),li);
    if(b>=0){ u32 o=bkOff(b), p=o+BK_HDR; { int nl=bkNl(svRd(o+35)); for(int k=0;k<LK_N;k++) s->look[k]=k<nl?svRd(p+k):0; bkSex(s->look,nl); }
        s->stage=svRd(o+34); if(s->stage>=AG_N) s->stage=AG_ADULT; s->asp=AS_FORTUNE; for(int k=0;k<TR_N;k++) s->tr[k]=5;
        bkName(b,1,s->name); bkName(b,0,s->last); return; }
    const HhFam*F=&hhFams[nbFamOf(&nbT,li)]; hhNew(s,&F->m[rnd8()%F->n]); famLast(F,s->last);
}

// ---------- playing another household / a new one ----------
static int hhLeaveHome(void){   // the household you play goes into the bank, on your home lot. 1 = done
    if(custom){ toast("HAND BUILT SIMS CANNOT MOVE"); return 0; }
    if(!bkN){ toast("THIS SAVE CHIP HAS NO ROOM FOR HOUSEHOLDS"); return 0; }
    u16 key=nbKey(&nbT); int me=bkFind(key,nbT.home); if(me<0) me=bkFree();
    if(me<0){ toast("THE HOUSEHOLD BANK IS FULL"); return 0; }
    ldShow("PACKING UP YOUR HOUSEHOLD",0,4);
    if(!bkPutMine(me,key,nbT.home)){ ldEnd(); toast("YOUR HOUSEHOLD COULD NOT BE SAVED"); return 0; }
    return 1;
}
static int hhPlayAt(int li){   // NEIGHBORHOOD lot menu > PLAY THE ...: you play the household of lot li (yours waits in the bank). 1 = the game was played
    char nm[24]; nm[0]=0; int who=nbWho(li,nm); if(!who) return 0;
    { char q[32]; char*e=slCat(q,"PLAY "); slCat(e,nm); const char*yn[2]={"YES","NO"}; if(menu(q,yn,2)!=0) return 0; }
    int b=who==1?bkFind(nbKey(&nbT),li):-1, f=who==2?nbFamOf(&nbT,li):-1;
    if(!hhLeaveHome()) return 0;
    int old=nbT.home; nbT.home=(u8)li;
    if(!nbGo(li)){ nbT.home=(u8)old; nbSave(); toast(nbErr); return 0; }
    ldShow("MOVING THEM IN",2,4);
    if(b>=0){ if(!bkTake(b)){ toast("THAT HOUSEHOLD IS DAMAGED"); hhFresh(f<0?0:f); } } else hhFresh(f);
    nbSave(); return 1;
}
static int hhNewAt(int li){   // NEIGHBORHOOD lot menu > NEW HOUSEHOLD HERE (a free lot): new Sims, made in the creator. 1 = done
    { const char*yn[2]={"YES","NO"}; if(menu("NEW SIMS ON THIS LOT",yn,2)!=0) return 0; }
    if(!hhLeaveHome()) return 0;
    int old=nbT.home; nbT.home=(u8)li;
    if(!nbGo(li)){ nbT.home=(u8)old; nbSave(); toast(nbErr); return 0; }
    simsNewLife(); moodReset(); hhN=0; hhRelClear(); hhPLast[0]=0; stOff(); hhAfterSwitch(); nbSave();
    return 1;
}
static int hhMoveOut(int m){   // member m moves out: to a free lot of the town if there is one (they live there and come to visit), else away
    if(m<0||m>=hhN) return 0;
    int lot=-1; if(nbOk) for(int li=0;li<NB_LOTS&&lot<0;li++){ const NbLot*L=&nbT.lot[li]; if(L->on&&L->kind==LKIND_RES&&li!=nbT.home&&!nbLives(li)) lot=li; }
    int r=lot>=0?bkFree():-1, there=0;
    if(r>=0&&bkPutSim(r,nbKey(&nbT),lot,&hhM[m])) there=1;
    hhRemove(m); for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; } hhSave();
    if(there){ static char t[40]; char*e=slCat(t,"THEY LIVE ON "); slCat(e,nbT.lot[lot].name); toast(t); nbSave(); } else toast("MOVED OUT OF TOWN");
    return 1;
}

// ---------- the phone ----------
static u8 phFood; static short phT;   // food on its way: 1 pizza, 2 Chinese; steps until the doorbell
static void phTick(void){   // once per logic step in the life game
    if(!phFood||--phT>0) return;
    int f=phFood; phFood=0; lfood=100; for(int m=0;m<hhN;m++) hhM[m].need[HN_FOOD]=100;   // everyone eats
    if(f==1){ toast("DING DONG  THE PIZZA IS HERE"); toast("WAIT... IT'S NOT DELIVERY"); toast("IT'S DIGIORNO"); }
    else toast("THE CHINESE FOOD IS HERE");
    lnote="EVERYONE ATE"; lnoteT=60; liveInvalidate();
}
static void phInvite(void){   // someone from another household comes over (they take a free member place, like a visitor)
    int k=0, v=TW_V(0); if(v<hhN){ toast("NO ROOM FOR GUESTS"); return; }
    const char* it[NB_LOTS+1]; static char nm[NB_LOTS][28] EWRAM_BSS; int lot[NB_LOTS], n=0;
    if(nbOk) for(int li=0;li<NB_LOTS;li++){ char f[24]; f[0]=0; if(!nbWho(li,f)) continue; char*e=slCat(nm[n],f); e=slCat(e,"  "); slCat(e,nbT.lot[li].name); it[n]=nm[n]; lot[n++]=li; }
    if(!n){ toast("NOBODY ELSE LIVES IN TOWN YET"); return; }
    int c=menu("WHO DO YOU CALL",it,n); if(c<0) return;
    HhSim*s=&hhM[v]; nbSimFrom(lot[c],s,twFrom[k]);
    s->uid=255; s->bubT=0; s->hp=HP_MAX; s->act=HA_IDLE; s->pn=s->pi=0;
    twHas[k]=1;   // (a guest staying over counts for STORY MODE's HAVE A NEIGHBOR OVER: story.h watches twOn) twOn[k]=0; twWait[k]=90; hhKey[v]=0; twKeep=1;
    toast("PLEASE WAIT  THEY ARE ON THEIR WAY"); hhBakeAll();
    static char t[40]; char*e=simCat(t,s->name); simCat(e," IS COMING OVER"); toast(t);
}
static void phoneMenu(void){   // pause menu > PHONE
    const char* it[5]; int id[5], n=0;
    it[n]="INVITE SOMEONE OVER"; id[n++]=0;
    it[n]="ORDER PIZZA  \xC2\xA7" "20"; id[n++]=1;
    it[n]="ORDER CHINESE  \xC2\xA7" "15"; id[n++]=2;
    if(dbgOn&&hhN){ it[n]="MOVE SOMEONE OUT"; id[n++]=3; }   // (the DEBUG CODE: every change to who lives in the house)
    int c=menu("PHONE",it,n); if(c<0) return;
    switch(id[c]){
        case 0: phInvite(); break;
        case 1: case 2: { int cost=id[c]==1?20:15;
            if(phFood){ toast("FOOD IS ALREADY ON ITS WAY"); break; }
            if(simMoney<cost){ toast("NOT ENOUGH SIMOLEONS"); break; }
            simMoney-=cost; simsSave(); phFood=(u8)id[c]; phT=(short)(480+(rnd8()<<1));   // the doorbell in 8 to 16 seconds
            toast(id[c]==1?"A PIZZA IS ON ITS WAY":"CHINESE FOOD IS ON ITS WAY"); break; }
        case 3: { const char* who[HH_MAX]; for(int m=0;m<hhN;m++) who[m]=hhM[m].name;
            int m=menu("WHO MOVES OUT?",who,hhN); if(m<0) break;
            const char*yn[2]={"YES  GOODBYE","NO"}; if(menu("ARE YOU SURE?",yn,2)!=0) break;
            hhMoveOut(m); break; }
    }
}
