// babycare.h - BABY CARE. A baby cannot look after itself any more.
//
//  NEEDS    its FOOD (a bottle), WC (a nappy), CLEAN, REST, FUN and SOCIAL run down like anyone's, a little faster for FOOD and WC, but a baby never goes to the
//           fridge or the loo: it CRIES (a balloon, a note, a wail when you are near) until somebody feeds it, changes it, cuddles it, plays with it or puts it to bed.
//  CARERS   an awake household Sim with free will goes to a crying baby (parents first, kind Sims more, children only to cuddle and play) and does the job. Your own
//           Sim, left alone (YOUR SIM ALONE), walks over to one too. And you: R next to a baby opens FEED BOTTLE / CHANGE DIAPER / CUDDLE / PLAY / PUT TO BED.
//  THINGS   the FAMILY tab of the catalogue (catalog.h) helps: a HIGH CHAIR makes a bottle go further, a CHANGING TABLE gives a spotless nappy, a NURSING CHAIR
//           and a PLAYPEN, BABY MOBILE or TOY BOX make a cuddle or a game worth more, and a baby sleeps in a BASSINET or a CRIB (a TODDLER BED too). Grown-ups
//           leave the nursery beds alone as long as a bed of their own exists (hhBedKind, house.h).
//  AGES     a baby is a NEWBORN for the first half of its stage and a TODDLER for the second (it crawls about and plays; a notice says when). At midnight a baby
//           that went hungry or wet all day does not grow that day, and one that was looked after really well grows a day faster (famDay, family.h).
//           A BIRTHDAY CAKE in the house makes every birthday a party: everyone's FUN and SOCIAL go up.
// Include BEFORE family.h and AFTER house.h / catalog.h.
enum { BCR_FEED, BCR_DIAPER, BCR_CUDDLE, BCR_PLAY, BCR_BED, BCR_N };
enum { BI_BASSINET, BI_CRIB, BI_TODDLER, BI_HIGH, BI_TABLE, BI_PEN, BI_NURSE, BI_MOBILE, BI_TOYS, BI_CAKE, BI_N };
static const char* const biNm[BI_N]={"BASSINET","CRIB","TODDLER BED","HIGH CHAIR","CHANGING TABLE","PLAYPEN","NURSING CHAIR","BABY MOBILE","TOY BOX","BIRTHDAY CAKE"};
static u16 biId[BI_N]; static u8 biInit;
static int bcId(int k){   // the catalogue id of a nursery thing (found once, by name)
    if(!biInit){ biInit=1;
        for(int j=0;j<BI_N;j++) for(int i=0;i<CAT_N;i++){ const char*a=catTab[i].nm,*b=biNm[j]; while(*a&&*a==*b){ a++; b++; } if(!*a&&!*b){ biId[j]=(u16)(i+1); break; } } }
    return biId[k];
}
static int bcNear(int x,int y,int r,int k){   // is one of these within r tiles?
    int id=bcId(k); if(!id) return 0;
    for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){ int tx=x+dx, ty=y+dy; if(tx>=0&&ty>=0&&tx<MW&&ty<MH&&lifeMap[ty][tx]!='.'&&catIdAt(tx,ty)==id) return 1; }
    return 0;
}
static int bcCribAt(int x,int y){ if(x<0||y<0||x>=MW||y>=MH) return 0; int id=catIdAt(x,y); return id&&(id==bcId(BI_BASSINET)||id==bcId(BI_CRIB)||id==bcId(BI_TODDLER)); }   // a nursery bed
static int bcBedKind(const HhSim*s){   // which beds this Sim may plan to sleep in: 1 nursery beds only (a baby, when it has one), 2 everyone else's (when there is one), 0 any
    int cr=0, ot=0;
    for(int y=0;y<MH;y++) for(int x=0;x<MW;x++) if(lifeMap[y][x]=='S'){ if(bcCribAt(x,y)) cr++; else ot++; }
    if(s->stage==AG_BABY) return cr?1:0;
    return ot?2:0;
}
// ---- what a baby wants: the lowest need that is below its line ----
static int bcWant(const HhSim*s,int*low){
    int f=s->need[HN_FOOD], w=s->need[HN_WC], c=s->need[HN_CLEAN], r=s->need[HN_REST], fu=s->need[HN_FUN], so=s->need[HN_SOC], d=w<c?w:c, k=-1, v=100;
    if(f<50&&f<v){ k=BCR_FEED; v=f; }
    if(d<45&&d<v){ k=BCR_DIAPER; v=d; }
    if(r<35&&r<v){ k=BCR_BED; v=r; }
    if(so<40&&so<v){ k=BCR_CUDDLE; v=so; }
    if(fu<40&&fu<v){ k=BCR_PLAY; v=fu; }
    if(low) *low=v;
    return k;
}
static const char* const bcCry[BCR_N]={" IS HUNGRY"," NEEDS A CHANGE"," WANTS CUDDLES"," IS BORED"," IS SLEEPY"};
static const char* const bcCryW[BCR_N]={"HUNGRY","NEEDS A CHANGE","WANTS A CUDDLE","WANTS TO PLAY","SLEEPY"};
static const char* const bcDid[BCR_N]={" FED "," CHANGED "," CUDDLED "," PLAYED WITH "," PUT TO BED "};
static const char* const bcStNext[5]={"CHILD","TEEN","ADULT","ELDER",""};
static int famNeed(int st);   // family.h
static int bcToddler(int u){   // a baby in the second half of its stage
    int m=hhMemOf(u); if(m<0||hhM[m].stage!=AG_BABY) return 0;
    int need=famNeed(AG_BABY); return need>=2&&famAge[u]*2>=need;
}
static const char* bcWord(int u,int sx){   // FAMILY panel: BABY GIRL / TODDLER BOY ...
    static const char* const t[SX_N]={"TODDLER GIRL","TODDLER BOY","TODDLER"};
    if(uStage(u)==AG_BABY&&bcToddler(u)) return t[sx<SX_N?sx:SX_NB];
    return whoWord(uStage(u),sx);
}
static u8 bcClaim[HH_MAX];   // steps a baby is spoken for (somebody is on the way)
static char bcB[34];
static int bcMeanNeed(const HhSim*s){ return (s->need[HN_FOOD]+s->need[HN_WC]+s->need[HN_CLEAN]+s->need[HN_SOC]+s->need[HN_FUN])/5; }
static void bcAdd(HhSim*s,int n,int v){ int x=s->need[n]+v; s->need[n]=(u8)(x<0?0:x>100?100:x); }
static int bcPutToBed(int bm){   // carry the baby to the nearest nursery bed (any bed if the house has none): 1 = it is asleep there
    HhSim*b=&hhM[bm]; int bx=(int)(b->fx>>8), by=(int)(b->fy>>8), best=1<<30, tx=-1, ty=-1, any=0;
    for(int pass=0;pass<2&&tx<0;pass++)
        for(int y=0;y<MH;y++) for(int x=0;x<MW;x++){
            if(lifeMap[y][x]!='S') continue; if(pass==0&&!bcCribAt(x,y)) continue; any=1;
            int d=(x-bx)*(x-bx)+(y-by)*(y-by); if(d>=best) continue;
            for(int k=0;k<4;k++){ int nx=x+hhDx[k], ny=y+hhDy[k]; if(hhWalk(nx,ny)&&lifeMap[ny][nx]!='S'&&!hhTaken(nx,ny,b)){ best=d; tx=nx; ty=ny; break; } } }
    (void)any; if(tx<0) return 0;
    b->fx=tx*256+128; b->fy=ty*256+128; b->pn=b->pi=0; b->gok=0; b->act=HA_USE; b->use=HN_REST; b->item=0; b->t=(short)(HH_USE*3); b->jl=b->jt=0;
    return 1;
}
static void bcDo(int bm,int k,int who){   // who gave the care (a household uid, or you)
    HhSim*b=&hhM[bm]; int bx=(int)(b->fx>>8), by=(int)(b->fy>>8), good=0;
    switch(k){
    case BCR_FEED:   good=bcNear(bx,by,3,BI_HIGH); bcAdd(b,HN_FOOD,60+(good?25:0)); bcAdd(b,HN_WC,-8); bcAdd(b,HN_COMFY,10); break;
    case BCR_DIAPER: good=bcNear(bx,by,3,BI_TABLE); b->need[HN_WC]=100; if(good) b->need[HN_CLEAN]=100; else bcAdd(b,HN_CLEAN,45); break;
    case BCR_CUDDLE: good=bcNear(bx,by,3,BI_NURSE); bcAdd(b,HN_SOC,45+(good?15:0)); bcAdd(b,HN_COMFY,25); bcAdd(b,HN_FUN,8); break;
    case BCR_PLAY:   good=bcNear(bx,by,3,BI_PEN)||bcNear(bx,by,3,BI_MOBILE)||bcNear(bx,by,3,BI_TOYS); bcAdd(b,HN_FUN,45+(good?20:0)); bcAdd(b,HN_SOC,15); bcAdd(b,HN_REST,-6); break;
    case BCR_BED:    break;
    }
    if(b->act==HA_USE&&b->use==HN_REST&&k!=BCR_BED) b->act=HA_IDLE;   // (a cry wakes it)
    if(k!=BCR_BED){ hhFreeze(b->uid,60); b->bub=IC_HEART; b->bubT=100; }
    if(who<HU_N&&b->uid<HU_N){ int v=relD[who][b->uid]+2; relD[who][b->uid]=(signed char)(v>100?100:v); v=relD[b->uid][who]+3; relD[b->uid][who]=(signed char)(v>100?100:v); v=relL[who][b->uid]+1; relL[who][b->uid]=(signed char)(v>100?100:v); }
    needAdd(who,HN_FUN,4);
    char*e=simCat(bcB,bcDid[k]); simCat(e,b->name);
    if(who==hhPUid){ static char pb[40]; e=simCat(pb,bcDid[k]+1); e=simCat(e,b->name); if(good) simCat(e,"  NICE SETUP"); lnote=pb; lnoteT=70; }
    else { int wm=hhMemOf(who); if(wm>=0) hhNote(&hhM[wm],bcB); }
}
// ---- the baby itself ----
static int bcBaby(HhSim*s){   // its own decisions: cry, sleep when it must, crawl about when it is a toddler
    int v, k=bcWant(s,&v);
    if(k>=0&&v<30){ s->bub=IC_SAD; s->bubT=100; }
    if(k==BCR_BED||(simIsNight()&&s->need[HN_REST]<85)){   // bedtime: its own bed, a nursery one if there is one
        int r=hhPlan(s,'S'); if(r==1){ s->act=HA_USE; s->use=HN_REST; s->t=hhUseT(s); return 1; } if(r>1){ s->act=HA_WALK; s->use=HN_REST; return 1; } }
    if(bcToddler(s->uid)&&(rnd8()&1)&&hhPlan(s,0)>1){ s->act=HA_WANDER; s->use=HN_FUN; return 1; }
    s->act=HA_IDLE; return 1;
}
static int bcCarer(HhSim*s,int self){   // an awake Sim with free will goes to a baby that needs something
    if(!xo[XO_FREEWILL]||s->stage<AG_CHILD||s->stage==AG_BABY) return 0;
    int lowOwn=100; for(int n=0;n<HN_FUN;n++) if(s->need[n]<lowOwn) lowOwn=s->need[n];
    if(lowOwn<20) return 0;   // looks after itself first
    int best=-1, bv=101;
    for(int m=0;m<hhN;m++){ HhSim*b=&hhM[m]; if(m==self||b->stage!=AG_BABY||b->act==HA_AWAY||bcClaim[m]) continue; if(xo[XO_MULTIFL]&&hhFl[m]!=curFl) continue;
        int v, k=bcWant(b,&v); if(k<0) continue;
        if(b->act==HA_USE&&b->use==HN_REST&&v>=25) continue;   // let sleeping babies sleep
        if(s->stage<AG_TEEN&&k!=BCR_CUDDLE&&k!=BCR_PLAY) continue;   // children cuddle and play; the rest is for the big ones
        int par=kin[b->uid][s->uid]>=KN_DAUGHTER&&kin[b->uid][s->uid]<=KN_CHILD;   // its parent
        int eager=110+(par?90:0)+(v<25?60:0)+s->tr[TR_NICE]*8; if((int)rnd8()>=eager) continue;
        if(v<bv){ bv=v; best=m; } }
    if(best<0) return 0;
    HhSim*b=&hhM[best]; s->tgt=b->uid; hhGX=(int)(b->fx>>8); hhGY=(int)(b->fy>>8);
    int r=hhPlan(s,1); if(r<1) return 0;
    if(r==1) s->pn=s->pi=0;
    s->act=HA_SEEK; bcClaim[best]=240; s->bub=IC_HEART; s->bubT=70; return 1;
}
static int bcDecide(HhSim*s){   // house.h hhDecide, first thing: 1 = handled
    if(s->stage==AG_BABY) return bcBaby(s);
    return bcCarer(s,(int)(s-hhM));
}
static int bcArrive(int m){   // house.h hhArrive: reached a baby it came to look after. 1 = handled
    HhSim*s=&hhM[m]; int bm=hhMemOf(s->tgt); if(bm<0||bm>=hhN||hhM[bm].stage!=AG_BABY) return 0;
    HhSim*b=&hhM[bm]; bcClaim[bm]=0; s->act=HA_IDLE; s->think=(short)HH_THINK;
    int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8), x=(int)(b->fx>>8), y=(int)(b->fy>>8); if((sx-x)*(sx-x)+(sy-y)*(sy-y)>4) return 1;   // it crawled off
    s->hd=(u8)(x>sx?0:x<sx?8:y>sy?4:12);
    int v, k=bcWant(b,&v); if(k<0) k=(rnd8()&1)?BCR_CUDDLE:BCR_PLAY;
    if(s->stage<AG_TEEN&&k!=BCR_CUDDLE&&k!=BCR_PLAY) k=BCR_PLAY;
    if(k==BCR_BED&&!bcPutToBed(bm)) k=BCR_CUDDLE;
    bcDo(bm,k,s->uid); s->act=HA_SOC; s->t=60; s->bub=IC_HEART; s->bubT=90;
    return 1;
}
static int bcNeedy(void){   // the uid of a baby that needs somebody now and is awake (-1: none). For your Sim left alone
    int best=-1, bv=101;
    for(int m=0;m<hhN;m++){ HhSim*b=&hhM[m]; if(b->stage!=AG_BABY||b->act==HA_AWAY||bcClaim[m]) continue; if(xo[XO_MULTIFL]&&hhFl[m]!=curFl) continue;
        int v, k=bcWant(b,&v); if(k<0||k==BCR_BED||(b->act==HA_USE&&v>=25)) continue; if(v<bv){ bv=v; best=b->uid; } }
    return best;
}
static void bcAuto(int bm){ HhSim*b=&hhM[bm]; int v, k=bcWant(b,&v); if(k<0) k=BCR_CUDDLE; if(k==BCR_BED&&!bcPutToBed(bm)) k=BCR_CUDDLE; bcDo(bm,k,hhPUid); }   // selfwill.h: your Sim does the most needed thing
static int bcPlayerAdd(const HhSim*s,const char**it,int*cat,int*id,int n){   // hhSocR: the baby entries of the pie menu
    if(s->stage!=AG_BABY) return n;
    static const char* const nm[BCR_N]={"FEED BOTTLE","CHANGE DIAPER","CUDDLE","PLAY","PUT TO BED"}; static const u8 ct[BCR_N]={4,4,0,1,4};
    for(int k=0;k<BCR_N;k++){ it[n]=nm[k]; cat[n]=ct[k]; id[n++]=-10-k; }
    return n;
}
static void bcPlayer(int bm,int k){   // you chose one
    if(k==BCR_BED&&!bcPutToBed(bm)){ lnote="NO BED FOR A BABY"; lnoteT=70; return; }
    bcDo(bm,k,hhPUid); hhFreeze(hhPUid,50); simEvent(SE_TALK);
}
// ---- time ----
static void bcTick(void){   // house.h hhTick, every step
    for(int m=0;m<hhN;m++){ HhSim*b=&hhM[m]; if(bcClaim[m]) bcClaim[m]--; if(b->stage!=AG_BABY||b->act==HA_AWAY) continue;
        if((int)(lfr%100)==(m*9)%100){ if(b->need[HN_FOOD]>0) b->need[HN_FOOD]--; if(b->need[HN_WC]>0&&(lfr&1)) b->need[HN_WC]--; }   // hungrier and wetter than the rest
        if(b->act==HA_USE&&b->use==HN_REST) continue;
        int v, k=bcWant(b,&v);
        if(k>=0&&v<25&&(int)(lfr%260)==(m*17)%260){   // a wail
            b->bub=IC_SAD; b->bubT=110; s32 dx=(b->fx-lfx)>>8, dy=(b->fy-lfy)>>8; if(dx<0) dx=-dx; if(dy<0) dy=-dy;
            if(dx<=9&&dy<=9){ hhNote(b,bcCry[k]); voxPlay(V_cry); } }
    }
}
// ---- midnight (family.h famDay): 0 = a neglected baby does not grow today, 1 = a day, 2 = a thriving one grows a day faster ----
static char bcNb[2][46]; static u8 bcNbI;
static int bcDay(HhSim*s,int need){
    if(s->stage!=AG_BABY) return 1;
    if(s->need[HN_FOOD]<10||s->need[HN_WC]<10){
        char*e=simCat(bcNb[bcNbI],s->name); simCat(e," WAS NEGLECTED  NO GROWING"); simQueue(bcNb[bcNbI]); bcNbI^=1; return 0; }
    if(need>=3&&bcMeanNeed(s)>=72) return 2;
    return 1;
}
static void bcToddlerNote(HhSim*s){ char*e=simCat(bcNb[bcNbI],s->name); simCat(e," IS A TODDLER  CRAWLS AROUND"); simQueue(bcNb[bcNbI]); bcNbI^=1; }
static int bcCakeParty(HhSim*bday){   // a BIRTHDAY CAKE anywhere in the house: everybody celebrates
    int id=bcId(BI_CAKE), have=0; if(!id) return 0;
    for(int y=0;y<MH&&!have;y++) for(int x=0;x<MW;x++) if(lifeMap[y][x]!='.'&&catIdAt(x,y)==id){ have=1; break; }
    if(!have) return 0;
    for(int m=0;m<hhN;m++){ HhSim*o=&hhM[m]; if(o->act==HA_AWAY) continue; bcAdd(o,HN_FUN,25); bcAdd(o,HN_SOC,20); if(o!=bday&&o->uid<HU_N&&bday->uid<HU_N){ int v=relD[o->uid][bday->uid]+3; relD[o->uid][bday->uid]=(signed char)(v>100?100:v); } }
    needAdd(hhPUid,HN_FUN,25); needAdd(hhPUid,HN_SOC,20);
    return 1;
}
