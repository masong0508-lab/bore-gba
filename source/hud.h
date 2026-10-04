// hud.h - BORE life-mode screen furniture, in the spirit of The Sims 2 control panel. Include AFTER sims.h / mood.h / the life globals, faceArt/drawFace and
// text()/rect(), and BEFORE lifeDraw.
//
//  TOP BAR    rows 0..HUD_TOPH-1: score and cash on the left, the one message that matters in the middle (prompt > note > combo > hint), the clock on the right.
//  BOTTOM     rows HUD_BOT..159: a portrait that shows the mood, the eight needs as bars, the aspiration level and the two wants (or the fear).
//  OVER HEAD  the plumbob (mood diamond) or a thought bubble. They are the only things drawn on top of the room: the room itself is drawn in
//             rectangles (see liveDraw in main.c), so anything drawn over it has to be told to the renderer (hudOverlayRc).
//
// The panels never overlap the room, so they are only redrawn when something they show changes (hudKeys), then copied to the screen.
#define HUD_TOPH 10          // rows of the top bar
#define HUD_BOTY 130         // first row of the bottom panel
#define HUD_BOTH (SH-HUD_BOTY)

#define HC_BG0   RGB(3,5,11)
#define HC_BG1   RGB(5,9,18)
#define HC_EDGE  RGB(14,21,30)
#define HC_DARK  RGB(1,2,5)
#define HC_LABEL RGB(20,25,30)
#define HC_GOLD  RGB(31,26,7)
#define HC_DIM   RGB(11,14,17)

typedef struct { short x0,y0,x1,y1; } Rc;
// The panels are drawn in pieces: each piece remembers what it last showed (hudKeys) and is redrawn, and handed to the screen copy (hudRc), only
// when that changed. The first draw after anything covered the screen (all=1) draws every piece.
enum { HK_SCORE, HK_CASH, HK_MSG, HK_CLOCK, HK_NEED, HK_PORT=HK_NEED+8, HK_HEAD, HK_ASP, HK_W0, HK_W1, HK_HP, HK_N };
static unsigned hudKeys[HK_N];
#define HUD_NR 24
static Rc hudRc[HUD_NR]; static int hudRcN;
static void hudMark(int x,int y,int w,int h){ if(hudRcN<HUD_NR){ Rc*r=&hudRc[hudRcN++]; r->x0=(short)x; r->y0=(short)y; r->x1=(short)(x+w); r->y1=(short)(y+h); } }
static unsigned hudHash(const char*t){ unsigned h=2166136261u; while(*t) h=(h^(unsigned char)*t++)*16777619u; return h|1u; }
static int hudChg(int all,int k,unsigned v){ if(all||hudKeys[k]!=v){ hudKeys[k]=v; return 1; } return 0; }
static u16 hudBgAt(int y){ return (y<HUD_TOPH)?((y&~0)<(HUD_TOPH/2)?HC_BG1:HC_BG0): (y-HUD_BOTY<HUD_BOTH/2?HC_BG1:HC_BG0); }
static void hudClear(int x,int y,int w,int h){ for(int j=0;j<h;j++) rect(x,y+j,w,1,hudBgAt(y+j)); }

static u16 hudLvlCol(int v){   // Sims colours: green is good, yellow is getting low, red is urgent
    return v>=55?RGB(9,27,8): v>=28?RGB(29,25,5): RGB(30,7,6);
}
static int hudBarPx(int v){ if(v<0) v=0; if(v>100) v=100; return (v*21)>>6; }   // 0..32 (v*32/100 without a division)
static void hudBar(int x,int y,int v,u16 col){   // a 5 row bar, 34 wide, with a dark well, a coloured fill and a lit top edge
    rect(x,y,34,5,HC_DARK); rect(x+1,y+1,32,3,RGB(3,5,9));
    int f=hudBarPx(v); if(f>0){ rect(x+1,y+1,f,3,col); rect(x+1,y+1,f,1,lite(col,22)); }
}
// ---- top bar ----
static int hudMsg(const char**txt,u16*col,int*pts){   // the message for the middle of the top bar; 0 = none
    *pts=0;
    if(ldead){ *txt="PRESS A TO RESPAWN"; *col=RGB(31,12,8); return 1; }
    if(lnear&&!lcamF){ *txt=simAct?"A OR B GET UP":(lnear==1?"R OPEN FRIDGE":lnear==2?"R USE TOILET":lnear==3?"R SLEEP IN BED":lnear==4?"R TAKE A SHOWER":lnear==6?(pipeOk()?"R PUFF THE PIPE":"GROWN-UPS ONLY"):"R SIT ON SOFA"); *col=HC_GOLD; return 1; }
    if(!lcamF&&!ldead&&lstun<=0){ int m=hhNearest(); if(m>=0){ static char b[24]; char*e=simCat(b,"R TALK TO "); simCat(e,hhM[m].name); if(lnoteT<=0){ *txt=b; *col=HC_GOLD; return 1; } } }   // a household Sim next to you
    if(lnoteT>0){ *txt=lnote; *col=WHITE; *pts=(lpts&&lnote[0]=='N')?lpts:0; return 1; }
    return 0;
}
static void hudTopStatic(void){
    for(int y=0;y<HUD_TOPH-1;y++) rect(0,y,SW,1,hudBgAt(y)); rect(0,HUD_TOPH-1,SW,1,HC_EDGE);
}
static void hudTopUpdate(int all){
    if(all) hudTopStatic();
    if(hudChg(all,HK_SCORE,(unsigned)lscore)){ hudClear(3,0,58,HUD_TOPH-1); numText(text(4,1,"SCORE",HC_LABEL,1)+3,1,lscore,WHITE); hudMark(3,0,58,HUD_TOPH-1); }
    if(hudChg(all,HK_CASH,(unsigned)simMoney)){ hudClear(62,0,40,HUD_TOPH-1);
        rect(63,3,5,5,HC_GOLD); rect(64,4,3,3,RGB(24,19,3)); rect(64,3,3,1,RGB(31,30,16)); numText(71,1,simMoney,HC_GOLD); hudMark(62,0,40,HUD_TOPH-1); }
    // the middle: prompt > note > combo > hint
    const char*t; u16 c; int pts; int has=hudMsg(&t,&c,&pts); char b[24]; int cn=0; int n2=0;
    unsigned mk;
    if(has) mk=(unsigned)(uintptr_t)t*7u+(unsigned)pts;
    else if(lcamF>0) mk=1;
    else if(lcN>0) mk=2u+(unsigned)(lcN*1000+lcPts*lcN)*4u;
    else if(lcBankT>0) mk=3u+(unsigned)lcBank*4u;
    else if(sHud==0) mk=5u+(unsigned)(lskate*2+lhave)*8u;
    else mk=0;
    if(hudChg(all,HK_MSG,mk)){
        hudClear(102,0,78,HUD_TOPH-1); clipSet(102,0,180,HUD_TOPH-1);
        if(!has){
            if(lcamF>0){ t="BIG COMBO"; c=GOLD; has=1; }
            else if(lcN>0){ const char*p="COMBO X"; while(*p) b[cn++]=*p++; int n=lcN; if(n>=10) b[cn++]=(char)('0'+n/10%10); b[cn++]=(char)('0'+n%10); b[cn]=0; t=b; c=GOLD; has=1; n2=lcPts*lcN; }
            else if(lcBankT>0){ t="COMBO"; c=GOLD; has=1; n2=lcBank; }
            else if(sHud==0){ t=lskate?"A PUSH  B OLLIE  L WALK":(lhave?"B RUN  A HOP  L SKATE":"B RUN  A HOP"); c=HC_DIM; has=1; }
        }
        if(has&&t){ int x=text(104,1,t,c,1); if(pts) numText(text(x+3,1,"+",GOLD,1)+1,1,pts,GOLD); else if(n2) numText(x+3,1,n2,WHITE); }
        clipAll(); hudMark(102,0,78,HUD_TOPH-1);
    }
    if(hudChg(all,HK_CLOCK,(unsigned)simMin*8u+(unsigned)simInShift()*4u+(unsigned)xo[XO_CLOCK]+(unsigned)(simIsNight()?64:0)*100000u)){
        hudClear(182,0,56,HUD_TOPH-1);
        if(xo[XO_CLOCK]!=2){
            const char*ck=simsClock(); int w=tw(ck,1); int xx=SW-4-w;
            text(xx,1,ck,simInShift()?HC_GOLD:RGB(22,25,29),1);
            if(simIsNight()){ rect(184,2,5,6,RGB(22,24,31)); rect(185,2,4,6,HC_BG0); rect(184,3,3,4,RGB(22,24,31)); }       // moon
            else { rect(185,3,4,4,RGB(31,27,6)); rect(184,4,6,2,RGB(31,27,6)); }                                           // sun
        }
        hudMark(182,0,56,HUD_TOPH-1);
    }
}
// ---- bottom panel ----
static const u16 hudFaceBg[5]={RGB(6,8,16),RGB(9,9,12),RGB(8,11,16),RGB(9,13,14),RGB(12,12,10)};
static void hudFace3(int x,int y,int st){   // the 7x7 mood face at 3x scale
    static const u16 skin[5]={RGB(14,18,28),RGB(22,22,20),RGB(30,26,8),RGB(26,30,10),RGB(31,20,6)};
    for(int j=0;j<7;j++)for(int i=0;i<7;i++){ char c=faceArt[st][j][i]; if(c=='.') continue; u16 col=c=='k'?RGB(4,3,6):skin[st]; rect(x+i*3,y+j*3,3,3,col); }
}
static u16 hudMoodCol(int st){ return st==MS_SAD?RGB(30,7,6): st==MS_BORED?RGB(29,19,4): st==MS_OK?RGB(18,27,8): st==MS_HAPPY?RGB(8,28,10): RGB(10,31,24); }
static const char* const hudNeedNm[8]={"FOOD","REST","CLEAN","COMFY","WC","FUN","ROOM","SOCIAL"};   // (mood is the face). WC = water closet, the toilet: its bar is the bladder, full = fine
// HEALTH (HP) is the 2 px bar under the face, not one of the eight needs
#define HUD_FX 31
static void hudNeedPos(int i,int*x,int*y){ *x=HUD_FX+(i>>2)*66; *y=HUD_BOTY+4+(i&3)*6; }
static void hudBotStatic(void){
    for(int y=HUD_BOTY;y<SH;y++) rect(0,y,SW,1,hudBgAt(y)); rect(0,HUD_BOTY,SW,1,HC_EDGE); rect(0,HUD_BOTY+1,SW,1,HC_DARK);
    for(int i=0;i<8;i++){ int x,y; hudNeedPos(i,&x,&y); text(x,y,hudNeedNm[i],HC_LABEL,1); }
}
static void hudBotUpdate(int all){
    if(all) hudBotStatic();
    int st=moodState();
    if(hudChg(all,HK_PORT,(unsigned)st)){   // portrait: a frame in the mood colour around the face
        rect(3,HUD_BOTY+4,24,24,hudMoodCol(st)); rect(4,HUD_BOTY+5,22,22,hudFaceBg[st]); hudFace3(5,HUD_BOTY+6,st); hudMark(3,HUD_BOTY+4,24,24); }
    {   // HEALTH: a 2 px bar under the portrait (full width = 100 HP), green / yellow / red like the needs
        int w=(lhp*61)>>8; if(w>24) w=24; unsigned hk=(unsigned)w*4u+(unsigned)(lhp>=55?2:lhp>=28?1:0)+(unsigned)(lhp<=0?1000:0);
        if(hudChg(all,HK_HP,hk)){ rect(3,HUD_BOTY+28,24,2,HC_DARK); if(w>0) rect(3,HUD_BOTY+28,w,2,hudLvlCol(lhp)); hudMark(3,HUD_BOTY+28,24,2); }
    }
    int v[8]={lfood,sNrg,sHyg,sCom,100-lbl,moodFunPct(),sRoom,sSoc};
    for(int i=0;i<8;i++){
        int q=hudBarPx(v[i])*4+(v[i]>=55?2:v[i]>=28?1:0);
        if(hudChg(all,HK_NEED+i,(unsigned)q)){ int x,y; hudNeedPos(i,&x,&y); hudBar(x+28,y+1,v[i],hudLvlCol(v[i])); hudMark(x+28,y+1,34,5); }
    }
    if(sHud>=1) return;   // slim: no aspiration or wants
    int rx=165;
    // header: PERFORMANCE INFO option, else the aspiration and the shift (or the meter's zone)
    int wish=simWishes();
    unsigned hk=sShow?(unsigned)(lfpsV*1000+lloadV*10+sShow)+5000000u:(unsigned)(aspNow()*100000+simZone*10000+wish*5000+(simInShift()?shiftPts+1:0));
    if(!sShow&&simInShift()) hk=hk*31u+(unsigned)simQuota();
    if(hudChg(all,HK_HEAD,hk)){
        hudClear(rx,HUD_BOTY+2,72,8); clipSet(rx,HUD_BOTY+2,rx+72,HUD_BOTY+10);
        if(sShow){ int nx=numText(text(rx,HUD_BOTY+2,"FPS",HC_LABEL,1)+3,HUD_BOTY+2,lfpsV,HC_GOLD)+5; if(sShow==2) numText(text(nx,HUD_BOTY+2,"LOAD",HC_LABEL,1)+3,HUD_BOTY+2,lloadV,lloadV>=100?RGB(30,10,8):HC_GOLD); }
        else if(!wish) text(rx,HUD_BOTY+2,"BABY  NO WANTS YET",HC_DIM,1);
        else {
            int nx=text(rx,HUD_BOTY+2,aspNm[aspNow()],HC_GOLD,1);
            if(simInShift()){ char b[20]; char*e=simCatN(b,shiftPts); *e++='/'; simCatN(e,simQuota());
                int w=tw(b,1); if(rx+72-w>nx+3) text(rx+72-w,HUD_BOTY+2,b,RGB(20,24,28),1); }
            else { const char*z=simZoneNm[simZone]; int w=tw(z,1); if(rx+72-w>nx+3) text(rx+72-w,HUD_BOTY+2,z,simZoneCol(simZone),1); }
        }
        clipAll(); hudMark(rx,HUD_BOTY+2,72,8);
    }
    {   // the aspiration meter
        int w=simMeter*72/1000;
        if(hudChg(all,HK_ASP,(unsigned)w*8u+(unsigned)simZone+(unsigned)wish*4096u)){ hudClear(rx,HUD_BOTY+10,72,3); if(wish) simMeterBar(rx,HUD_BOTY+10,72,3); hudMark(rx,HUD_BOTY+10,72,3); }
    }
    // four wants and three fears as icon cells; the line under them spotlights one at a time, with its points
    int show=xo[XO_WANTS]&&wish, n=0, at[SIM_WS+SIM_FS];
    for(int s=0;s<SIM_WS;s++) if(simW[s]>=0) at[n++]=s;
    for(int s=0;s<SIM_FS;s++) if(simF[s]>=0) at[n++]=SIM_WS+s;
    int spot=n?at[(simT/150)%n]:-1;
    unsigned ck=(unsigned)show;
    for(int s=0;s<SIM_WS;s++) ck=ck*37u+(unsigned)(simW[s]+2);
    for(int s=0;s<SIM_FS;s++) ck=ck*37u+(unsigned)(simF[s]+2);
    ck=ck*17u+(unsigned)simLock*4u+(unsigned)(spot+1)*64u;
    if(hudChg(all,HK_W0,ck)){
        hudClear(rx,HUD_BOTY+14,72,9);
        if(show){
            for(int s=0;s<SIM_WS;s++) simCell(rx+s*10,HUD_BOTY+14,simW[s]>=0?simWants[simW[s]].icon:0,0,simLock>>s&1,simW[s]>=0);
            for(int s=0;s<SIM_FS;s++) simCell(rx+43+s*10,HUD_BOTY+14,simF[s]>=0?simFears[simF[s]].icon:0,1,0,simF[s]>=0);
            if(spot>=0){ int x=spot<SIM_WS?rx+spot*10:rx+43+(spot-SIM_WS)*10; rect(x+1,HUD_BOTY+22,7,1,WHITE); }   // the spotlit cell
        }
        hudMark(rx,HUD_BOTY+14,72,9);
    }
    {
        const char*nm=0; int pts=0, fear=0;
        if(show&&spot>=0){
            if(spot<SIM_WS){ int i=simW[spot]; if(i>=0){ nm=simWantName(spot); pts=simWants[i].pts; } }
            else { int i=simF[spot-SIM_WS]; if(i>=0){ nm=simFears[i].name; pts=simFears[i].pts; fear=1; } } }
        unsigned nk=nm?hudHash(nm)*7u+(unsigned)fear:0;
        if(hudChg(all,HK_W1,nk)){
            hudClear(rx,HUD_BOTY+23,72,7);
            if(nm){ char b[8]; b[0]=fear?'-':'+'; simCatN(b+1,pts); int pw=tw(b,1);
                clipSet(rx,HUD_BOTY+23,rx+70-pw,HUD_BOTY+30); text(rx,HUD_BOTY+23,nm,fear?RGB(30,18,16):RGB(22,28,22),1); clipAll();
                text(rx+72-pw,HUD_BOTY+23,b,fear?RGB(30,10,8):RGB(12,30,12),1); }
            hudMark(rx,HUD_BOTY+23,72,7);
        }
    }
}
// ---- over the head ----
// Returns 1 and the rectangle (x1,y1 excluded) when something is drawn over the player's head. The picture is made in two steps: the room
// rectangle, then this on top, so the rectangle must be redrawn whenever this moves or goes away.
static int hudOverlayWhat(const char**txt,int*alert){   // 0 none, 1 plumbob, 2 bubble
    *txt=0; *alert=0;
    if(lcamF>0||ldead||sHud>=2||!xo[XO_BUBBLE]) return 0;
    if(hhBubT&&hhBubTxt){ *txt=hhBubTxt; return 2; }   // talking (house.h)
    if(hhStill<30) return 0;   // nothing over your head while you move: it only pops up once you stand still for half a second
    const char*t=simsAlert();
    if(t){ *txt=t; *alert=1; return 2; }
    if(xo[XO_BUBBLE]>=2&&simWishes()){ int s0=(simT/240)%SIM_WS; for(int i=0;i<SIM_WS;i++){ int s=(s0+i)%SIM_WS; if(simW[s]>=0){ *txt=simWantName(s); return 2; } } }   // the wants take turns
    return 1;
}
static int hudOverlayRc(int*x0,int*y0,int*x1,int*y1){
    const char*t; int al; int k=hudOverlayWhat(&t,&al); if(!k) return 0;
    int top=plY-plZ-40;
    if(k==2){ int w=tw(t,1)+8, x=plX-w/2; if(x<2) x=2; if(x+w>SW-2) x=SW-2-w; *x0=x-1; *x1=x+w+1; *y0=top-20; *y1=top-2; }
    else { *x0=plX-5; *x1=plX+6; *y0=top-16; *y1=top-1; }
    return 1;
}
static unsigned hudOverlaySig(void){   // what the overlay is made of, apart from where it is (that is part of the rectangle)
    const char*t; int al; int k=hudOverlayWhat(&t,&al); if(!k) return 0;
    return k==2?hudHash(t)*3u+(unsigned)al: 1u+(unsigned)moodState()*8u+(unsigned)(((lfr>>4)&1)*64);
}
static void hudOverlayDraw(void){
    const char*t; int al; int k=hudOverlayWhat(&t,&al); if(!k) return;
    int top=plY-plZ-40;
    if(k==2){
        int w=tw(t,1)+8, x=plX-w/2; if(x<2) x=2; if(x+w>SW-2) x=SW-2-w;
        int y=top-19; u16 edge=al?RGB(30,16,14):RGB(14,26,14), fill=RGB(31,31,31);
        rect(x+1,y,w-2,1,edge); rect(x+1,y+10,w-2,1,edge); rect(x,y+1,1,9,edge); rect(x+w-1,y+1,1,9,edge);     // rounded border
        rect(x+1,y+1,w-2,9,fill);
        text(x+4,y+2,t,RGB(6,6,10),1);
        px(plX-2,y+11,edge); px(plX-1,y+11,edge); px(plX-1,y+12,edge); px(plX,y+12,edge); px(plX,y+13,edge);   // tail
        px(plX-2,y+10,fill); px(plX-1,y+10,fill); px(plX-1,y+11,fill);
    } else {
        int st=moodState(); u16 c=hudMoodCol(st), hi=lite(c,22), lo=shade(c,10);
        int by=top-15+(((lfr>>4)&1)?1:0);     // the plumbob bobs
        static const u8 wd[11]={1,3,5,5,5,5,5,3,3,1,1};
        for(int j=0;j<11;j++){ int hw=wd[j]/2; for(int i=-hw;i<=hw;i++) px(plX+i,by+j,i<0?hi:i>0?lo:c); }
        px(plX-1,by+3,WHITE);
    }
}
