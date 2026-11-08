// kingamb.h - EP6 THE LAND BOREFORE TIME: the SEASONS and the AMBIENT LIFE over the map. PURE C (needs kingdom.h, kingsim.h, kingview.h and kingfx.h before it; no hardware).
// The year is 32 realm days, eight to a season (spring, summer, autumn, winter; a new realm opens in spring). kaSeason says which it is and how far into the turn to the next one.
// The colours of the land (BG palette bank 0: the grass, the meadow, the trees, the roads) follow it (kaSeasonC): spring a little fresher, summer as drawn, autumn the greens turned to gold
// and rust, winter dusted white; the day's light is put on top of that (kvTintC, in kingplay.h).
// The weather and the wildlife are sprites in screen space, made from the frame counter alone (nothing is kept, nothing is saved): snow in winter, falling leaves in autumn, butterflies by
// day in spring and summer, a flock of birds crossing now and then, fireflies on the warm nights, bats at dusk. They are drawn in front of everything (priority 0) but not over the text strip.
// OBJ tiles KA_AMB.. (the art is made at start by kaAmbArt: nine 8 x 8 pictures, drawn here as text), OBJ palette bank 9.
#ifndef KINGAMB_H
#define KINGAMB_H
#define KA_AMB 372           // first OBJ tile (after the crowds, 316..371)
#define KA_AMB_N 15
#define KA_AMBPAL 9          // OBJ palette bank
enum { KS_SPRING, KS_SUMMER, KS_AUTUMN, KS_WINTER };
static const char* const kaSeasonNm[4]={ "SPRING", "SUMMER", "AUTUMN", "WINTER" };
static const char* const kaSeasonMsg[4]={ "SPRING  THE FIELDS WAKE UP", "SUMMER  LONG WARM DAYS", "AUTUMN  THE LEAVES TURN", "WINTER  SNOW ON THE FIELDS  THE WORK SLOWS" };
#define KA_SEASON_DAYS 8     // realm days to a season
#define KA_TURN (2*KS_DAYT)  // the colours turn over the last two days of a season
// the season, the next one, and how far (0..16) the colours have turned towards it
static inline void kaSeason(const KSim*S,int*sn,int*nx,int*k){
    u32 d=S->R->day; *sn=(int)((d/KA_SEASON_DAYS)&3u); *nx=(*sn+1)&3;
    int p=(int)(d%KA_SEASON_DAYS)*KS_DAYT+(int)(S->t%KS_DAYT), from=KA_SEASON_DAYS*KS_DAYT-KA_TURN;
    *k=p>from?(p-from)*16/KA_TURN:0;
}
static inline int kaClamp5(int v){ return v<0?0:v>31?31:v; }
// one colour of the land in a season
static inline u16 kaSeasonC(u16 c,int sn){
    int r=c&31, g=(c>>5)&31, b=(c>>10)&31, fol=g>r+2&&g>=b+4;   // (foliage: the greens; the browns, the flowers and the dark stay what they are)
    if(sn==KS_SPRING){ if(fol){ g+=2; r+=1; } }
    else if(sn==KS_AUTUMN){ if(fol){ int nr=g+3, ng=(g*12)>>4, nb=(b*4)>>4; r=(r*4+nr*12)>>4; g=(g*4+ng*12)>>4; b=(b*4+nb*12)>>4; } }   // (mostly turned: the fields go gold and the trees rust, not desert)
    else if(sn==KS_WINTER){ int kk=fol?12:6; r=(r*(16-kk)+25*kk)>>4; g=(g*(16-kk)+27*kk)>>4; b=(b*(16-kk)+30*kk)>>4; }
    return (u16)(kaClamp5(r)|(kaClamp5(g)<<5)|(kaClamp5(b)<<10));
}
static inline u16 kaSeasonMix(u16 c,int sn,int nx,int k){   // the colour in season sn, k/16 of the way to season nx
    u16 a=kaSeasonC(c,sn); if(k<=0) return a; u16 b=kaSeasonC(c,nx);
    int r=((a&31)*(16-k)+(b&31)*k)>>4, g=(((a>>5)&31)*(16-k)+((b>>5)&31)*k)>>4, bl=(((a>>10)&31)*(16-k)+((b>>10)&31)*k)>>4;
    return (u16)(r|(g<<5)|(bl<<10)); }
// ---- the art: hex digits are palette entries (0 clear). 11 tiles: flake, big flake, leaf, bird up, bird down, bat up, bat down, firefly, butterfly open, butterfly shut, petal ----
static const char* const kaAmbArt[KA_AMB_N][8]={
 {"........","........","..11....","..111...","...1....","........","........","........"},
 {"........","...2....","..212...","...2....","........","........","........","........"},
 {"........","..3.....",".343....","..434...","...43...","....3...","........","........"},
 {"........","........","5.....5.",".5...5..","..555...","........","........","........"},
 {"........","........","........","55...55.",".55.55..","..555...","........","........"},
 {"........","........","6.....6.","66.6.66.",".66666..","........","........","........"},
 {"........","........","........","66.6.66.",".66666..","..6.6...","........","........"},
 {"........","........","...8....","..878...","...8....","........","........","........"},
 {"........","........","a.....b.","aa.5.bb.","aaa5bbb.","aa.5.bb.","........","........"},
 {"........","........","........","...5....","..a5b...","...5....","........","........"},
 {"........","........","........","...c....","..ccc...","...c....","........","........"},
 {"........","........","....9999","...99999","..999999","..999999","..999999",".ddddddd"},      // the caravan's wagon, 16 x 16: top left, top right, bottom left, bottom right
 {"........","........","9999....","99999...","999999..","999999..","999999..","ddddddd."},
 {".ddddddd",".deddddd",".ddddddd","..eeee..",".eeffee.",".eeffee.","..eeee..","........"},
 {"ddddddd.","ddddded.","ddddddd.","..eeee..",".eeffee.",".eeffee.","..eeee..","........"} };
static const u16 kaAmbPal[16]={ 0, RGB(31,31,31), RGB(24,28,31), RGB(28,15,3), RGB(15,7,2), RGB(5,5,8), RGB(8,4,12), RGB(31,31,10), RGB(18,31,8), RGB(30,27,19), RGB(31,20,3), RGB(12,22,31), RGB(31,18,24), RGB(15,8,3), RGB(4,3,3), RGB(28,6,5) };
// (the digits above: 3 orange leaf, 4 brown, 5 dark body, 6 bat, 7 and 8 the firefly's light, 9 canvas, a and b butterfly wings, c petal, d wagon wood, e wheel, f red)
// the picture of tile t as 8 words of 4 bits a pixel (low nibble first), row by row: 16 half words
static inline void kaAmbTile(int t,u16*out){
    for(int r=0;r<8;r++){ unsigned v=0; for(int x=0;x<8;x++){ char c=kaAmbArt[t][r][x]; int n=c>='a'?10+(c-'a'):c>='0'&&c<='9'?c-'0':0; v|=(unsigned)n<<(x*4); } out[r*2]=(u16)v; out[r*2+1]=(u16)(v>>16); }
}
// ---- the plan: appended to the sprites of the map. Returns the new count. part: kfPart (0 dawn .. 4 night) ----
static inline int kaPut(u16*o,int n,int x,int y,int tile,int flip){
    if(n>=124||x<-8||x>=240||y<-8||y>136) return n;   // (nothing over the text strip: the bottom 16 rows, so the picture's bottom edge stays above row 144)
    o[n*3]=(u16)(y&255); o[n*3+1]=(u16)((x&511)|(flip?0x1000:0)); o[n*3+2]=(u16)((KA_AMB+tile)|(0<<10)|(KA_AMBPAL<<12)); return n+1; }
static inline int kaTri(unsigned p,int amp){ unsigned q=p&255u; int v=q<128u?(int)q:255-(int)q; return (v-64)*amp/64; }
static inline int kaAmbient(u16*o,int n,const KSim*S,unsigned tick){
    int sn,nx,k; kaSeason(S,&sn,&nx,&k); int part=(int)((S->t%KS_DAYT)*5u/KS_DAYT), day=part<=3, night=part==4, dusk=part==3;
    if(sn==KS_WINTER||(k>8&&nx==KS_WINTER)){ int cnt=sn==KS_WINTER?26:8; for(int i=0;i<cnt;i++){ unsigned h=kvHash(i,91); int sp=1+(int)(h&1u); int x=(int)((h>>4)%240u)+kaTri(tick*2u+(unsigned)i*37u,6), y=(int)((tick*(unsigned)sp/2u+(h>>12))%176u)-8;
            n=kaPut(o,n,x,y,(h>>9&3u)==0?1:0,0); } }
    if(sn==KS_AUTUMN||(k>8&&nx==KS_AUTUMN)){ int cnt=sn==KS_AUTUMN?7:3; for(int i=0;i<cnt;i++){ unsigned h=kvHash(i,57); int x=(int)((h>>4)%230u)+kaTri(tick*3u+(unsigned)i*53u,14), y=(int)((tick/2u+(h>>12))%176u)-8; n=kaPut(o,n,x,y,2,(int)((tick>>4)+i)&1); } }
    if(day&&(sn==KS_SPRING||sn==KS_SUMMER)){ int cnt=sn==KS_SUMMER?4:3; for(int i=0;i<cnt;i++){ unsigned h=kvHash(i,13); int x=40+(int)(h%160u)+kaTri(tick*2u+(unsigned)i*71u,34), y=30+(int)((h>>9)%80u)+kaTri(tick*3u+(unsigned)i*29u,12); n=kaPut(o,n,x,y,((tick>>3)+(unsigned)i)&1u?9:8,0); } }
    if(sn==KS_SPRING&&day){ for(int i=0;i<4;i++){ unsigned h=kvHash(i,29); int x=(int)((h>>4)%230u)+kaTri(tick*2u+(unsigned)i*41u,10), y=(int)((tick/3u+(h>>12))%176u)-8; n=kaPut(o,n,x,y,10,0); } }   // blossom drifting down
    if(day){ unsigned c=tick%1800u; if(c<170u){ for(int i=0;i<3;i++){ int x=(int)(c*3u/2u)-30-i*14, y=24+i*9+kaTri(c*4u+(unsigned)i*30u,6); n=kaPut(o,n,x,y,((c>>3)+(unsigned)i)&1u?4:3,0); } } }   // a flock crosses now and then
    if(night&&sn!=KS_WINTER){ int cnt=sn==KS_SUMMER?9:6; for(int i=0;i<cnt;i++){ unsigned h=kvHash(i,5); int x=20+(int)(h%200u)+kaTri(tick+(unsigned)i*47u,16), y=50+(int)((h>>9)%80u)+kaTri(tick*2u+(unsigned)i*19u,10); if(((tick>>4)+(unsigned)i*3u)%9u<7u) n=kaPut(o,n,x,y,7,0); } }   // fireflies, blinking
    if(dusk||night){ for(int i=0;i<2;i++){ unsigned h=kvHash(i,99); int x=(int)((tick*3u/2u+(h&255u)*2u)%300u)-30, y=30+(int)((h>>9)%50u)+kaTri(tick*5u+(unsigned)i*61u,18); n=kaPut(o,n,x,y,((tick>>2)&1u)?5:6,0); } }   // bats
    return n;
}

// ---- THE CARAVAN: a trader camps by the storehouse for four days of every eight (days 2 to 5 of a season, so once a season), and buys what the realm has too much of: HAND + A near the wagon
// (kpTrade, kingplay.h) opens its list, or sells the load in the giant's hands. The money is the life game's own (simMoneyAdd). What it wants this visit pays half as much again. ----
static const char* const kaGoodNm[KG_GOODS]={ "WOOD", "STONE", "WOOL", "CRYSTAL", "PLANKS", "CUT STONE", "CLOTH", "POLISHED" };
static const u16 kaPrice[KG_GOODS]={ 20, 25, 45, 90, 60, 75, 130, 220 };   // simoleons a piece [ASSUMED, tune freely]
static inline int kaStoreOf(const KSim*S){ for(int i=0;i<S->R->nB;i++){ const KBld*b=&S->B[i]; if(b->lvl&&ksT(b->type)->kind==KK_STORE) return i; } return -1; }
static inline int kaCaravanHere(const KSim*S){ int d=(int)(S->R->day&7u); return d>=2&&d<=5&&kaStoreOf(S)>=0; }
static inline int kaWanted(const KSim*S){ return (int)((S->R->day>>3)*3u+1u)&7; }   // a different good each visit: every one of the eight comes round in two years
static inline int kaPay(const KSim*S,int g,int n){ int v=(int)kaPrice[g&7]*n; return g==kaWanted(S)?v+v/2:v; }
// the wagon's cell: two cells off the storehouse's right side, one down (the giant is "at the wagon" anywhere within two cells of it: it is a big fellow)
static inline int kaWagonCell(const KSim*S,int*wx,int*wy){ int i=kaStoreOf(S); if(i<0) return 0; *wx=S->B[i].x+3; *wy=S->B[i].y+1; return 1; }
static inline int kaNearWagon(const KSim*S,int cx,int cy){ int wx,wy; if(!kaCaravanHere(S)||!kaWagonCell(S,&wx,&wy)) return 0; return cx>=wx-2&&cx<=wx+2&&cy>=wy-2&&cy<=wy+2; }
static inline int kaCaravan(u16*o,int n,const KSim*S,const KView*V,unsigned tick){   // the wagon and two traders (appended to the plan)
    int wx,wy; if(!kaCaravanHere(S)||!kaWagonCell(S,&wx,&wy)) return n;
    int px=wx*KV_CELL, py=wy*KV_CELL; if(!kfOn(V,px-8,py-8,48,40)) return n;
    if(n<KF_MAX-3){ n=(int)kfPut(o,n,px-V->x,py-V->y,0,1,0,KA_AMB+11,1,KA_AMBPAL); }
    n=kfNub(o,n,V,px+22,py+15,0,(int)((tick>>5)&1u),1,2); n=kfNub(o,n,V,px+31,py+14,1,(int)((tick>>4)&1u),0,0);
    return n; }
#endif
