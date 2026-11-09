// ggmenu.h - GODGUY: filtered stylus input, touch hit slop, the STATS page and the WORLD page (parameters of the world, peoples, cities and kingdoms).
// ---- the stylus. The DS pen is noisy: the first sample of a contact bounces, the last one before release drifts, and now and then a (0,0) ghost comes through.
// penPoll() drops the first sample, ignores ghosts and wild jumps (once), takes the median of three, holds a 1 pixel dead zone against jitter and makes a hold repeat.
static struct { int held, press, rel, rep, moved, x, y, lx, ly, age, t, n, bad, out, fromUi, fromBar, sx[3], sy[3]; } pen = { .age=-1 };
static int med3(int a,int b,int c){ int t; if(a>b){ t=a; a=b; b=t; } if(b>c) b=c; return a>b?a:b; }
static inline int iabs(int v){ return v<0?-v:v; }
static void penPoll(int touching,int uiOn){
    // v2: no sample is thrown away (the ARM7 already averages five conversions and holds a contact through noise), so a tap lands on the frame it happens and a stroke starts at once;
    // the median only smooths a hand that is nearly still, a moving pen is followed exactly (a median of three lags a stroke by a frame).
    pen.press=pen.rel=pen.rep=pen.moved=0; pen.lx=pen.x; pen.ly=pen.y;
    if(!touching){ if(pen.held) pen.rel=1; pen.held=0; pen.age=-1; pen.n=0; pen.bad=0; pen.out=0; return; }
    pen.age++; touchPosition tp; touchRead(&tp); int rx=tp.px, ry=tp.py;
    if((rx==0&&ry==0&&pen.held)||rx<0||ry<0||rx>255||ry>191){ pen.bad++; return; }   // ghost sample (a (0,0) is believed on the first frame of a contact: the top left corner is a real place)
    if(pen.held&&pen.out<2){ int dx=rx-pen.x, dy=ry-pen.y; if(iabs(dx)+iabs(dy)>110){ pen.out++; return; } }   // a wild jump is believed only if it repeats
    pen.out=0; pen.bad=0; pen.sx[2]=pen.sx[1]; pen.sx[1]=pen.sx[0]; pen.sx[0]=rx; pen.sy[2]=pen.sy[1]; pen.sy[1]=pen.sy[0]; pen.sy[0]=ry; if(pen.n<3) pen.n++;
    int x=rx, y=ry;
    if(pen.n==3){ int xl=pen.sx[0], xh=xl, yl=pen.sy[0], yh=yl; for(int i=1;i<3;i++){ if(pen.sx[i]<xl) xl=pen.sx[i]; if(pen.sx[i]>xh) xh=pen.sx[i]; if(pen.sy[i]<yl) yl=pen.sy[i]; if(pen.sy[i]>yh) yh=pen.sy[i]; }
        if(xh-xl<=2&&yh-yl<=2){ x=med3(pen.sx[0],pen.sx[1],pen.sx[2]); y=med3(pen.sy[0],pen.sy[1],pen.sy[2]); } }
    if(!pen.held){ pen.held=1; pen.press=1; pen.fromUi=uiOn; pen.fromBar=!uiOn&&y>=TB_Y; pen.x=pen.lx=x; pen.y=pen.ly=y; pen.t=0; return; }
    if(iabs(x-pen.x)>1||iabs(y-pen.y)>1){ pen.x=x; pen.y=y; pen.moved=1; }
    pen.t++; if(pen.t>=22&&((pen.t-22)%5)==0) pen.rep=1; }
static int uiHitSlop(int px,int py){   // the exact button, or the nearest one within 4 pixels (fingers are fat, styluses drift)
    int h=uiHit(px,py); if(h>=0) return h; int sl=4+3*hwUiScale, best=-1, bd=sl*sl+1;
    for(int i=uiNBtn-1;i>=0;i--){ const UiBtn*b=&uiBtns[i]; if(b->w>=200&&b->h>=150) continue; int dx=px<b->x?b->x-px:px>=b->x+b->w?px-(b->x+b->w-1):0, dy=py<b->y?b->y-py:py>=b->y+b->h?py-(b->y+b->h-1):0, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=i; } }
    return best; }
static int menuRetScr=2, menuRetSel=3;
static void menuSelId(int id){ int ids[14]; int n=menuList(ids); for(int i=0;i<n;i++) if(ids[i]==id){ mSel=i; return; } mSel=0; }
// ---- STATS
static int raceCnt(int r){ int n=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&sp[cr[i].sp].a==r) n++; return n; }
static void uiStats(void){
    int alive=0; for(int i=0;i<nCr;i++) alive+=cr[i].alive; int hh; int folk=vilTotals(&hh); int nk=0; for(int k=0;k<KN_N;k++) nk+=kd[k].on; char b[48];
    siprintf(b,"YEAR %d  %s",year,ageN[worldAge()]); uiWin("STATS",b,0);
    struct { const char*l; int v; } r[12]={{"LIFE",alive},{"FOLK",folk},{"HOMES",hh},{"VILLAGES",nVilOn},{"KINGDOMS",nk},{"MONSTERS",monCount()},{"BORN",(int)stBorn},{"DIED",(int)stDied},{"SLAIN",(int)stKill},{"WARS",(int)stWars},{"GIFTS",(int)stGift},{"POWERS",(int)stUse}};
    for(int i=0;i<12;i++){ int x=6+(i/6)*124, y=18+(i%6)*11; uiText(x,y,r[i].l,U_DIM,1); siprintf(b,"%d",r[i].v); uiTextR(x+114,y,b,U_WHITE); }
    uiText(6,88,"THE PEOPLES",U_GOLD,1); int mx=1, c[8]; for(int i=0;i<8;i++){ c[i]=raceCnt(i); if(c[i]>mx) mx=c[i]; }
    for(int i=0;i<8;i++){ int y=98+i*10, v=0; for(int q=0;q<VIL_N;q++) v+=vil[q].on&&vil[q].race==i; uiText(8,y,arch[i].nm,U_LGREY,1); siprintf(b,"%d",c[i]); uiTextR(88,y,b,U_WHITE); siprintf(b,"%dV",v); uiTextR(112,y,b,U_DIM); uiBar(118,y,108,7,c[i],mx,U_G0+i); }
    uiFlush(); }
// ---- WORLD: four tabs, rows that change a parameter (LEFT RIGHT or tap the arrows), rows that do something (A)
static const struct { const char*n[5]; int c; } wk[12]={ {{"FEW","NORMAL","MANY"},3},{{"LOW","NORMAL","HIGH"},3},{{"SHORT","NORMAL","LONG"},3},{{"OFF","RARE","NORMAL"},3},{{"FRAIL","NORMAL","HARDY"},3},{{"PEACEFUL","AS IT WAS","WARLIKE"},3},{{"NORMAL","BOOM","SLOW"},3},{{"NORMAL","PACIFIST","FORTIFIED"},3},{{"NEVER","ALLOWED"},2},{{"AUTO","CLEAR","RAIN","STORM","SNOW"},5},{{"AUTO","DAY","DAWN","DUSK","NIGHT"},5},{{"WARLIKE","PEACEFUL","GREEDY","WISE"},4} };
typedef struct { const char*lb; u8*p; int k, act; const char*ds; } WR;
static WR wr[12]; static int wn, wTab, wSel, wTabPrev, wPeo, wCity, wKing; static char wVal[12][20];
static const char* const wTabN[4]={"WORLD","PEOPLES","CITIES","KINGS"};
static int wNextOn(int cur,int d,int total,int isCity){ for(int k=1;k<=total;k++){ int i=((cur+d*k)%total+total)%total; if(isCity?vil[i].on:kd[i].on) return i; } return cur; }
static void worldBuild(void){
    wn=0;
#define WRW(L_,P_,K_,A_,D_) do{ wr[wn].lb=L_; wr[wn].p=P_; wr[wn].k=K_; wr[wn].act=A_; wr[wn].ds=D_; wn++; }while(0)
    if(wTab==0){ WRW("MONSTERS",&gx.mon,0,0,"HOW MANY NIGHTMARES THE RIFTS OPEN"); WRW("BIRTH RATE",&gx.birth,1,0,"HOW FAST EVERYONE BREEDS"); WRW("HUNGER",&gx.hunger,1,0,"HOW QUICKLY BELLIES EMPTY"); WRW("LIFESPAN",&gx.life,2,0,"HOW LONG LIVES LAST"); WRW("DISASTERS",&gx.dis,3,0,"COMETS, LIGHTNING, CREEPING LAVA"); WRW("WARS",&gx.war,8,0,"CAN KINGDOMS DECLARE WAR"); WRW("WEATHER",&gx.wx,9,0,"LOCK THE SKY"); WRW("TIME OF DAY",&gx.tm,10,0,"LOCK THE LIGHT"); WRW("RESET WORLD",0,-1,1,"BACK TO THE DEFAULTS (A)"); }
    else if(wTab==1){ WRW("PEOPLE",0,-1,7,"LEFT RIGHT: CHOOSE A PEOPLE"); WRW("BIRTH RATE",&gx.rBirth[wPeo],1,0,"HOW FAST THIS PEOPLE BREEDS"); WRW("TOUGHNESS",&gx.rVigor[wPeo],4,0,"HEALTH OF THIS PEOPLE"); WRW("TEMPER",&gx.rTemper[wPeo],5,0,"WILL THEY GO LOOKING FOR WAR"); WRW("LIFESPAN",&gx.rLife[wPeo],2,0,"HOW LONG THEY LIVE"); WRW("RESET PEOPLE",0,-1,8,"BACK TO NORMAL (A)"); }
    else if(wTab==2){ if(vil[wCity].on||(wCity=wNextOn(wCity,1,VIL_N,1),vil[wCity].on)){ WRW("CITY",0,-1,7,"LEFT RIGHT: CHOOSE A CITY"); WRW("GROWTH",&gx.vGrow[wCity],6,0,"HOW FAST IT GROWS"); WRW("DEFENCE",&gx.vDef[wCity],7,0,"PACIFIST NEVER MARCHES, FORTIFIED IS HARD TO TAKE"); WRW("FILL STORE",0,-1,2,"FOOD AND GOODS TO THE BRIM (A)"); WRW("GO THERE",0,-1,3,"LOOK AT THIS CITY (A)"); } }
    else { if(kd[wKing].on||(wKing=wNextOn(wKing,1,KN_N,0),kd[wKing].on)){ WRW("KINGDOM",0,-1,7,"LEFT RIGHT: CHOOSE A KINGDOM"); WRW("RULER TEMPER",&kd[wKing].rtr,11,0,"HOW THE RULER BEHAVES"); WRW("MAKE PEACE",0,-1,4,"END ITS WARS (A)"); WRW("GIFT 200 GOLD",0,-1,5,"FILL ITS TREASURY (A)"); WRW("NEW RULER",0,-1,6,"THE OLD ONE FALLS (A)"); } }
    for(int i=0;i<wn;i++){ char*v=wVal[i]; v[0]=0; if(wr[i].k>=0) strcpy(v,wk[wr[i].k].n[*wr[i].p%wk[wr[i].k].c]);
        else if(wr[i].act==7){ if(wTab==1) siprintf(v,"%.12s",arch[wPeo].nm); else if(wTab==2){ char nm[24]; vilName(wCity,nm); siprintf(v,"%.12s",nm); } else { char nm[24]; kdName(wKing,nm); siprintf(v,"%.12s",nm); } } else if(wr[i].act) strcpy(v,">"); }
    if(wSel>=wn) wSel=wn?wn-1:0; }
static void worldAct(int a){
    if(a==1){ u32 b=stBorn, d=stDied; gxDefaults(); stBorn=b; stDied=d; siprintf(info,"WORLD RESET"); }
    else if(a==8){ gx.rBirth[wPeo]=gx.rVigor[wPeo]=gx.rTemper[wPeo]=gx.rLife[wPeo]=1; siprintf(info,"PEOPLE RESET"); }
    else if(a==2){ vil[wCity].stock=400; siprintf(info,"STORE FILLED"); }
    else if(a==3){ BrE e; e.type=1; e.id=(u16)wCity; brGo(&e); }
    else if(a==4){ for(int o=0;o<KN_N;o++) if(kd[wKing].war>>o&1) kdWar(wKing,o,0); for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==wKing+1) vil[v].foe=0; warMatrix(); siprintf(info,"PEACE"); }
    else if(a==5){ int g=kd[wKing].gold+200; kd[wKing].gold=(u16)(g>60000?60000:g); siprintf(info,"+200 GOLD"); }
    else if(a==6){ Kd*K=&kd[wKing]; K->rn1=(u8)(rnd()%12); K->rn2=(u8)(rnd()%8); K->rtr=(u8)(rnd()%4); K->reign=0; siprintf(info,"A NEW RULER"); } }
static void worldStep(int d){ if(wTab==1) wPeo=(wPeo+8+d)%8; else if(wTab==2) wCity=wNextOn(wCity,d,VIL_N,1); else if(wTab==3) wKing=wNextOn(wKing,d,KN_N,0); }
static void worldInput(int k){
    if(wTab!=wTabPrev){ wTabPrev=wTab; wSel=0; } worldBuild();
    if(k&KEY_X){ wTab=(wTab+1)%4; } if(k&KEY_Y){ wTab=(wTab+3)%4; } if(wTab!=wTabPrev){ wTabPrev=wTab; wSel=0; worldBuild(); }
    if(!wn) return; if(k&KEY_UP) wSel=(wSel+wn-1)%wn; if(k&KEY_DOWN) wSel=(wSel+1)%wn;
    int d=((k&KEY_RIGHT)?1:0)-((k&KEY_LEFT)?1:0); WR*r=&wr[wSel];
    if(d||(k&KEY_A)){ int s=d?d:1;
        if(r->k>=0){ int c=wk[r->k].c; *r->p=(u8)((*r->p+c+s)%c); fxReq(FX_MENU_MOVE); }
        else if(r->act==7) worldStep(s); else if(k&KEY_A) worldAct(r->act); } }
static void uiWorld(void){
    if(wTab!=wTabPrev){ wTabPrev=wTab; wSel=0; } worldBuild(); uiWin("WORLD","X Y: TAB",0);
    for(int i=0;i<4;i++){ int x=4+i*62, sel=i==wTab; uiRound(x,15,60,13,sel?U_SEL:U_PANELHI,sel?U_GOLD:U_BHI); uiTextC(x+30,18,wTabN[i],sel?U_WHITE:U_DIM,1); uiBtn(x,15,60,13,0,0,&wTab,i); }
    if(!wn){ uiText(10,40,wTab==2?"NO CITIES YET.":"NO KINGDOMS YET.",U_DIM,1); uiFlush(); return; }
    for(int i=0;i<wn;i++){ int y=32+i*13, sel=i==wSel, adj=wr[i].k>=0||wr[i].act==7;
        uiRound(4,y,226,12,sel?U_SEL:U_PANELHI,sel?U_GOLD:U_BHI); uiText(10,y+2,wr[i].lb,sel?U_WHITE:U_LGREY,1);
        uiBtn(4,y,226,12,adj?0:KEY_A,0,&wSel,i);
        if(adj){ uiBtn(120,y,50,12,KEY_LEFT,0,&wSel,i); uiBtn(176,y,54,12,KEY_RIGHT,0,&wSel,i); if(sel){ uiIcon8(122,y+2,IC8_LEFT); uiIcon8(216,y+2,IC8_RIGHT); } uiTextC(170,y+2,wVal[i],sel?U_GOLD:U_DIM,1); }
        else uiTextR(224,y+2,wVal[i],sel?U_GOLD:U_DIM); }
    int y=32+wn*13+4; char b[64];
    if(wTab==2){ const Vil*V=&vil[wCity]; siprintf(b,"POP %d HOMES %d FARMS %d TOWERS %d",V->pop,V->houses,V->farms,V->towers); uiText(8,y,b,U_WHITE,1); if(vilK[wCity]){ char kn[24]; kdName(vilK[wCity]-1,kn); siprintf(b,"%s LOYALTY %d",kn,vilL[wCity]); uiText(8,y+10,b,U_DIM,1); } }
    else if(wTab==3){ int c,p,w; kdStats(wKing,&c,&p,&w); siprintf(b,"%s  %dC %dP POWER %d",kd[wKing].empire?"EMPIRE":kd[wKing].lord?"VASSAL":"KINGDOM",c,p,w); uiText(8,y,b,U_WHITE,1); siprintf(b,"GOLD %d  %s",kd[wKing].gold,kd[wKing].war?"AT WAR":"AT PEACE"); uiText(8,y+10,b,kd[wKing].war?U_RED:U_GREEN,1); }
    else if(wTab==1){ siprintf(b,"%d ALIVE",raceCnt(wPeo)); uiText(8,y,b,U_WHITE,1); }
    uiText(8,168,wr[wSel].ds,U_DIM,1); uiFlush(); }
