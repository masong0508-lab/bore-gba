#!/bin/sh
# BORE: (1) lunar seeds: plant as many as you carry, each pot keeps its own 30-day clock (no more "one growing at a time"), pockets hold 3;
#       (2) THE STASH screen: MY STASH + SELL SOME (seeds and grams one by one), PLANT EVERY EMPTY POT, 1/5/10 seed packs.
# Needs bore-lunarweed-patch.sh (the lunar weed one) applied. Run inside ~/bore-gba. Nothing is written unless every required anchor matches.
[ -f source/weed.h ] && [ -f source/main.c ] && [ -f source/self.h ] && [ -f source/lunrado.h ] || { echo "run this inside ~/bore-gba"; exit 1; }
grep -q "LUNAR WEED: bought at" source/lunrado.h || { echo "apply bore-lunarweed-patch.sh first"; exit 1; }
grep -q "lwLock" source/lunrado.h && { echo "already applied"; exit 0; }
python3 -I - <<'EOF'
import re,sys
def rd(p): return open(p).read()
def wr(p,s): open(p,"w").write(s)
W=rd("source/weed.h"); M=rd("source/main.c"); S=rd("source/self.h"); L=rd("source/lunrado.h")
def req(src,old,new,label):
    if src.count(old)!=1: print("anchor not found once:",label); sys.exit(1)
    return src.replace(old,new)
def opt(src,old,new,label):
    if src.count(old)!=1: print("skipped (code differs):",label); return src
    return src.replace(old,new)
# ===== (1) lunar seeds: one clock per pot. The pot byte: bits 0-2 stage (6 growing, 7 ready), bits 3-7 the day it was planted (mod 32) =====
L=req(L,"static int lwPlantDays(void){ return lwG(lwRd(),17,5); }   // 0 none, 1 planted .. 31 ready (30 days on)",
 "static int lwStamp(void){ return (((int)simDay>>1)%31)+1; }   // the pot keeps the day it was planted in 2 day steps (1..31; 0 = a plant from the old one-counter version)\nstatic int lwUnits(int b){ int s=b>>3; if(!s) return 0; return ((((int)simDay>>1)%31)-(s-1)+31)%31; }   // 2 day steps since planted; 15 = ready (30 days)\nstatic int lwDaysOf(int b){ return (b&7)==7?31:lwUnits(b)>=15?31:lwUnits(b)*2+1; }   // 1 planted .. 31 ready, from the pot's byte\nstatic void lwLock(void){   // AUTOMATIC: every game minute and on every room load, each lunar pot on the live floor: a ready plant locks as ready (stage 7); an old-version plant takes its age from the old counter\n    for(int y=0;y<MH;y++) for(int x=0;x<MW;x++){ if(lifeMap[y][x]!='u') continue; int b=wallMap[y][x], st=b&7; if(st!=6) continue;\n        if(!(b>>3)){ u32 v=lwRd(); int p=lwG(v,17,5); int u=p>1?(p-1)/2:0; if(p) lwWr(lwP(v,17,5,0)); wallMap[y][x]=(u8)(6|((((((int)simDay>>1)%31)-u+31)%31+1)<<3)); continue; }\n        if(lwUnits(b)>=15) wallMap[y][x]=7; } }","lwPlantDays")
L=req(L,"static void lwDrawPlant(int sx,int sy,int t){","static void lwDrawPlant(int sx,int sy,int d,int t){","lwDrawPlant head")
L=req(L,"    int d=lwPlantDays(); int h=3+d*21/31, top=sy-7-h;","    int h=3+d*21/31, top=sy-7-h;","lwDrawPlant body")
L=req(L,"    int d=lwPlantDays(), ready=d>=31;","    int d=lwDaysOf(wallMap[y][x]), ready=d>=31; if(ready) wallMap[y][x]=7;","lwPlanter days")
L=req(L,"wpSet(x,y,WP_EMPTY,0,0); lwWr(lwP(lwRd(),17,5,0)); moodEvent(M_PAY);","wpSet(x,y,WP_EMPTY,0,0); moodEvent(M_PAY);","harvest")
L=req(L,"u32 v=lwRd(); int s=lwG(v,15,2); if(s<=0||lwG(v,17,5)){ toast(\"NO LUNAR SEED  OR ONE IS ALREADY GROWING\"); return; }\n    wpSet(x,y,6,0,0); lwWr(lwP(lwP(v,15,2,s-1),17,5,1));",
 "u32 v=lwRd(); int s=lwG(v,15,2); if(s<=0){ toast(\"NO LUNAR SEED\"); return; }\n    wallMap[y][x]=(u8)(6|(lwStamp()<<3)); lwWr(lwP(v,15,2,s-1));","lwPlantSeed")
L=req(L,"    if(simMin==0){ u32 v=lwRd(); int p=lwG(v,17,5); if(p>=1&&p<31) lwWr(lwP(v,17,5,p+1)); }   // a day on: the lunar plant grows",
 "    lwLock();   // the pots keep their own clocks: a ready plant locks as ready, on any floor you are on, at once","lwMinute")
L=req(L,"if(s>=2){ toast(\"YOUR POCKETS ARE FULL OF SEEDS\"); return; }","if(s>=3){ toast(\"YOUR POCKETS ARE FULL OF SEEDS\"); return; }","pocket cap")
L=req(L,"s=s+g>2?2:s+g;","s=s+g>3?3:s+g;","pocket cap 2")
W=req(W,"static int lwPlantDays(void); static void lwDrawPlant(int sx,int sy,int t);","static void lwDrawPlant(int sx,int sy,int d,int t);","fwd decls")
W=req(W,"    if(st==6){ lwDrawPlant(sx,sy,t); return; }","    if(st>=6){ int b=st|(wa<<3)|(sn<<5); lwDrawPlant(sx,sy,lwDaysOf(b),t); return; }   // (lunrado.h: the pot's byte holds the planting day)","draw")
W=req(W,"    if(st==6){ lwPlanter(x,y); return; }","    if(st>=6){ lwPlanter(x,y); return; }","planter")
W=req(W,"if(lwSeedsN()>0&&!lwPlantDays()){ it[n]=\"PLANT LUNAR SEED\"; id[n++]=6; }","if(lwSeedsN()>0){ it[n]=\"PLANT LUNAR SEED\"; id[n++]=6; }","menu row")
W=req(W,"static int lwDaysOf","static int lwDaysOf","x") if False else W
W=req(W,"static void lwMinute(void);","static void lwMinute(void); static int lwDaysOf(int b);","fwd lwDaysOf")
# ===== (2) the stash =====
M=opt(M,'static void mapScan(void){   // find the skateboard (B), the spawn point (P) and the flags (a, k); fall back to sane defaults\n    wDirty=1;','static void lwLock(void);   // lunrado.h\nstatic void mapScan(void){   // find the skateboard (B), the spawn point (P) and the flags (a, k); fall back to sane defaults\n    wDirty=1; lwLock();   // (lunrado.h: lunar pots lock as ready on every room load)','mapScan hook')
if "stash.h" in M:
    print("stash.h already there: only moving its plant-all row to id 7")
    W=opt(W,'id[n++]=6; } }   // stash.h','id[n++]=7; } }   // stash.h',"stash row id")
    W=opt(W,'        case 6: { int s=wdPickStrain("PLANT WHICH SEEDS",1);','        case 7: { int s=wdPickStrain("PLANT WHICH SEEDS",1);',"stash case id")
    wr("source/main.c",M); wr("source/self.h",S); wr("source/weed.h",W); wr("source/lunrado.h",L); print("lunar seeds: one clock per pot"); sys.exit(0)
m=re.search(r'\n[^\n]*#include "weed\.h"[^\n]*',M)
if not m: print("main.c: weed.h include not found"); sys.exit(1)
M=M[:m.end()]+'\n#include "stash.h"           // THE STASH: seeds and grams one by one: USE, PLANT, SELL (needs weed.h)'+M[m.end():]
S=req(S,'else if(pick==-11) wdCorner();','else if(pick==-11) wdCorner(); else if(pick==-13) stInv(0);',"self.h pick")
S=req(S,'if(c==2&&wdOk()&&wdAll()&&q<8){ l[q]="ROLL ONE UP";','if(c==2&&wdOk()&&stCount()&&q<8){ l[q]="MY STASH"; col[q]=RGB(8,24,10); id[q++]=-13; }   // stash.h\n        if(c==2&&wdOk()&&wdAll()&&q<8){ l[q]="ROLL ONE UP";',"self.h row")
smoke=re.search(r'static void wdSmokeUp\(int s,int g,int fancy\)',W) is not None
W=req(W,'static void wdPlanter(int x,int y){','static int stPots(void); static int stPlantAll(int s); static int stBuySeeds(int s,int each); static void stInv(int sell);   // stash.h\nstatic void wdPlanter(int x,int y){',"fwd stash")
W=opt(W,'const char* it[4]; int id[4], n=0;','const char* it[6]; int id[6], n=0;',"planter arrays")
W=opt(W,'id[n++]=0; if(lwSeedsN()>0){','id[n++]=0; if(stPots()>1){ it[n]="PLANT EVERY EMPTY POT"; id[n++]=7; } if(lwSeedsN()>0){',"planter row")
W=opt(W,'        case 1: wpSet(x,y,st,3,sn); lnote="WATERED";','        case 7: { int s=wdPickStrain("PLANT WHICH SEEDS",1); if(s<0){ toast("NO SEEDS  THE DISPENSARY SELLS THEM"); lstun=0; return; } stPlantAll(s); break; }   // stash.h\n        case 1: wpSet(x,y,st,3,sn); lnote="WATERED";',"planter case")
W=opt(W,'const char* it[10]; char lb[5][34]; int id[10];','const char* it[12]; char lb[5][34]; int id[12];',"counter arrays")
W=opt(W,'        it[n]="ASK THE BUDTENDER"; id[n++]=8;','        it[n]="SELL SOME"; id[n++]=10;   // stash.h\n        it[n]="ASK THE BUDTENDER"; id[n++]=8;',"counter row")
W=opt(W,'        if(c==2){ dpJars(); continue; }','        if(c==10){ stInv(1); continue; }   // stash.h\n        if(c==2){ dpJars(); continue; }',"counter handler")
old_buy='int s=menu("SEEDS  A PACK",si,WD_N); if(s<0) continue; if(simMoney<dpPrice(60)){ toast("NOT ENOUGH SIMOLEONS"); continue; } simMoney-=dpPrice(60); wdSeed[s]=(u8)(wdSeed[s]<99?wdSeed[s]+1:99); wdSave(); sfxPlay(SFX_CASH); toast("A PACK OF SEEDS  PLANT IT AT HOME"); continue; }'
W=opt(W,old_buy,'int s=menu("SEEDS  A PACK",si,WD_N); if(s<0) continue; if(stBuySeeds(s,dpPrice(60))) toast("SEEDS  PLANT THEM AT HOME"); continue; }   // stash.h: 1, 5 or 10 packs',"buy packs")
STASH=r'''// stash.h - THE STASH: seeds and grams one by one (MY STASH in YOUR ACTIONS > FUN; SELL SOME at the dispensary counter). Needs weed.h.
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
'''
if smoke: STASH=STASH.replace("// stash.h - THE STASH","#define ST_SMOKE 1\n// stash.h - THE STASH",1)
else: print("note: wdSmokeUp(int,int,int) not found: no ROLL row in the stash")
wr("source/main.c",M); wr("source/self.h",S); wr("source/weed.h",W); wr("source/lunrado.h",L); wr("source/stash.h",STASH)
print("lunar seeds: one clock per pot; stash screen wired in")
EOF
