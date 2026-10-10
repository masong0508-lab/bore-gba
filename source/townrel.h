// townrel.h - TOWN RELATIONSHIPS: the Sims of the town's other households are people you know, not passers-by.
//   GUESTS          a neighbour who drops by (or that you call over) is GUEST k while on the lot: uid GU0+k, rows of its own in relD / relL /
//                   relF / kin (house.h), so they get the whole social pie (TALK ... KISS, GO STEADY, PROPOSE, PUNCH), balloons, NEW FRIEND
//                   notes, wants and fears, exactly like the household. Household Sims go and talk to them too (hhSeek), and a guest who is
//                   staying now and then comes to you with something to say (nrGuestChat).
//   REMEMBERED      what a guest and your Sims feel about each other goes into the town table (nrT, house.h) when they leave, when you save,
//                   or when someone else takes their place (nrSync), and comes back the next time they visit (nrGuestIn). One entry per
//                   pair (who in the town, whose relationship), both ways. It is saved in the household block ('HA'), so it is per household.
//   VISITS          who drops by is weighted by how much their household means to yours (nrLotPull) and which of them comes by how much
//                   each means to you (nrMemPick). Friends come back sooner (house.h twTick).
//   THE PHONE       INVITE SOMEONE OVER lists the people you know first, by name (nrKnown), then every household of the town.
//   MOVING IN       ASK TO MOVE IN (adults asking someone teen or older they get on with; there must be room): a yes and they live here.
//                   PROPOSE to a neighbour: a yes moves them in and marries you (nrWedIn). A Sim of a pre-made family is then gone from
//                   that family (a marker entry, NR_MARK); one who lived alone on a lot (bank kind 2) leaves the lot. A member who MOVES OUT
//                   (households.h) takes what you feel about each other with them: they are someone you know in the town from then on.
//   DRIFT           every midnight the daily score of someone you did not see drifts a third of the way back to the lifetime one (nrDay).
//   SHOWN           RELATIONSHIPS (pause > HOUSEHOLD) and MY SIM > PEOPLE list the household, then the town Sims you know (blue names).
//                   Career promotions count town friends too (career.h jobFriends).
// Needs before it: house.h, households.h (nbT, bkFind, nbFamOf, hhFams, bkName), the UI kit.

// (NR_MARK, the marker entries' own, is in house.h next to NR_LOST)
static u8 nrHash(const char*s){ u8 h=0x5D; for(int i=0;s[i]&&i<HH_NM-1;i++) h=(u8)(h*31+(u8)s[i]); return h; }   // (as far as a name is kept)
static void nrCheckTown(void){ if(!nbOk) return; u16 k=nbKey(&nbT); if(k!=nrKey){ if(nrKey) nrN=0; nrKey=k; } }   // entries of another town are no use here (no town loaded yet: nothing to tell; entries made before there was a town are kept)
static int nrFind(int lot,int mem,int nh,int own){ for(int e=0;e<nrN;e++){ const NrE*q=&nrT[e]; if(q->lot==lot&&q->mem==mem&&q->nh==nh&&q->own==own) return e; } return -1; }
static int nrLost(int lot,int mem,int nh){ return nrFind(lot,mem,nh,NR_MARK)>=0; }
static int nrAbs(int v){ return v<0?-v:v; }
static int nrNew(void){   // a free entry (the table full: the one that means least goes)
    if(nrN<NR_N) return nrN++;
    int w=-1, bw=1<<30; for(int e=0;e<nrN;e++){ const NrE*q=&nrT[e]; if(q->own==NR_MARK) continue;
        int v=nrAbs(q->d1)+nrAbs(q->d2)+2*(nrAbs(q->l1)+nrAbs(q->l2))+((q->f1|q->f2)?300:0); if(v<bw){ bw=v; w=e; } }
    return w<0?0:w;
}
static void nrDel(int e){ for(int i=e;i<nrN-1;i++) nrT[i]=nrT[i+1]; nrN--; }
static void nrForget(int u){ for(int e=nrN-1;e>=0;e--) if(nrT[e].own==u) nrDel(e); }   // (house.h hhRemove: u left the household)
static void nrMark(int lot,int mem,int nh){ if(nrLost(lot,mem,nh)) return; int e=nrNew(); NrE*q=&nrT[e]; q->lot=(u8)lot; q->mem=(u8)mem; q->nh=(u8)nh; q->own=NR_MARK; q->d1=q->l1=q->d2=q->l2=0; q->f1=0; q->f2=NR_LOST; }

// ---- guests in and out ----
static void nrGuestIn(int k){   // visitor k is set up (house.h twWho): their rows from the table, as this household knows them
    int g=GU0+k; for(int u=0;u<RU_N;u++){ relD[g][u]=relD[u][g]=0; relL[g][u]=relL[u][g]=0; relF[g][u]=relF[u][g]=0; kin[g][u]=kin[u][g]=0; }
    if(twLot[k]==255) return;
    nrCheckTown();
    for(int i=-1;i<hhN;i++){ int u=i<0?hhPUid:hhM[i].uid, e=nrFind(twLot[k],twMem[k],twNh[k],u); if(e<0) continue; const NrE*q=&nrT[e];
        relD[u][g]=q->d1; relL[u][g]=q->l1; relF[u][g]=q->f1; relD[g][u]=q->d2; relL[g][u]=q->l2; relF[g][u]=(u8)(q->f2&~NR_LOST); }
}
static int nrGuestOk(int k){ int v=TW_V(k); return twHas[k]&&v>=hhN&&hhM[v].uid==GU0+k&&twLot[k]!=255; }   // place k still holds that guest
static void nrSync(void){   // the guests' rows into the table (house.h hhSave, twPick, a visitor leaving)
    for(int k=0;k<TW_N;k++){ if(!nrGuestOk(k)) continue; int g=GU0+k; nrCheckTown();
        for(int i=-1;i<hhN;i++){ int u=i<0?hhPUid:hhM[i].uid, any=relD[u][g]|relL[u][g]|relF[u][g]|relD[g][u]|relL[g][u]|relF[g][u];
            int e=nrFind(twLot[k],twMem[k],twNh[k],u);
            if(e<0){ if(!any) continue; e=nrNew(); NrE*q=&nrT[e]; q->lot=twLot[k]; q->mem=twMem[k]; q->nh=twNh[k]; q->own=(u8)u; q->f2=0; }
            NrE*q=&nrT[e]; q->d1=relD[u][g]; q->l1=relL[u][g]; q->f1=relF[u][g]; q->d2=relD[g][u]; q->l2=relL[g][u]; q->f2=(u8)((q->f2&NR_LOST)|relF[g][u]); } }
}
static void nrDay(void){   // midnight (sims.h): the daily score of the people you know drifts back towards the lifetime one
    for(int e=0;e<nrN;e++){ NrE*q=&nrT[e]; if(q->own==NR_MARK) continue; q->d1=(signed char)(q->d1+(q->l1-q->d1)/3); q->d2=(signed char)(q->d2+(q->l2-q->d2)/3); }
}

// ---- who comes by ----
static int nrPullOf(const NrE*q){ int p=(q->l1+q->l2+q->d1/2)/10; if(p<0) p=nrAbs(q->l1)>=50?3:0; if(q->f1&(RF_STEADY|RF_LOVE)) p+=20; else if(q->f1&RF_CRUSH) p+=10; if(q->f1&RF_BFF) p+=10; return p; }   // (enemies drop by now and then too)
static int nrMine(int u){ return u==hhPUid||hhMemOf(u)>=0; }   // a uid of this household (not a guest)
static int nrLotPull(int lot){ int p=0; for(int e=0;e<nrN;e++){ const NrE*q=&nrT[e]; if(q->lot==lot&&q->own<GU0&&nrMine(q->own)) p+=nrPullOf(q); } return p>60?60:p; }
static int nrMemPick(int lot,const HhFam*F){   // which member of pre-made family F (on lot) comes: the ones who mean most to you more often; -1 = they all moved in with you
    int w[4], sum=0; u8 nh[4];
    for(int i=0;i<F->n&&i<4;i++){ nh[i]=nrHash(F->m[i].name); w[i]=0; if(nrLost(lot,i,nh[i])) continue;
        w[i]=6; for(int e=0;e<nrN;e++){ const NrE*q=&nrT[e]; if(q->lot==lot&&q->mem==i&&q->nh==nh[i]&&q->own<GU0&&nrMine(q->own)) w[i]+=nrPullOf(q); }
        sum+=w[i]; }
    if(!sum) return -1;
    int x=(rnd8()*sum)>>8; for(int i=0;i<F->n&&i<4;i++){ if(x<w[i]) return i; x-=w[i]; }
    for(int i=F->n-1;i>=0;i--) if(w[i]) return i; return -1;
}
static int nrPull(int k){ int g=GU0+k; return nrGuestOk(k)&&(relD[g][hhPUid]>=30||(relF[hhPUid][g]&(RF_STEADY|RF_LOVE|RF_BFF))); }   // visitor k likes you: back sooner
static void nrGuestChat(int k){   // a guest who is staying comes out with something now and then (house.h twTick)
    int g=GU0+k, me=hhPUid; if(!guestHere(g)||(rnd8()&1)) return;
    if(lstun>0||simAct||lz>(s32)(surfH(lfx,lfy)<<8)) return;   // you are busy, or in the air
    HhSim*s=&hhM[TW_V(k)]; int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8), px=(int)(lfx>>8), py=(int)(lfy>>8);
    if((sx-px)*(sx-px)+(sy-py)*(sy-py)>9) return;   // within 3 tiles
    s->hd=(u8)(px>sx?0:px<sx?8:py>sy?4:12);
    int i=socPick(g,me); if(!socAllowed(g,me,i)) i=SC_TALK;
    socDo(g,me,i);
}

// ---- who someone of the town is (for the lists) ----
static int nrWho(const NrE*q,char*nm,int*sx){   // their first name and gender; 0 = not there any more (the lot changed hands)
    const HhPre*p=0; nm[0]=0; *sx=SX_NB;
    if(q->lot&0x80){ int f=q->lot&0x7F; if(f<HH_NFAM&&q->mem<hhFams[f].n) p=&hhFams[f].m[q->mem]; }
    else if(nbOk&&q->lot<NB_LOTS&&nbLives(q->lot)){ int b=bkFind(nbKey(&nbT),q->lot);
        if(b>=0){ if(q->mem) return 0; bkName(b,1,nm); u32 o=bkOff(b); if(svRd(o+35)>=LK_N15){ int x=svRd(o+BK_HDR+LK_SEX); *sx=x<SX_N?x:SX_NB; } }
        else { int f=nbFamOf(&nbT,q->lot); if(f>=0&&q->mem<hhFams[f].n) p=&hhFams[f].m[q->mem]; } }
    if(p){ int i=0; for(;p->name[i]&&i<HH_NM-1;i++) nm[i]=p->name[i]; nm[i]=0; *sx=p->sex; }
    return nm[0]&&nrHash(nm)==q->nh;
}
static const char* nrWord(const NrE*q,int sx){   // like relWord (house.h), from an entry
    int d=q->d1, l=q->l1; u8 f=q->f1;
    if(f&RF_STEADY) return sexWord(SW_DATE,sx);
    if(f&RF_LOVE) return "IN LOVE";
    if(f&RF_CRUSH) return "CRUSH";
    if(d>=70&&l>=70) return "BEST FRIEND";
    if(d>=50) return "FRIEND";
    if(d<=-50) return "ENEMY";
    if(d<=-20) return "DISLIKE";
    return "ACQUAINTANCE";
}
static u8 nrL[NR_N] EWRAM_BSS; static int nrLn;   // the entries of the Sim you control, closest first (nrList)
static void nrList(void){
    nrSync(); nrLn=0;
    for(int e=0;e<nrN;e++){ const NrE*q=&nrT[e]; char nm[HH_NM]; int sx; if(q->own!=hhPUid||!nrWho(q,nm,&sx)) continue;
        int v=q->d1+2*q->l1+((q->f1&(RF_STEADY|RF_LOVE))?400:0), j=nrLn++;
        while(j>0){ const NrE*r=&nrT[nrL[j-1]]; if(r->d1+2*r->l1+((r->f1&(RF_STEADY|RF_LOVE))?400:0)>=v) break; nrL[j]=nrL[j-1]; j--; }
        nrL[j]=(u8)e; }
}
static int nrFriends(int u){ nrSync(); int n=0; for(int e=0;e<nrN;e++){ const NrE*q=&nrT[e]; char nm[HH_NM]; int sx; if(q->own==u&&(q->f1&RF_FRIEND)&&nrWho(q,nm,&sx)) n++; } return n; }   // (career.h)
static int nrKnown(int*lot,int*mem,char (*nm)[28],int max){   // the phone: the people you know (not over right now), "NAME  FRIEND"
    nrList(); int n=0;
    for(int i=0;i<nrLn&&n<max;i++){ const NrE*q=&nrT[nrL[i]]; if(q->lot&0x80) continue;   // (someone from down the street has no lot to call)
        int here=0; for(int k=0;k<TW_N;k++) if(nrGuestOk(k)&&twLot[k]==q->lot&&twMem[k]==q->mem) here=1; if(here) continue;
        char f[HH_NM]; int sx; nrWho(q,f,&sx); char*e=simCat(nm[n],f); e=simCat(e,"  "); simCat(e,nrWord(q,sx)); lot[n]=q->lot; mem[n++]=q->mem; }
    return n;
}

// ---- the people lists: RELATIONSHIPS (house.h relScreen) and MY SIM > PEOPLE (mysim.h). The household first, then the town ----
static int pplCount(void){ nrList(); return hhN+nrLn; }   // (call once per screen: it orders the town rows)
static void pplRow(int r,int y,int b1,int b2,int hx){   // row r at y: name, what they are, the four bars (you to them, them to you) at b1 / b2, a heart at hx
    int a=hhPUid; char q[40];
    if(r<hhN){ int b=hhM[r].uid; const char*w=relWord(a,b); int kr=kin[b][a];   // what they are to you (CREATE-A-HOUSEHOLD), then how you feel
        text(10,y,hhM[r].name,WHITE,1);
        if(kr){ char*e=simCat(q,kinNm[kr]); e=simCat(e,"  "); simCat(e,w); if(tw(q,1)<=b1-14) w=q; else w=kinNm[kr]; }
        text(10,y+8,w,(relF[a][b]&(RF_LOVE|RF_STEADY|RF_CRUSH))?RGB(31,14,20):relD[a][b]<=-20?RGB(30,10,8):RGB(16,26,16),1);
        relBar(b1,y+1,relD[a][b]); relBar(b1,y+8,relL[a][b]); relBar(b2,y+1,relD[b][a]); relBar(b2,y+8,relL[b][a]);
        if(relF[a][b]&RF_STEADY) simIcon(hx,y+2,IC_HEART,RGB(31,14,20));
        return; }
    const NrE*e=&nrT[nrL[r-hhN]]; char nm[HH_NM]; int sx; nrWho(e,nm,&sx);
    text(10,y,nm,RGB(15,26,31),1);   // (blue: they live in the town)
    text(10,y+8,nrWord(e,sx),(e->f1&(RF_LOVE|RF_STEADY|RF_CRUSH))?RGB(31,14,20):e->d1<=-20?RGB(30,10,8):RGB(16,26,16),1);
    relBar(b1,y+1,e->d1); relBar(b1,y+8,e->l1); relBar(b2,y+1,e->d2); relBar(b2,y+8,e->l2);
    if(e->f1&RF_STEADY) simIcon(hx,y+2,IC_HEART,RGB(31,14,20));
}

// ---- moving in ----
static int nrMoveIn(int k){   // guest k comes to live here: their new uid, -1 = they cannot
    int v=TW_V(k), g=GU0+k; if(!nrGuestOk(k)||hhN>=HH_MAX) return -1;
    if(twLot[k]<NB_LOTS&&nbOk){ int b=bkFind(nbKey(&nbT),twLot[k]); if(b>=0){ if(svRd(bkOff(b)+2)!=2) return -1; bkDel(b); nbSave(); } else nrMark(twLot[k],twMem[k],twNh[k]); }   // (a household of its own stays where it is)
    else nrMark(twLot[k],twMem[k],twNh[k]);
    int t=hhN;
    for(int j=0;j<TW_N;j++) if(j!=k&&TW_V(j)==t&&twHas[j]){ nrSync(); twHas[j]=0; twOn[j]=0; }   // the other visitor was in the place they get: off home
    if(v!=t){ HhSim ts=hhM[t]; hhM[t]=hhM[v]; hhM[v]=ts; HhSpr sp=hhSp[t]; hhSp[t]=hhSp[v]; hhSp[v]=sp; u32 kk=hhKey[t]; hhKey[t]=hhKey[v]; hhKey[v]=kk;   // (their sprites come with them: only descriptors move)
        for(int i=0;i<16;i++){ u16 c=hhPal[t][i]; hhPal[t][i]=hhPal[v][i]; hhPal[v][i]=c; } }
    int u=hhFreeUid(); HhSim*s=&hhM[t]; hhN++;
    s->uid=(u8)u; s->act=HA_IDLE; s->think=60; s->pn=s->pi=0; s->bubT=0; s->ltw=0; if(s->stage<AG_ADULT&&s->asp<AS_PICK) s->asp=AS_GROW;
    hhFl[t]=(u8)curFl; hhUp[t]=0;
    for(int x=0;x<RU_N;x++){ relD[u][x]=relD[g][x]; relD[x][u]=relD[x][g]; relL[u][x]=relL[g][x]; relL[x][u]=relL[x][g]; relF[u][x]=relF[g][x]; relF[x][u]=relF[x][g]; kin[u][x]=kin[x][u]=0;
        relD[g][x]=relD[x][g]=0; relL[g][x]=relL[x][g]=0; relF[g][x]=relF[x][g]=0; }
    relD[u][u]=relL[u][u]=0; relF[u][u]=0; famAge[u]=0; twHas[k]=0; twOn[k]=0;
    for(int e=nrN-1;e>=0;e--){ const NrE*q=&nrT[e]; if(q->lot==twLot[k]&&q->mem==twMem[k]&&q->nh==twNh[k]&&q->own!=NR_MARK) nrDel(e); }   // they live here now: not in the town table
    for(int m=0;m<hhN;m++){ hhOld[m].x0=hhOld[m].x1=0; hhOldSig[m]=0xFFFFFFFFu; }
    hhSlotsFree(); hhSave(); return u;
}
static int nrCanAsk(int a,int g){ return uGuest(g)&&hhN<HH_MAX&&uStage(a)>=AG_ADULT&&uStage(g)>=AG_TEEN&&relD[a][g]>=30; }
static void nrAskIn(int a,int g){   // ASK TO MOVE IN: how they feel about you decides
    int c=25+relD[g][a]/2+relL[g][a]/2+((relF[g][a]&(RF_STEADY|RF_LOVE))?30:0)+(uTr(g,TR_NICE)-5)*2; c=c<5?5:c>95?95:c;
    char*t=simMsg2; t=simCat(t,uName(g));
    hhFreeze(a,60); hhFreeze(g,60);
    if((rnd8()*100>>8)>=c){ relD[g][a]=(signed char)clampR(relD[g][a]-5); hhSay(a,IC_HEART,"MOVE IN?"); hhSay(g,IC_BAIL,"NO"); voxPlay(V_nah); simCat(t," SAID NO  NOT YET"); lnote=simMsg2; lnoteT=110; return; }
    hhSay(a,IC_HEART,"MOVE IN?"); voxPlay(V_yahoo);
    int u=nrMoveIn(g-GU0); if(u<0){ toast("THEY HAVE A HOUSEHOLD OF THEIR OWN"); return; }
    relD[u][a]=(signed char)clampR(relD[u][a]+10); relL[u][a]=(signed char)clampR(relL[u][a]+5);
    simCat(t," MOVED IN"); simQueue(simMsg2); simEvent(SE_FRIEND); moodEvent(M_WANT);
}
static int nrWedIn(int g){ int u=nrMoveIn(g-GU0); if(u<0) toast("THEY HAVE A HOUSEHOLD OF THEIR OWN"); else { static char t[32] EWRAM_BSS; simCat(simCat(t,uName(u))," MOVES IN WITH YOU"); simQueue(t); } return u; }   // PROPOSE to a neighbour (house.h socDo)
static void nrBye(int g){   // SAY GOODBYE: they head home
    int k=g-GU0; hhSay(g,IC_TALK,"BYE"); hhSay(hhPUid,IC_TALK,"SEE YA");
    if(twOn[k]==2) twWait[k]=1; else if(twOn[k]==1){ HhSim*s=&hhM[TW_V(k)]; s->pn=s->pi=0; }   // (walking in: they turn round where they are)
    char*t=simCat(simMsg2,uName(g)); simCat(t," HEADS HOME"); lnote=simMsg2; lnoteT=80;
}
static void nrMovedOut(int m,int lot){   // households.h hhMoveOut: member m now lives alone on lot: what you feel about each other stays, as town relationships
    if(m<0||m>=hhN||lot<0) return; nrCheckTown(); int b=hhM[m].uid, nh=nrHash(hhM[m].name);
    for(int i=-1;i<hhN;i++){ int u=i<0?hhPUid:hhM[i].uid; if(u==b||!(relD[u][b]|relL[u][b]|relF[u][b]|relD[b][u]|relL[b][u])) continue;
        int e=nrFind(lot,0,nh,u); if(e<0){ e=nrNew(); } NrE*q=&nrT[e]; q->lot=(u8)lot; q->mem=0; q->nh=(u8)nh; q->own=(u8)u;
        q->d1=relD[u][b]; q->l1=relL[u][b]; q->f1=(u8)(relF[u][b]&~RF_STEADY); q->d2=relD[b][u]; q->l2=relL[b][u]; q->f2=(u8)(relF[b][u]&~RF_STEADY); }   // (moving out ends going steady)
}
