// statscreen.h - PAUSE > MY SIM > MORE > LIFETIME STATS: pages 1 and 2 are the stats of the Sim you control, page 3 the totals of the whole game. L / R change the page.
// Needs: lifestats.h, stBack, text, tw, numText, present, keyNow, simCat / simCatN, house.h (HU_N, HH_REC, hhPName ...) for the SRAM checks below.
_Static_assert(LS_CH==HU_N,"one stats row per household uid");
_Static_assert(LS_OFF+3+4*LS_N*LS_CH<=SLOT_BASE&&LG_OFF+3+4*LG_N<=LS_OFF,"the stats blocks overflow the household area");
_Static_assert(LG_OFF>=SL_HH_OFF+4+2*HH_NM+HH_MAX*HH_REC+2*HU_N*HU_N*4+8,"the stats blocks overlap the household block");
static char* lsTime(char*b,u32 s){ char*e=simCatN(b,(int)(s/3600)); e=simCat(e," H "); e=simCatN(e,(int)(s/60%60)); e=simCat(e," MIN "); e=simCatN(e,(int)(s%60)); return simCat(e," S"); }
static int lsI(u32 v){ return (int)(v>0x7FFFFFFF?0x7FFFFFFF:v); }
static u32 lsPlayerSecs(int slot){   // hours of a player's save file (all the Sims of the household), in seconds (0 = none saved)
    SlInfo I; if(!slInfo(slot,&I)) return 0;
    SlR r={SLB(slot)+SLOT_HDR,0,I.len,0};
    while(r.pos<I.len){ int tag=slrGet(&r); if(!tag) break; int cl=slrGet16(&r); if(r.bad||r.pos+cl>I.len) break;
        if(tag==SLC_STATS&&cl>=8){ volatile u8*p=r.p+r.pos; u32 t=0; for(int c=0;c<LS_CH&&4*(c*LS_N+LS_SECS)+4<=cl;c++) t+=lsGet(p,c*LS_N+LS_SECS); return t; }
        r.pos+=cl; }
    return 0;
}
static const char* lsRank(u32 p,u32*next,u32*lo){   // a title for the lifetime score, the score of the next one and where this one began
    static const u32 at[6]={0,5000,25000,100000,500000,2000000}; static const char* const nm[6]={"NEWBIE","AMATEUR","SPONSORED","PRO","LEGEND","GOAT"};
    int i=0; while(i<5&&p>=at[i+1]) i++; *next=i<5?at[i+1]:0; *lo=at[i]; return nm[i];
}
static void statsScreen(void){
    lsEnsure(); static char b[32] EWRAM_BSS, t[28] EWRAM_BSS; u16 prev=keyNow(); u32 cnt=0; int pg=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START|K_A)) return;
        if(pr&(K_R|K_RIGHT)) pg=(pg+1)%3;
        if(pr&(K_L|K_LEFT)) pg=(pg+2)%3;
        if(cnt&7){ vsync(); continue; }
        const u16 lab=RGB(22,25,28);
        if(pg<2){ char*e=simCat(t,"STATS  "); simCat(e,hhPName); } else simCat(t,"GAME TOTALS");
        stBack(t,(int)cnt);
        if(pg==0){
            text(10,21,"TIME PLAYED",GOLD,1); lsTime(b,lsc[LS_SECS]); text(230-tw(b,1),21,b,WHITE,1);
            text(10,34,"LIFETIME SCORE",GOLD,1); numText(150,34,lsI(lsc[LS_SCORE]),GOLD);
            static const char* const nm[8]={"TRICKS LANDED","COMBOS BANKED","BEST COMBO","BEST TRICK","BAILS","DEATHS","DAYS LIVED","LIVES LIVED"};
            for(int i=0;i<8;i++){
                u32 v=i==0?lsc[LS_TRICKS]:i==1?lsc[LS_COMBOS]:i==2?lsc[LS_BESTCOMBO]:i==3?lsc[LS_BESTTRICK]:i==4?lsc[LS_BAILS]:i==5?lsc[LS_DEATHS]:i==6?lsc[LS_DAYS]:(lsc[LS_LIVES]?lsc[LS_LIVES]:1);
                text(10,48+i*11,nm[i],lab,1); numText(150,48+i*11,lsI(v),WHITE);
            }
        } else if(pg==1){
            text(10,21,"STEPS TAKEN",GOLD,1); numText(150,21,lsI(lsc[LS_STEPS]),GOLD);
            static const char* const nm[7]={"WANTS MET","PROMOTIONS","JUMPS AND LAUNCHES","RAILS GRINDED","MEALS EATEN","TIME ON THE BOARD","TIME GRINDING"};
            for(int i=0;i<7;i++){
                u32 v=lsc[i==0?LS_WANTS:i==1?LS_PROMOS:i==2?LS_AIRS:i==3?LS_GRINDS:i==4?LS_MEALS:i==5?LS_BOARDSECS:LS_GRINDSECS];
                text(10,36+i*10,nm[i],lab,1);
                if(i>=5){ lsTime(b,v); text(230-tw(b,1),36+i*10,b,WHITE,1); } else numText(170,36+i*10,lsI(v),WHITE);
            }
        } else {
            static const char* const nm[10]={"TOTAL TIME PLAYED","STEPS TAKEN","TOTAL SCORE","TRICKS LANDED","COMBOS BANKED","BAILS","DEATHS","DAYS LIVED","JUMPS AND LAUNCHES","RAILS GRINDED"};
            for(int i=0;i<10;i++){
                text(10,21+i*9,nm[i],i==0?GOLD:lab,1);
                if(i==0){ lsTime(b,lg[LG_SECS]); text(230-tw(b,1),21,b,WHITE,1); } else numText(170,21+i*9,lsI(lg[i==1?LG_STEPS:i==2?LG_SCORE:i==3?LG_TRICKS:i==4?LG_COMBOS:i==5?LG_BAILS:i==6?LG_DEATHS:i==7?LG_DAYS:i==8?LG_AIRS:LG_GRINDS]),WHITE);
            }
            int ch=0; for(int c=0;c<LS_CH;c++) if(lsAll[c][LS_SECS]) ch++;
            text(10,111,"SESSIONS",lab,1); numText(170,111,lsI(lg[LG_BOOTS]),WHITE);
            text(10,120,"SIMS PLAYED IN THIS HOUSE",lab,1); numText(170,120,ch,WHITE);
        }
        if(pg<2){ u32 nx,lo; const char*rk=lsRank(lsc[LS_SCORE],&nx,&lo); text(10,117,"RANK",lab,1); text(40,117,rk,GOLD,1);
            int w=nx?(int)((u32)(lsc[LS_SCORE]-lo)*120/(nx-lo)):120; rect(100,118,120,6,RGB(4,9,18)); rect(100,118,w,6,nx?RGB(6,18,10):GOLD); if(nx&&w>0) rect(100,118,1,6,GOLD); }
        { const char*pn=pg==0?"PAGE 1 OF 3":pg==1?"PAGE 2 OF 3":"PAGE 3 OF 3"; text(10,128,pn,GOLD,1); text(230-tw("L R PAGE",1),128,"L R PAGE",RGB(17,29,31),1); }
        text(10,138,pg<2?"THIS SIM ONLY  SELECT SWITCHES SIMS":"EVERY SIM AND EVERY PLAYER",RGB(12,14,16),1);
        text(10,148,"A OR B BACK",RGB(12,14,16),1);
        present();
    }
}
