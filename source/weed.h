// weed.h - THE GREEN: grow it, smoke it, sell it, and the DISPENSARY. Grown-ups; teens only with OPTIONS' teen pipe switch (pipeOk), and at three
// times the risk when they sell. The dispensary itself never serves anyone under grown-up.
//   GROW      the GROW PLANTER ('u', BUY > HOME). R next to it: PLANT SEEDS (seeds come from the dispensary), WATER IT, HARVEST. It grows every two game
//             hours while it has water (a sprout, growing, flowering, READY); a dry one may wither. The plant lives in the tile itself: its stage, water and
//             strain are the wallpaper byte of the planter's tile (wallMap: free on any tile that is not a wall), so it is saved with the room, every slot
//             and every floor without a byte of its own. Growing makes you better at it (GROW, up to 100: faster, bigger harvests).
//   STASH     grams of each of 8 strains (SRAM block WD_OFF, and the player save's 'W' chunk). YOUR ACTIONS > FUN > ROLL ONE UP smokes a gram: CHILLED OUT
//             (longer the stronger the strain), the munchies.
//   SELL      the dispensary buys your harvest (less money, perfectly legal). SELL ON THE CORNER (YOUR ACTIONS > FUN, away from home) pays more, and every
//             buyer is a roll: somebody may call it in (npc.h copCrime: the cops come). Teens roll three times as often. Visitors who drop by may buy a bag too.
//   DISPENSARY  a community lot (CT_DISP; a cab takes you, and if the town has none the cab makes one). The counter ('&'): PRE-ROLL, AN EIGHTH, SEEDS,
//             GUMMIES (they kick in later, and hard), BY THE GRAM (any strain), ROLL AND SMOKE HERE, a FREE SAMPLE a day, SELL YOUR HARVEST, ASK THE
//             BUDTENDER. Regulars (100 g sold: 10% off, 300 g: 20% off). The shop has a lounge: couches, a bong, a hookah, snacks from the vending machine.
#define WD_N 8
#define WD_OFF 7440   // SRAM: 'W' 'D' 1, grams (2 bytes each), seeds, grow skill, sold (2), checksum
#define WD_LEN (3+2*WD_N+WD_N+1+2+1)
_Static_assert(HH_OFF+HH_MAXB<=WD_OFF&&WD_OFF+WD_LEN<=LG_OFF,"the weed block sits between the household and the life stats");
typedef struct { const char*nm; u16 bud; u8 pot, yield, price; } WdStrain;   // pot 1..5, grams a harvest gives, simoleons a gram on the street
static const WdStrain wdS[WD_N]={
    {"BORE KUSH",      RGB(10,22,8), 3,10,12}, {"PURPLE HAZE",   RGB(18,9,24),4, 8,16}, {"SOUR DIESEL",  RGB(20,24,6), 4, 9,15}, {"GRANDDADDY BORE",RGB(15,7,20),5,7,20},
    {"NORTHERN LIGHTS",RGB(14,24,20),3,12,11}, {"GELATO",        RGB(24,14,20),4, 9,18}, {"OG BOREDOM",   RGB(11,17,6), 2,14, 9}, {"WEDDING CAKE",  RGB(26,24,22),5, 8,22} };
static u16 wdG[WD_N]; static u8 wdSeed[WD_N], wdSk; static u16 wdSold; static short wdEdT;   // stash, seed packs, GROW skill, grams sold, steps until the gummy kicks in
enum { WP_EMPTY, WP_SPROUT, WP_GROW, WP_FLOWER, WP_READY, WP_DEAD };
static int wdOk(void){ return pipeOk(); }   // (sims.h: grown-ups, or teens with the teen pipe option)
static int wdAll(void){ int n=0; for(int i=0;i<WD_N;i++) n+=wdG[i]; return n; }
static int wdBest(void){ int b=-1; for(int i=0;i<WD_N;i++) if(wdG[i]&&(b<0||wdS[i].pot>wdS[b].pot)) b=i; return b; }
static void wdSave(void){ volatile u8*m=SRAM_BASE+WD_OFF; int k=3; m[0]='W'; m[1]='D'; m[2]=1;
    for(int i=0;i<WD_N;i++){ m[k++]=(u8)wdG[i]; m[k++]=(u8)(wdG[i]>>8); } for(int i=0;i<WD_N;i++) m[k++]=wdSeed[i]; m[k++]=wdSk; m[k++]=(u8)wdSold; m[k++]=(u8)(wdSold>>8);
    u8 s=0x57; for(int i=2;i<k;i++) s=(u8)(s*3+m[i]); m[k]=s; }
static void wdClear(void){ for(int i=0;i<WD_N;i++){ wdG[i]=0; wdSeed[i]=0; } wdSk=0; wdSold=0; wdEdT=0; }
static int wdLoadFrom(volatile const u8*m){   // 1 = a sound block
    if(m[0]!='W'||m[1]!='D'||m[2]!=1) return 0;
    u8 s=0x57; for(int i=2;i<WD_LEN-1;i++) s=(u8)(s*3+m[i]); if(m[WD_LEN-1]!=s) return 0;
    int k=3; for(int i=0;i<WD_N;i++){ wdG[i]=(u16)(m[k]|m[k+1]<<8); k+=2; } for(int i=0;i<WD_N;i++) wdSeed[i]=m[k++]; wdSk=m[k++]; if(wdSk>100) wdSk=100; wdSold=(u16)(m[k]|m[k+1]<<8);
    return 1; }
static void wdLoad(void){ wdClear(); wdLoadFrom(SRAM_BASE+WD_OFF); }
// ---- the planter: its state is the tile's wallMap byte (bits 0-2 stage, 3-4 water 0..3, 5-7 strain) ----
static int wpSt(int x,int y){ return wallMap[y][x]&7; }
static int wpWa(int x,int y){ return (wallMap[y][x]>>3)&3; }
static int wpSn(int x,int y){ return wallMap[y][x]>>5; }
static void wpSet(int x,int y,int st,int wa,int sn){ wallMap[y][x]=(u8)((st&7)|((wa&3)<<3)|((sn&7)<<5)); }
static void wdGrowTick(void){   // every two game hours (simMinute): every planter on the live floor drinks, and may grow a stage
    int ch=0;
    for(int y=0;y<MH;y++) for(int x=0;x<MW;x++){ if(lifeMap[y][x]!='u') continue; int st=wpSt(x,y), wa=wpWa(x,y), sn=wpSn(x,y), was=wallMap[y][x];
        if(st<WP_SPROUT||st>WP_FLOWER) continue;
        if(wa){ wa--; if(((int)rnd8()*100>>8)<45+wdSk/3) st++; }
        else if(((int)rnd8()*100>>8)<30) st=WP_DEAD;
        wpSet(x,y,st,wa,sn); if(wallMap[y][x]!=was) ch=1; }
    if(ch) liveInvalidate();   // (only when a planter changed: no redraw for nothing)
}
static void wdFan(int cx,int cy,int big,u16 c,u16 d){   // a cannabis fan leaf: fingers spread from the stem (big: seven, small: five)
    if(big){ px(cx-3,cy-3,c); px(cx,cy-4,c); px(cx+3,cy-3,c); px(cx-2,cy-2,c); px(cx,cy-3,c); px(cx+2,cy-2,c); px(cx-1,cy-1,c); px(cx,cy-2,c); px(cx+1,cy-1,c);
        px(cx-3,cy,c); px(cx+3,cy,c); px(cx-2,cy,d); px(cx+2,cy,d); px(cx-1,cy,c); px(cx+1,cy,c); px(cx,cy-1,d); px(cx,cy,d); px(cx,cy+1,d); }
    else { px(cx-2,cy-2,c); px(cx,cy-3,c); px(cx+2,cy-2,c); px(cx-1,cy-1,c); px(cx,cy-2,c); px(cx+1,cy-1,c); px(cx-2,cy,c); px(cx+2,cy,c); px(cx-1,cy,c); px(cx+1,cy,c); px(cx,cy-1,d); px(cx,cy,d); }
}
static void drawPlanterS(int sx,int sy,int st,int wa,int sn,int t){   // a planter (drawn by code): a clay pot, the plant by its stage, the buds in the strain's colour
    rect(sx-4,sy-6,9,5,RGB(22,10,5)); rect(sx-5,sy-7,11,2,RGB(26,14,7)); rect(sx-3,sy-1,7,1,RGB(14,6,3)); rect(sx+2,sy-6,3,5,RGB(17,7,4));   // the pot
    rect(sx-4,sy-7,9,1,wa?RGB(9,6,3):RGB(17,13,8));   // the soil: dark when watered
    u16 lf=RGB(6,20,6), ld=RGB(4,13,4), bud=wdS[sn].bud;
    if(st==WP_DEAD){ rect(sx-1,sy-12,1,5,RGB(14,11,5)); rect(sx-4,sy-10,3,1,RGB(16,12,5)); rect(sx+1,sy-11,3,1,RGB(15,11,4)); return; }
    if(st==WP_SPROUT){ rect(sx,sy-10,1,3,lf); rect(sx-2,sy-11,2,1,lf); rect(sx+1,sy-11,2,1,lf); }
    else if(st>=WP_GROW){ int h=st==WP_GROW?14:st==WP_FLOWER?20:24, top=sy-7-h;   // a stalk with fan leaves in tiers, left and right in turn
        rect(sx,top,1,h,ld);
        for(int k=0,y=sy-10;y>top+2;k++,y-=5){ int side=(k&1)?1:-1, big=y>top+8; wdFan(sx+side*(big?4:3),y,big,(k&1)?lf:RGB(5,17,5),ld); }
        wdFan(sx,top+2,0,lf,ld);   // the crown
        if(st>=WP_FLOWER){ u16 bc=st==WP_READY?bud:RGB(26,28,24); int n=0;   // white hairs, then buds in the strain's colour up the stalk; a fat one on top
            for(int y=sy-12;y>top+3;y-=5,n++){ rect(sx-1,y,3,2,bc); if(st==WP_READY) px(sx+(n&1?1:-1),y-1,bc); }
            rect(sx-1,top-1,3,4,bc); px(sx,top-2,bc);
            if(st==WP_READY){ px(sx,top,RGB(31,30,24)); if((t>>2)&1) px(sx+3,top+1,RGB(31,31,20)); else px(sx-3,top+3,RGB(31,31,20)); } } }   // READY sparkles
    if(st>=WP_SPROUT&&st<=WP_FLOWER&&!wa&&((t>>3)&1)){ rect(sx+5,sy-14,3,4,RGB(8,18,31)); px(sx+6,sy-15,RGB(8,18,31)); }   // a blinking drop: thirsty
}
static void drawPlanter(int sx,int sy,int x,int y,int t){ drawPlanterS(sx,sy,wpSt(x,y),wpWa(x,y),wpSn(x,y),t); }   // the planter on tile x, y (its state is the tile's wallMap byte)
static int wdNear(char c,int*ox,int*oy){ int tx=(int)(lfx>>8), ty=(int)(lfy>>8);
    for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){ int x=tx+dx, y=ty+dy; if(x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]==c){ if(ox){ *ox=x; *oy=y; } return 1; } }
    return 0; }
static char wdMsg[48] EWRAM_BSS;
static int wdPickStrain(const char*title,int seeds){   // a strain you have seeds (or grams) of; -1 none / cancelled
    const char* it[WD_N]; char lb[WD_N][28]; int id[WD_N], n=0;
    for(int i=0;i<WD_N;i++){ int have=seeds?wdSeed[i]:wdG[i]; if(!have) continue; char*e=simCat(lb[n],wdS[i].nm); e=simCat(e,"  "); e=simCatN(e,have); simCat(e,seeds?" PACKS":" G"); it[n]=lb[n]; id[n++]=i; }
    if(!n) return -1;
    int c=menu(title,it,n); return c<0?-1:id[c];
}
static void wdPlanter(int x,int y){   // R next to a planter
    if(!wdOk()){ lnote=stage==AG_TEEN?"NOT OLD ENOUGH YET":"GROWN-UPS ONLY"; lnoteT=60; return; }
    int st=wpSt(x,y), wa=wpWa(x,y), sn=wpSn(x,y);
    static const char* const stNm[6]={"AN EMPTY POT","A SPROUT","GROWING","FLOWERING","READY TO HARVEST","WITHERED"};
    char tl[40]; { char*e=simCat(tl,st?wdS[sn].nm:"GROW PLANTER"); e=simCat(e,"  GROW "); simCatN(e,wdSk); }
    const char* it[4]; int id[4], n=0;
    if(st==WP_EMPTY){ it[n]="PLANT SEEDS"; id[n++]=0; }
    if(st>=WP_SPROUT&&st<=WP_FLOWER){ if(wa<3){ it[n]="WATER IT"; id[n++]=1; } it[n]="CHECK ON IT"; id[n++]=2; it[n]="TALK TO IT"; id[n++]=3; }
    if(st==WP_READY){ it[n]="HARVEST"; id[n++]=4; }
    if(st==WP_DEAD){ it[n]="CLEAR THE POT"; id[n++]=5; }
    int c=menu(tl,it,n); liveInvalidate(); camSnap=1; if(c<0) return;
    lstun=50; lsp=0; lgrind=0;
    switch(id[c]){
        case 0: { int s=wdPickStrain("PLANT WHICH SEEDS",1); if(s<0){ toast("NO SEEDS  THE DISPENSARY SELLS THEM"); lstun=0; return; } wdSeed[s]--; wpSet(x,y,WP_SPROUT,3,s); wdSk=(u8)(wdSk<100?wdSk+1:100); lnote="PLANTED  WATER IT NOW AND THEN"; lnoteT=120; wdSave(); break; }
        case 1: wpSet(x,y,st,3,sn); lnote="WATERED"; lnoteT=50; sfxPlay(SFX_SPLASH); break;
        case 2: { char*e=simCat(wdMsg,stNm[st]); simCat(e,wa?"  NOT THIRSTY":"  NEEDS WATER"); lnote=wdMsg; lnoteT=100; break; }
        case 3: { static const char* const tk[4]={"YOU TELL IT ABOUT YOUR DAY","IT SEEMS TO LIKE JAZZ","YOU CALL IT A GOOD GIRL","IT GROWS A LITTLE PROUDER"}; lnote=tk[rnd8()&3]; lnoteT=110; moodEvent(M_CHILL); if(((int)rnd8()*100>>8)<20&&st<WP_READY) wpSet(x,y,st+1,wa,sn); break; }
        case 4: { int g=wdS[sn].yield+wdSk/12+(rnd8()&3); wdG[sn]=(u16)(wdG[sn]+g>9999?9999:wdG[sn]+g); if(((int)rnd8()*100>>8)<25) wdSeed[sn]++;
            wpSet(x,y,WP_EMPTY,0,0); wdSk=(u8)(wdSk+3>100?100:wdSk+3); moodEvent(M_PAY); voxPlay(V_yahoo);
            char*e=simCat(wdMsg,"HARVESTED "); e=simCatN(e,g); e=simCat(e,"G OF "); simCat(e,wdS[sn].nm); lnote=wdMsg; lnoteT=150; wdSave(); stQEvent(QE_HARVEST,g); break; }
        case 5: wpSet(x,y,WP_EMPTY,0,0); lnote="CLEARED  TRY AGAIN"; lnoteT=60; break; }
}
// ---- smoking and selling (YOUR ACTIONS > FUN, self.h) ----
static void wdRoll(void){
    if(!wdOk()){ lnote="GROWN-UPS ONLY"; lnoteT=50; return; }
    int s=wdBest(); if(s<0){ lnote="YOUR STASH IS EMPTY"; lnoteT=60; return; }
    wdG[s]--; lchill=1200+wdS[s].pot*240; lstun=110; lsp=0; lgrind=0; moodEventN(M_CHILL,1+wdS[s].pot/2); voxChain(V_lighter_spark,V_spark_inhale,(rnd8()&1)?V_after_smoke_cough:V_after_smoke_cough_2);   // spark, inhale, cough
    char*e=simCat(wdMsg,"ROLLED A FAT ONE  "); simCat(e,wdS[s].nm); lnote=wdMsg; lnoteT=150; wdSave();
}
static int wdRisk(int base){ int r=base*(stage==AG_TEEN?3:1); return ((int)rnd8()*100>>8)<r; }   // a roll for being seen: teens three times as likely
static void wdCorner(void){   // SELL ON THE CORNER: a few buyers come by, each one a chance somebody calls it in
    if(!wdOk()){ lnote="GROWN-UPS ONLY"; lnoteT=50; return; }
    if(!nbOk||nbT.cur==nbT.home){ lnote="NOT ON YOUR OWN LOT"; lnoteT=60; return; }
    if(prHere()){ lnote="NOT IN HERE"; lnoteT=60; return; }
    if(wdAll()<1){ lnote="NOTHING TO SELL"; lnoteT=50; return; }
    int buyers=1+(rnd8()&1)+skLvl(SK_CHARM)/4, cash=0, sold=0, seen=0;
    for(int b=0;b<buyers;b++){ int s=wdBest(); if(s<0) break; int g=1+(rnd8()&3); if(g>wdG[s]) g=wdG[s];
        wdG[s]=(u16)(wdG[s]-g); sold+=g; cash+=g*wdS[s].price*3/2; if(wdRisk(12)) seen=1; }
    simMoneyAdd(cash); wdSold=(u16)(wdSold+sold); lstun=240; lsp=0; lgrind=0; sfxPlay(SFX_CASH); wdSave(); if(sold) stQEvent(QE_SOLD,sold);
    char*e=simCat(wdMsg,"SOLD "); e=simCatN(e,sold); e=simCat(e,"G FOR \xC2\xA7"); simCatN(e,cash); lnote=wdMsg; lnoteT=150;
    if(seen){ copCrime(stage==AG_TEEN?4:2); simQPush("SOMEBODY SNITCHED"); voxPlay(V_shriek); }   // (npc.h: the heat; enough of it and the cops come)
}
static int wdCanSell(int stg){ return wdOk()&&wdAll()>0&&stg>=AG_TEEN&&!prIn(); }   // (house.h: a neighbour who dropped by)
static void wdSellTo(const char*nm,int stg){   // SELL A BAG: they take a few grams of your best, at a street price; somebody may see (a teen seller, or a teen buyer, ups the odds)
    int s=wdBest(); if(s<0) return;
    if(stg==AG_TEEN&&stage>=AG_ADULT&&(rnd8()&1)){ char*e=simCat(wdMsg,nm); simCat(e," IS TOO YOUNG  YOU WALK AWAY"); lnote=wdMsg; lnoteT=150; voxPlay(V_nah); return; }   // a grown-up thinks twice
    int g=1+(rnd8()%3); if(g>wdG[s]) g=wdG[s]; int cash=g*wdS[s].price*5/4;
    wdG[s]=(u16)(wdG[s]-g); wdSold=(u16)(wdSold+g); simMoneyAdd(cash); sfxPlay(SFX_CASH); lstun=50; lsp=0; wdSave(); stQEvent(QE_SOLD,g);
    char*e=simCat(wdMsg,nm); e=simCat(e," BOUGHT "); e=simCatN(e,g); e=simCat(e,"G  \xC2\xA7"); simCatN(e,cash); lnote=wdMsg; lnoteT=150;
    if(wdRisk(stg==AG_TEEN?10:5)){ copCrime(stage==AG_TEEN?4:2); simQPush("A NEIGHBOR SAW THE DEAL"); voxPlay(V_shriek); }
}
// ---- THE DISPENSARY: a community lot with a counter ('&'), a back wall of jars and shelves, and a smoking lounge ----
static int catFind(const char*nm);   // venues.h
static int dpX0,dpY0,dpX1,dpY1,dpCx;   // the room being laid out (dispBuild)
static void dpPut(int x,int y,char c,const char*look){   // a prop on free floor inside the room (never in the way of the door)
    if(x<=dpX0||x>=dpX1||y<=dpY0||y>=dpY1||lifeMap[y][x]!='.'||(x==dpCx&&y>=dpY1-1)) return;
    gPut(x,y,c); if(look) wallMap[y][x]=(u8)catFind(look);
}
static void dispBuild(int x0,int y0,int x1,int y1){   // nbTemplate: a green shop: the counter across it, jars and shelves behind, a lounge with couches and pipes
    int cx=(x0+x1)/2, rx0=x0+1, ry0=y0+1, rx1=x1-1, ry1=y1-3, hw=(rx1-rx0>=12)?3:2, ly=ry0+5;
    dpX0=rx0; dpY0=ry0; dpX1=rx1; dpY1=ry1; dpCx=cx;
    gBox(x0,y0,x1,y1,4); gRoom(rx0,ry0,rx1,ry1,11,11); gPut(cx,ry1,'D');
    for(int x=cx-hw;x<=cx+hw;x++) gPut(x,ry0+3,'&');
    for(int d=-3;d<=3;d++){ int x=cx+d;   // the back wall: shelves of jars, lava lamps, the neon sign over the middle
        if(d==0) dpPut(x,ry0+1,'l',"NEON BORE SIGN");
        else if(d==-2||d==2) dpPut(x,ry0+1,'l',d<0?"GREEN LAVA LAMP":"PINK LAVA LAMP");
        else dpPut(x,ry0+1,'b',(d&1)?"TALL BOOKSHELF":"WHITE BOOKSHELF"); }
    dpPut(rx0+1,ry0+1,'p',"POTTED PALM"); dpPut(rx1-1,ry0+1,'p',"FICUS");
    dpPut(rx0+1,ry0+2,'V',0); dpPut(rx1-1,ry0+2,'R',0);   // the DeadSet VYBE and a radio by the counter
    dpPut(rx1-1,ry0+4,'F',"VENDING MACHINE");   // snacks: you will want them
    if(ry1-2>=ly+2){   // the smoking lounge: couches, a bong table and a hookah, beanbags, stools
        dpPut(rx0+1,ly,'C',"VELVET SOFA"); dpPut(rx0+1,ly+1,'C',"VELVET SOFA"); dpPut(rx0+3,ly,'G',"BONG"); dpPut(rx0+3,ly+2,'U',0); dpPut(rx0+1,ly+3,'l',"PINK LAVA LAMP");
        dpPut(rx1-1,ly+1,'C',"GREY COUCH"); dpPut(rx1-1,ly+2,'C',"GREY COUCH"); dpPut(rx1-3,ly+1,'G',"HOOKAH"); dpPut(rx1-3,ly+3,'U',0); dpPut(rx1-1,ly+3,'l',"GREEN LAVA LAMP");
        dpPut(cx-2,ly+2,'h',"BAR STOOL"); dpPut(cx+2,ly+2,'h',"BAR STOOL"); }
    if(ry1-2>ry0+4){ gPut(rx0+1,ry1-1,'U'); gPut(rx0+2,ry1-1,'U'); gPut(rx1-1,ry1-1,'u'); gPut(rx1-2,ry1-1,'r'); wpSet(rx1-1,ry1-1,WP_READY,3,1); }   // a show plant by the door (PURPLE HAZE, ready, behind glass)
}
static void drawDispCounter(int sx,int sy,int t){   // the counter: a glass case of jars (the day's strains) on a pale wood base, a green cross light
    rect(sx-8,sy-9,17,8,RGB(20,16,10)); rect(sx-8,sy-2,17,1,RGB(12,9,5)); rect(sx-8,sy-6,17,1,RGB(15,11,6));
    rect(sx-8,sy-11,17,3,RGB(18,26,28)); rect(sx-8,sy-11,17,1,RGB(28,31,31));   // the glass top
    for(int k=0;k<4;k++){ int jx=sx-7+k*4; rect(jx,sy-15,3,4,RGB(24,28,26)); rect(jx,sy-14,3,2,wdS[(simDay+k*2)%WD_N].bud); rect(jx,sy-16,3,1,RGB(9,11,11)); }   // jars of flower
    u16 g=((t>>3)&1)?RGB(6,30,8):RGB(4,18,6); rect(sx,sy-24,1,5,g); rect(sx-2,sy-22,5,1,g);   // the cross, blinking
}
static short wdSamp=-1;   // the game day you had the free sample (not saved: tomorrow there is another)
static int dpPrice(int p){ int d=wdSold>=300?20:wdSold>=100?10:0; return p-p*d/100; }   // regulars (100 g sold here: 10% off, 300 g: 20% off)
static void wdSmokeUp(int s,int g,int fancy){   // g grams rolled and lit; your own roll (fancy) can come out sloppy, and every one makes you better at it
    int sloppy=fancy&&(((int)rnd8()*100>>8)<26-wdSk/5);
    int ch=(1200+wdS[s].pot*240)*(g==1?10:g==2?17:23)/10; if(sloppy) ch=ch*2/3;
    lchill=ch; lstun=110+g*30; lsp=0; lgrind=0; moodEventN(M_CHILL,1+wdS[s].pot/2+(g>1));
    voxChain(V_lighter_spark,V_spark_inhale,(rnd8()&1)?V_after_smoke_cough:V_after_smoke_cough_2);
    if(fancy) wdSk=(u8)(wdSk<100?wdSk+1:100);
    char*e=simCat(wdMsg,sloppy?"SLOPPY ROLL  BURNS UNEVEN  ":g==1?"LIT A JOINT  ":g==2?"LIT A FAT ONE  ":"LIT A BLUNT  "); simCat(e,wdS[s].nm); lnote=wdMsg; lnoteT=150;
}
static int dpBuy(int s,int g,int cost){   // grams off the shelf into the stash
    if(simMoney<cost){ toast("NOT ENOUGH SIMOLEONS"); return 0; }
    simMoney-=cost; wdG[s]=(u16)(wdG[s]+g>9999?9999:wdG[s]+g); sfxPlay(SFX_CASH); wdSave(); simsSave();
    char*e=simCat(wdMsg,"BOUGHT "); e=simCatN(e,g); e=simCat(e,"G OF "); simCat(e,wdS[s].nm); toast(wdMsg); return 1;
}
static void dpJars(void){   // BY THE GRAM: any strain, 1 / 4 / 8 grams (the more, the cheaper)
    char sl[WD_N][34]; const char* si[WD_N];
    for(;;){
        for(int i=0;i<WD_N;i++){ char*q=simCat(sl[i],wdS[i].nm); q=simCat(q,"  POT "); q=simCatN(q,wdS[i].pot); q=simCat(q,"  \xC2\xA7"); simCatN(q,dpPrice(wdS[i].price+3)); si[i]=sl[i]; }
        int s=menu("THE JARS  A GRAM",si,WD_N); if(s<0) return;
        static const char* const am[3]={"1 GRAM","4 GRAMS  10% OFF","8 GRAMS  20% OFF"}; static const u8 ng[3]={1,4,8}, pc[3]={100,90,80};
        int a=menu(wdS[s].nm,am,3); if(a<0) continue;
        dpBuy(s,ng[a],dpPrice(wdS[s].price+3)*ng[a]*pc[a]/100);
    }
}
static int lnrHere(void);   // lunrado.h: the moonbase's dispensary is LUNARWEED
static void wdCounter(void){   // R at the dispensary counter
    if(stage<AG_ADULT){ static const char* const no[3]={"21 AND UP  NICE TRY KID","THEY CHECKED YOUR ID  NOPE","COME BACK WITH A BEARD"}; lnote=no[rnd8()%3]; lnoteT=110; voxPlay(V_disagree); return; }
    static const char* const tips[8]={"START LOW AND GO SLOW","GUMMIES TAKE A WHILE  TRUST ME","WATER YOUR PLANTS  NOT TOO MUCH","PURPLE HAZE  GOOD FOR THE SOUL","SNACKS FIRST  YOU WILL THANK ME","THE CORNER IS RISKY  JUST SAYING","ROLL ONE UP HERE  THE COUCHES ARE FREE","THE FREE SAMPLE IS ONCE A DAY"};
    char tt[40]; const char* it[10]; char lb[5][34]; int id[10];
    for(;;){
        int sd=(simDay+3)%WD_N, n=0, c;
        char*e=simCat(tt,lnrHere()?"LUNARWEED  \xC2\xA7":"GREEN BORE  \xC2\xA7"); simCatMoney(e,simMoney,1);
        { char*q=simCat(lb[0],"PRE-ROLL  \xC2\xA7"); simCatN(q,dpPrice(15)); it[n]=lb[0]; id[n++]=0; }
        { char*q=simCat(lb[1],"1/8TH "); q=simCat(q,wdS[sd].nm); q=simCat(q,"  \xC2\xA7"); simCatN(q,dpPrice(45)); it[n]=lb[1]; id[n++]=1; }   // (the strain of the day)
        it[n]="BY THE GRAM"; id[n++]=2;
        it[n]="ROLL AND SMOKE HERE"; id[n++]=3;
        if(wdSamp!=simDay){ it[n]="FREE SAMPLE"; id[n++]=4; }
        { char*q=simCat(lb[2],"SEEDS  \xC2\xA7"); q=simCatN(q,dpPrice(60)); simCat(q," A PACK"); it[n]=lb[2]; id[n++]=5; }
        { char*q=simCat(lb[3],"GUMMIES  \xC2\xA7"); simCatN(q,dpPrice(25)); it[n]=lb[3]; id[n++]=6; }
        { char*q=simCat(lb[4],"SELL YOUR HARVEST  "); q=simCatN(q,wdAll()); simCat(q,"G"); it[n]=lb[4]; id[n++]=7; }
        it[n]="ASK THE BUDTENDER"; id[n++]=8; it[n]="LEAVE"; id[n++]=9;
        c=menu(tt,it,n); if(c<0||id[c]==9) break; c=id[c];
        if(c==8){ if(rnd8()%3==0){ if(wdSold>=300) toast("YOU ARE A REGULAR  20% OFF"); else if(wdSold>=100) toast("YOU ARE A REGULAR  10% OFF"); else toast("SELL US 100G AND YOU ARE A REGULAR"); } else toast(tips[rnd8()&7]); continue; }
        if(c==7){ int cash=0, g=0; for(int i=0;i<WD_N;i++){ cash+=wdG[i]*wdS[i].price*3/5; g+=wdG[i]; wdG[i]=0; } if(!g){ toast("NOTHING TO SELL"); continue; } simMoneyAdd(cash); wdSold=(u16)(wdSold+g); wdSave(); sfxPlay(SFX_CASH); stQEvent(QE_SOLD,g); char*q=simCat(wdMsg,"SOLD "); q=simCatN(q,g); q=simCat(q,"G  \xC2\xA7"); simCatN(q,cash); toast(wdMsg); continue; }
        if(c==2){ dpJars(); continue; }
        if(c==3){   // ROLL AND SMOKE HERE: pick the strain from your stash, then the size; it goes straight in the lounge
            int s=wdPickStrain("ROLL WITH WHICH",0); if(s<0){ if(!wdAll()) toast("NOTHING TO ROLL  TRY THE JARS"); continue; }
            static const char* const sz[3]={"JOINT  1 G","FAT ONE  2 G","BLUNT  3 G"}; int z=menu("HOW BIG",sz,3); if(z<0) continue;
            if(wdG[s]<z+1){ toast("NOT ENOUGH OF THAT"); continue; }
            wdG[s]=(u16)(wdG[s]-(z+1)); wdSmokeUp(s,z+1,1); wdSave(); simsSave(); break; }
        if(c==4){ wdSamp=(short)simDay; lchill=500; lstun=70; lsp=0; lgrind=0; moodEventN(M_CHILL,1); voxChain(V_lighter_spark,V_spark_inhale,V_after_smoke_cough);
            char*q=simCat(wdMsg,"A FREE TOKE OF "); simCat(q,wdS[sd].nm); lnote=wdMsg; lnoteT=140; break; }
        if(c==5){ char sl[WD_N][28]; const char* si[WD_N]; for(int i=0;i<WD_N;i++){ char*q=simCat(sl[i],wdS[i].nm); q=simCat(q,"  HAVE "); simCatN(q,wdSeed[i]); si[i]=sl[i]; }
            int s=menu("SEEDS  A PACK",si,WD_N); if(s<0) continue; if(simMoney<dpPrice(60)){ toast("NOT ENOUGH SIMOLEONS"); continue; } simMoney-=dpPrice(60); wdSeed[s]=(u8)(wdSeed[s]<99?wdSeed[s]+1:99); wdSave(); sfxPlay(SFX_CASH); toast("A PACK OF SEEDS  PLANT IT AT HOME"); continue; }
        int price=c==0?dpPrice(15):c==1?dpPrice(45):dpPrice(25); if(simMoney<price){ toast("NOT ENOUGH SIMOLEONS"); continue; } simMoney-=price; sfxPlay(SFX_CASH);
        if(c==0){ wdG[0]++; toast("A PRE-ROLL OF BORE KUSH"); }
        else if(c==1){ wdG[sd]+=4; toast("AN EIGHTH  PLUS A LITTLE EXTRA"); }
        else { wdEdT=600; toast("YOU EAT ONE  NOTHING HAPPENS  YET"); }
        wdSave(); simsSave();
    }
    liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
}
static int wdUse(void){   // R: a planter or the counter within reach. 1 = handled
    int x,y;
    if(wdNear('&',0,0)){ wdCounter(); return 1; }
    if(wdNear('u',&x,&y)){ wdPlanter(x,y); return 1; }
    return 0;
}
static const char* wdHint(void){ if(wdNear('&',0,0)) return "R THE COUNTER"; if(wdNear('u',0,0)) return "R TEND THE PLANT"; return 0; }   // hud.h
static void wdTick(void){   // every logic step: the gummy
    if(wdEdT>0&&--wdEdT==0){ lchill=3600; moodEventN(M_CHILL,3); lstun=60; lsp=0; lnote="THE GUMMY KICKED IN  OH NO"; lnoteT=150; voxChain(V_laughing,V_laughing_2,V_im_hungryrururyry); }
}
static void wdHour(void){ if(simMin%120==0) wdGrowTick(); }   // simMinute
