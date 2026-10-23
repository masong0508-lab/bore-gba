#define ST_SMOKE 1
// stash.h - THE STASH: seeds and grams one by one (MY STASH in YOUR ACTIONS > FUN; SELL SOME at the dispensary counter). Needs weed.h.
//   Seeds: PLANT (a pot in reach), PLANT EVERY EMPTY POT (one seed each, as many pots as you have seeds, pots within 6 tiles), SELL ONE / ALL (at the counter).
//   Grams: ROLL ONE (a gram of that strain), SELL ONE / ALL (at the counter, the counter's price).  No new saved data: it is the stash weed.h already saves.
static int stCount(void){ int n=0; for(int i=0;i<WD_N;i++) n+=wdSeed[i]+wdG[i]; return n; }
static int stPots(void){ int n=0, tx=(int)(lfx>>8), ty=(int)(lfy>>8);   // empty planters within 6 tiles
    for(int y=ty-6;y<=ty+6;y++) for(int x=tx-6;x<=tx+6;x++) if(x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]=='u'&&wpSt(x,y)==WP_EMPTY) n++;
    return n; }
static int stPlantAll(int s){   // one seed of strain s into every empty pot in reach, while the seeds last
    int n=0, tx=(int)(lfx>>8), ty=(int)(lfy>>8);
    for(int y=ty-6;y<=ty+6;y++) for(int x=tx-6;x<=tx+6;x++){
        if(!wdSeed[s]) break;
        if(x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]=='u'&&wpSt(x,y)==WP_EMPTY){ wdSeed[s]--; wpSet(x,y,WP_SPROUT,3,s); n++; } }
    if(n){ wdSk=(u8)(wdSk+n>100?100:wdSk+n); wdSave(); liveInvalidate(); lstun=50; lsp=0; lgrind=0;
        char*e=simCatN(wdMsg,n); simCat(e,n==1?" SEED PLANTED":" SEEDS PLANTED"); lnote=wdMsg; lnoteT=120; }
    else toast("NO EMPTY POT IN REACH");
    return n; }
static int stBuySeeds(int s,int each){   // 1, 5 or 10 packs of strain s at 'each'; 1 = bought something
    static const char* const qn[3]={"1 PACK","5 PACKS","10 PACKS"}; static const u8 qv[3]={1,5,10};
    int q=menu("HOW MANY",qn,3); if(q<0) return 0;
    int k=qv[q]; if(wdSeed[s]+k>99) k=99-wdSeed[s];
    if(k<=0){ toast("YOU CARRY ENOUGH"); return 0; }
    if(simMoney<each*k){ toast("NOT ENOUGH SIMOLEONS"); return 0; }
    simMoney-=each*k; wdSeed[s]=(u8)(wdSeed[s]+k); wdSave(); sfxPlay(SFX_CASH); return 1; }
static void stInv(int sell){   // sell = 1 at the dispensary counter
    if(!wdOk()){ lnote=stage==AG_TEEN?"NOT OLD ENOUGH YET":"GROWN-UPS ONLY"; lnoteT=60; return; }
    char lb[2*WD_N][30]; const char* it[2*WD_N]; int id[2*WD_N];
    for(;;){
        int n=0;
        for(int i=0;i<WD_N;i++) if(wdSeed[i]){ char*e=simCat(lb[n],"SEEDS  "); e=simCat(e,wdS[i].nm); e=simCat(e,"  X"); simCatN(e,wdSeed[i]); it[n]=lb[n]; id[n++]=i; }
        for(int i=0;i<WD_N;i++) if(wdG[i]){ char*e=simCat(lb[n],wdS[i].nm); e=simCat(e,"  "); e=simCatN(e,wdG[i]); simCat(e," G"); it[n]=lb[n]; id[n++]=WD_N+i; }
        if(!n){ toast("NOTHING IN YOUR STASH"); break; }
        int c=menu(sell?"SELL WHICH":"MY STASH",it,n); if(c<0) break;
        int k=id[c], g=k>=WD_N, s=g?k-WD_N:k;
        const char* ai[3]; int ad[3], an=0; char l1[26], l2[26];
        int each=g?wdS[s].price*3/5:20, have=g?(int)wdG[s]:(int)wdSeed[s];   // the counter's price for a gram; a pack of seeds resells for 20
        if(sell){ char*e=simCat(l1,"SELL ONE  \xC2\xA7"); simCatN(e,each); ai[an]=l1; ad[an++]=0;
                  if(have>1){ e=simCat(l2,"SELL ALL  \xC2\xA7"); simCatN(e,each*have); ai[an]=l2; ad[an++]=1; } }
        else if(g){
#ifdef ST_SMOKE
            ai[an]="ROLL ONE UP"; ad[an++]=2;
#endif
        } else { if(wdNear('u',0,0)){ ai[an]="PLANT IT"; ad[an++]=3; } if(stPots()>1){ ai[an]="PLANT EVERY EMPTY POT"; ad[an++]=4; } }
        if(!an){ toast(g?"NOTHING TO DO WITH IT HERE":"GET NEAR A PLANTER  OR THE COUNTER"); continue; }
        int a=menu(g?wdS[s].nm:"SEED PACKS",ai,an); if(a<0) continue; a=ad[a];
        if(a==0||a==1){ int q=a?have:1; if(g) wdG[s]=(u16)(wdG[s]-q); else wdSeed[s]=(u8)(wdSeed[s]-q);
            simMoneyAdd(each*q); if(g) wdSold=(u16)(wdSold+q); sfxPlay(SFX_CASH); wdSave(); simsSave(); if(g) stQEvent(QE_SOLD,q);
            char*e=simCat(wdMsg,"SOLD "); e=simCatN(e,q); e=simCat(e,g?"G  \xC2\xA7":" PACKS  \xC2\xA7"); simCatN(e,each*q); toast(wdMsg); continue; }
#ifdef ST_SMOKE
        if(a==2){ wdG[s]--; wdSmokeUp(s,1,1); wdSave(); break; }
#endif
        if(a==3){ int x,y; if(wdNear('u',&x,&y)&&wpSt(x,y)==WP_EMPTY){ wdSeed[s]--; wpSet(x,y,WP_SPROUT,3,s); wdSk=(u8)(wdSk<100?wdSk+1:100); lnote="PLANTED  WATER IT NOW AND THEN"; lnoteT=120; lstun=50; lsp=0; lgrind=0; wdSave(); liveInvalidate(); break; }
            toast("THAT POT IS NOT EMPTY"); continue; }
        if(a==4){ stPlantAll(s); break; }
    }
    liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
}
