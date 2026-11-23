// gfx2.h - GodGuy's shared handheld pixel art. Lit clusters, quiet ground,
// individual settlement silhouettes and connected shorelines use the same
// 8x8 tiles in DS, DSi and New3DS map modes, including close/micro zoom.
// Integer maths only (the DS has no FPU). Ground uses terrain*4+variant.
// Building tiles use the BT0 and BT1 ranges (see BTILE in main.c).
// New palette entries (main background): 104..114 details (flowers, hot lava, cactus, cattail), 144..159 roofs (dark, light), 160 window glass.
#define GFX_PAL_N 192
#define UI_RESV(i) (((i)>=117&&(i)<=127)||((i)>=161&&(i)<=167))   // toolbar colours (ui.h): the day / night light leaves these slots alone
#define GFX_MEADOW0 108          // 16 spare tile slots (108..123): grass with flowers
#define GFX_MEADOW_N 16
enum { PX_FLW=104, PX_LAVAHOT=110, PX_LAVAWHITE=111, PX_CACT=112, PX_CACTD=113, PX_CATTAIL=114, PX_SNOWS1=115, PX_SNOWS2=116, PX_ROOFD=144, PX_ROOFL=152, PX_WIN=160 };
static const u8 gfxDet[13][3]={ {31,31,26},{30,26,13},{28,18,21},{27,11,10},{22,17,28},{30,21,13},{31,24,7},{31,31,22},{12,21,12},{6,13,9},{21,15,9},{21,25,28},{25,28,29} };
static void gfxBgPal(void){
    for(int i=0;i<13;i++) BG_PALETTE[PX_FLW+i]=RGB15(gfxDet[i][0],gfxDet[i][1],gfxDet[i][2]);
    for(int r=0;r<8;r++){ int a=roofCol[r][0],b=roofCol[r][1],c=roofCol[r][2];
        BG_PALETTE[PX_ROOFD+r]=RGB15(a*5/8,b*5/8,c*5/8+1);
        int la=a*11/10+4, lb=b*11/10+4, lc=c*11/10+3; if(la>31) la=31; if(lb>31) lb=31; if(lc>31) lc=31; BG_PALETTE[PX_ROOFL+r]=RGB15(la,lb,lc); }
    BG_PALETTE[PX_WIN]=RGB15(11,22,26);
}
#define SH(t,k) ((t)*8+((k)<0?0:(k)>7?7:(k)))
static inline int gAbs(int a){ return a<0?-a:a; }

// ---------------------------------------------------------------- grass and meadows
static int pxGrass(int v,int x,int y){
    // One coherent tuft and a soft two-pixel patch, not random confetti.
    int base=v==2?5:v==3?3:4;
    int tx=(v*3+2)&7,ty=2+(v&1)*3;
    if(y==ty&&x==tx)return SH(T_GRASS,base+1);
    if(y==ty+1&&(x==tx||x==((tx+1)&7)))return SH(T_GRASS,base-1);
    if(y==((ty+4)&7)&&((x+v)&7)<2)return SH(T_GRASS,base+1);
    return SH(T_GRASS,base);
}
static int pxMeadow(int m,int x,int y){
    for(int k=0;k<1+(m>=6);k++){ int fx=(int)((hsh(m,k,51)>>3)&7), fy=(int)((hsh(m,k,52)>>3)%7);
        if(x==fx&&y==fy) return PX_FLW+(m+k*3)%6; if(x==fx&&y==fy+1) return SH(T_GRASS,2); }
    return pxGrass(2,x,y);
}
// ---------------------------------------------------------------- a lit ball of leaves (half-pixel coordinates): ramp step 0..n-1, or -1 outside
static int gBall(int x,int y,int cx2,int cy2,int r2,int n){
    int dx=2*x+1-cx2, dy=2*y+1-cy2, d2=dx*dx+dy*dy; if(d2>r2*r2) return -1;
    int s=(n-1)/2+(-(dx+dy)*(n-1))/(r2*3);
    if(((x+cx2)/2+(y+cy2)/2)%4==0)s++; // clusters, rather than single-pixel speckling
    if(d2>(r2-2)*(r2-2)&&r2>2){ if(dx+dy>0) s=0; else s--; }
    return s<0?0:s>n-1?n-1:s;
}
static int pxForest(int t,int v,int x,int y){   // forest (T_FOREST) and jungle (T_JUNGLE)
    static const char canopy[4][8][9]={
      {"...66...",".67754..","6776553.","67655421",".5544321","..32221.","...tdq..","..qqqq.."},
      {"..66....",".6754...","675543..",".54321..","..21.65.","..td6542","..q.3211","....tqq."},
      {"....6...","...674..","..67542.","...543..","..654321",".6554321","..32221.","....tqq."},
      {".66..6..","67546743",".5435542","..216532",".6654321","6543221.",".321tdq.","..tqqqq."}
    };
    char c=canopy[v][y][x];
    if(c>='1'&&c<='7')return SH(t,c-'0');
    if(c=='t')return SH(T_DIRT,5);if(c=='d')return SH(T_DIRT,2);
    if(c=='q')return SH(T_GRASS,1);
    if(t==T_JUNGLE&&v==1&&x==7&&y==3)return PX_FLW+2;
    return SH(T_GRASS,3);
}
// ---------------------------------------------------------------- hills: a grassy mound with an earth flank
static int gMound(int x,int y,int cx2,int yb,int hw,int h,int*dxo){   // 1 inside the half-ellipse mound standing on row yb
    if(y>yb) return 0; int dx2=2*x+1-cx2, dy2=2*(yb+1-y)-1, h2=2*h; if(dy2<0) return 0;
    *dxo=dx2; return dx2*dx2*h2*h2+dy2*dy2*hw*hw<=hw*hw*h2*h2;
}
static int pxHill(int v,int x,int y){
    static const signed char M[4][2][4]={ {{7,6,7,4},{-1,0,0,0}}, {{4,7,5,3},{12,7,4,2}}, {{10,6,6,4},{2,7,3,2}}, {{8,7,9,5},{-1,0,0,0}} };
    int hh=(int)(hsh(x+v*8,y,84)&7);
    for(int k=0;k<2;k++){ if(M[v][k][0]<0) continue; int cx2=M[v][k][0], yb=M[v][k][1], hw=M[v][k][2], h=M[v][k][3], dx2; if(!gMound(x,y,cx2,yb,hw,h,&dx2)) continue;
        int depth=(yb-y), top=depth>=h-2, rim=!gMound(x+1,y,cx2,yb,hw,h,&dx2)||(y==yb);   // right and bottom edge: dark outline
        gMound(x,y,cx2,yb,hw,h,&dx2); int lit=3-(dx2*3)/hw;   // left side lighter
        if(rim&&(dx2>0||y==yb)) return SH(T_HILL,1);
        if(top){ int g=3+(lit>=3)+(lit>=5)+(hh==0); return SH(T_GRASS,g); }
        int e=lit+1+(hh==1)-(hh==0); return SH(T_HILL,e<2?2:e>6?6:e); }
    return SH(T_GRASS,2+(hh<2));
}
// ---------------------------------------------------------------- mountains: lit left face, shaded right face, snow caps
static int pxMount(int v,int x,int y){
    static const signed char PK[4][2][2]={ {{7,1},{-1,0}}, {{3,3},{11,2}}, {{5,2},{13,5}}, {{9,1},{2,5}} };
    int x2=2*x+1, hh=(int)(hsh(x+v*8,y,91)&7);
    for(int k=0;k<2;k++){ int ax2=PK[v][k][0], ay=PK[v][k][1]; if(ax2<0) continue; int w=2*(y-ay)+1; if(y<ay||gAbs(x2-ax2)>w) continue;
        int left=x2<ax2, edge=gAbs(x2-ax2)>=w-1, depth=y-ay;
        if(depth<=1+(k==0&&v!=2)) return left?SH(T_SNOW,hh==0?6:7):SH(T_SNOW,hh==0?3:4);   // snow cap
        if(edge) return left?SH(T_MOUNT,4):SH(T_MOUNT,1);
        if(left) return SH(T_MOUNT,hh==0?5:hh==1?7:6);
        return SH(T_MOUNT,hh==0?1:hh==1?4:3); }
    return SH(T_MOUNT,2+(hh==0)-(hh==1)+(y>=6));
}
// ---------------------------------------------------------------- sand, desert, dirt, snow
static int pxSand(int v,int x,int y){
    int base=v==2?5:4,dx=(x+v*3)&7,dy=(y+v)&7;
    if(dy==3&&dx>=2&&dx<=5)return SH(T_SAND,base+1);
    if(dy==4&&dx>=3&&dx<=5)return SH(T_SAND,base-1);
    if(v==3&&x==1&&y==6)return PX_FLW;
    return SH(T_SAND,base);
}
static int pxDesert(int v,int x,int y){
    static const signed char W[8]={0,1,1,0,-1,-1,0,1};
    int ridge=3+W[(x+v*2)&7]+(v&1), s=y<ridge?5:y==ridge?6:y<=ridge+1?3:4;
    if(v==3&&x==5&&y>=3&&y<=6){ return y>=4&&(x==5)?PX_CACT:PX_CACT; }
    if(v==3&&((x==4&&y==4)||(x==6&&y==3))) return PX_CACTD;
    return SH(T_DESERT,s);
}
static int pxDirt(int v,int x,int y){
    int base=v==2?5:v==3?3:4, s=base;
    int sx=(int)((hsh(v,1,121)>>3)&7), sy=(int)((hsh(v,1,122)>>3)%6);   // a pebble with a shadow
    if(x==sx&&y==sy) s=7; else if(x==((sx+1)&7)&&y==sy+1) s=1;
    if(v==1&&y==5&&x>=2&&x<=5) s=base-2; if(v==3&&x==y&&x<5) s=base-1;     // cracks
    return SH(T_DIRT,s);
}
static int pxSnow(int v,int x,int y){
    static const signed char W[8]={0,1,1,0,-1,-1,0,1};
    int h=(int)(hsh(x+v*8,y,31)&31), ridge=2+W[(x+v*3)&7]+(v&1)*2+(v==3), s=v==3?5:6;
    if(h==0) s=5; else if(h==1) s=7;
    if(y==ridge+1) return PX_SNOWS2; if(y==ridge+2&&(h&1)) return PX_SNOWS1;     // soft shadow under a drift
    return SH(T_SNOW,s);
}
// ---------------------------------------------------------------- swamp
static int pxSwamp(int v,int x,int y){
    static const signed char P[4][4]={ {3,2,3,2},{2,5,3,1},{5,3,2,2},{3,4,3,2} };   // a pool: centre x, y, half width, half height
    int h=(int)(hsh(x+v*8,y,32)&7), s=3+(h>5)+(((x+y*2+v)&7)==0);
    int dx=x-P[v][0], dy=y-P[v][1]; int hw=P[v][2], hh=P[v][3];
    if(dx*dx*hh*hh+dy*dy*hw*hw<=hw*hw*hh*hh+hw){ if(dx*dx*hh*hh+dy*dy*hw*hw>hw*hw*hh*hh-hw*2) return SH(T_WATER,1); return h==0?SH(T_WATER,4):SH(T_WATER,2+(dy<0&&dx<1)); }
    int rx=(int)((hsh(v,5,141)>>3)&7), ry=(int)((hsh(v,6,141)>>3)%3)+2;
    if(x==rx&&y>=ry&&y<=ry+3){ if(y==ry) return PX_CATTAIL; return SH(T_SWAMP,y==ry+1?7:6); }
    if((x==((rx+1)&7)&&y==ry+3)||(x==((rx+7)&7)&&y==ry+2)) return SH(T_SWAMP,6);
    if(v==2&&((x==6&&y==6)||(x==7&&y==6))) return SH(T_GRASS,5);     // a lily pad
    return SH(T_SWAMP,s);
}
// ---------------------------------------------------------------- lava: dark plates, glowing cracks
static int pxLava(int v,int x,int y){
    int d1=999,d2=999; for(int k=0;k<3;k++){ int px=(int)(hsh(v,k,101)&7), py=(int)(hsh(v,k,102)&7); int dx=gAbs(x-px); if(dx>4) dx=8-dx; int dy=gAbs(y-py); if(dy>4) dy=8-dy; int d=dx*dx+dy*dy; if(d<d1){ d2=d1; d1=d; } else if(d<d2) d2=d; }
    int e=d2-d1, h=(int)(hsh(x+v*8,y,31)&7);
    if(e<=1) return (e==0&&h<2)?PX_LAVAHOT:SH(T_LAVA,e==0?7:6);
    if(e<=2) return SH(T_LAVA,4);
    return SH(T_LAVA,h==0?2:h==1?3:1+(h==2));
}
// ---------------------------------------------------------------- water: wave crests over a gently mottled sea
static int pxWater(int t,int v,int x,int y){
    int deep=t==T_DEEP,base=deep?2:3;
    int dx=(x+v*3)&7,crest=2+(v&1)*3;
    // Long horizontal curls separated by clear water. Deep-sea glints are
    // dimmer than shallows; the bright foam belongs only to real coasts.
    if(y==crest&&dx>=2&&dx<=5)return SH(t,deep?4:4+(dx==3));
    if(y==crest+1&&(dx==1||dx==6))return SH(t,deep?3:4);
    if(y==crest+1&&dx>=3&&dx<=5)return SH(t,base-1);
    return SH(t,base+(v==2&&y>5));
}
// ---------------------------------------------------------------- the ground tile of terrain t, variant v (0 plain, 1 plain, 2 light patch, 3 dark patch for the flat grounds)
static int tilePix(int t,int v,int x,int y){
    switch(t){
    case T_DEEP: case T_WATER: return pxWater(t,v,x,y);
    case T_SAND: return pxSand(v,x,y);
    case T_GRASS: return pxGrass(v,x,y);
    case T_FOREST: case T_JUNGLE: return pxForest(t,v,x,y);
    case T_HILL: return pxHill(v,x,y);
    case T_MOUNT: return pxMount(v,x,y);
    case T_SNOW: return pxSnow(v,x,y);
    case T_LAVA: return pxLava(v,x,y);
    case T_DESERT: return pxDesert(v,x,y);
    case T_SWAMP: return pxSwamp(v,x,y);
    default: return pxDirt(v,x,y); }
}
// which of the four variants a cell uses: the flat grounds get large, soft patches (light and dark) instead of pure noise; the built shapes (trees, peaks, hills) stay random
static int tileVar(int t,int x,int y){
    int r=(int)(hsh(x,y,3)&3);
    if(t==T_FOREST||t==T_JUNGLE||t==T_HILL||t==T_MOUNT||t==T_LAVA) return r;
    int m=vnoise(x,y,6,77); if(m>160&&r<2) return 2; if(m<96&&r<2) return 3; return r&1;
}
// Settlement silhouettes use the original 8x8 tiles. Roof ridges,
// shaded eaves, stone corners and timber framing read at native scale.
static const char gfxBuildings[B_N][8][9]={
 [B_HUT]={"...R....","..RRr...",".RRrrs..",".srrss..","..wWwq..","..|dwq..","..-ddqq.","...qqq.."},
 [B_HOUSE]={".....m..","..RRrS..",".RRRRrr.","RRRrrrss",".ssssssq",".Wo|dwq.",".WW|dwq.","..--dqq."},
 [B_HALL]={"..|ff...","..|f....","S.S.S.S.","SmmmmmSq","SkWdoSkq","SmmdoSkq","SmmddSkq","SmmmmSSq"},
 [B_FARM]={"|------|",".cC.cC.|",".ce.ce.e",".cC.cC.e",".ce.ce.e",".cC.cC.e",".ee.eee|","|------|"},
 [B_TOWER]={"..S.S...","..SmS...","..Smkq..","..Sokq..","..Smkq..","..Sokq..","..Sdkq..",".SSSSqq."},
 [B_RUIN]={"........","..k.....","..Sk..k.","..mk.Sk.",".kmm.Skq",".mkmkmqq",".qqkmq..","..qqq..."},
 [B_DOCK]={"........",".|....|.",".W----W.","WWWWWWWW","w-w-w-ww",".|....|q",".q....q.","........"},
 [B_TEMPLE]={"...ff...","...S....","..RRrr..",".RRRrrs.",".SSSSSSq",".S|o|Skq",".S|d|Skq","SSSSSSSq"},
 [B_STATUE]={"...SS...","...Wk...","..SSmk..","...Sm...","...Sk...","..SSmk..",".SSmmmkq",".qqqqqq."},
 [B_LIBRARY]={".RRRRrr.",".RRrrrs.",".ssssss.",".W|W|wq.",".o|o|oq.",".W|W|wq.",".WWddwq.",".dddddq."},
 [B_BORDER]={"........","...|ff..","...|f...","...|....","...|q...","...|q...","..SSq...","..qqq..."},
 [B_MANOR]={".R...R..","RRr.RRr.","RRrsRRrs","SSSssSSq","WoW|oWmq","WW||Wwmq","Wo|ddomq","SSm--mqq"},
 [B_WORKSHOP]={".....SS.",".....Sm.",".RRRRSm.","RRrrrss.",".ss||swq",".W|dd|mq",".WoddCmq",".--eeeqq"},
 [B_INN]={"..RRrr..",".RRRRrs.","RRRRrrs.",".ssssss.",".oW|o|fq",".WWdd|Wq",".Woee|mq","..--kqq."},
 [B_BARRACKS]={".f..f...",".|..|...","..RRrr..",".RRRRrs.","SSSSSSSq","Smomomkq","Sm|ddmkq",".kk--qq."},
 [B_MARKET]={"........",".RRWWRR.","RRWWRRWw","s|sws|sq",".|..C|q.",".|cCc|q.",".W----wq","..eeqq.."},
 [B_KEEP]={"S.S..S.S","SmSm.Smk","SmSSSSmk","SokmSmok","SmkmSmkk","Smkmddmk","SSmSddmk",".kkk--kq"},
 // The wall tiles join at their midpoints in either direction. Gate
 // names indicate which way the road passes through the stonework.
 [B_WALL]={"..SSSm..","..S.Sk..","SSSSSSSm","SmmSmkmm","mSkmmkmk","kkkkkkkk","..kmmq..","..mkkq.."},
 [B_GATE_NS]={"S.S..S.S","SSS..SSS","mSk..Smk","SSWkkWSS","mSkddSmk","mSk--Smk","kSk--mkk","qq.--qqq"},
 [B_GATE_EW]={"..SSm...","..Smk...",".SSSkq..","---kk---","...dd...",".SSSkq..","..Smk...","..kqq..."},
 // North-up home plans. Match roof and wall edges to the shared east,
 // south and southeast wings so the plots read as a single structure.
 [B_HOUSE_EW]={"....RRRr","...mRRRr",".RRRRRrr",".sssssss",".W|oWW|o",".Wo|ddW|",".WW|ddWo","..mm--mm"},
 [B_HOUSE_NS]={"..SRr...",".RRmrrs.",".RRRrrss",".RRRrrss",".RRrrrss",".RRRrrss",".RrRrrss",".RRrrsss"},
 [B_ESTATE]={"..S.RRRr",".RmRRRRr","RRRRRRrr","mSSSssss","SWWoWW|o","SmWWoW|W","So|WW|WW","mRRrrsss"},
 [B_WING_E]={"rrrrs...","rrrrrss.","rrrsrssq","ssssssq.","W|oWwmq.","Wo|WWmq.","WW|oWmq.","mmmmkkq."},
 [B_WING_S]={".RRrrsss",".RRRrrss",".sssssss",".Wo|oWmm",".WW|dWmk",".Wo|dWmk",".mmm--km","..eeqqqq"},
 [B_WING_SE]={"rrrs....","rrrrrss.","ssssssq.","mW|oWmq.","mWo|Wmq.","mW|oWmq.","m--mmkq.","qqeeqq.."},
 // Stagger the joints between courses, including where the tiles repeat.
 [B_PAVING]={"::::::::","m:mmm:mm","p:mmp:mp","p:ppp:pp","::::::::","mmm:mmm:","mmp:mpp:","ppp:ppp:"},
 // A small timber canopy over a lit stone rim and a deep opening.
 [B_WELL]={"...Rr...","..RRrr..",".ssssss.","..-..|..","..WWSS..",".SOOOSk.",".mWSSmkk","..kmkkk."}
};
static int bldPix(int type,int race,int x,int y){
    if((unsigned)x>=8||(unsigned)y>=8||type<=B_NONE||type>=B_N)return SH(T_GRASS,4);
    char c=gfxBuildings[type][y][x];
    if(type==B_WELL&&c=='.')c=gfxBuildings[B_PAVING][y][x];
    switch(c){
    case 'R':return PX_ROOFL+race;case 'r':return 128+race;case 's':return PX_ROOFD+race;
    case 'W':return 136;case 'w':return SH(T_SAND,3);case 'd':return SH(T_DIRT,1);
    case 'o':return PX_WIN;case 'S':return SH(T_MOUNT,7);case 'm':return SH(T_MOUNT,5);
    case 'k':return SH(T_MOUNT,2);case '|':return 137;case '-':return SH(T_DIRT,5);
    case 'c':return SH(T_GRASS,6);case 'C':return PX_FLW+1;case 'e':return SH(T_DIRT,3);
    case 'f':return PX_ROOFL+race;case 'q':return SH(T_GRASS,1);
    case ':':return SH(T_MOUNT,3);case 'p':return SH(T_MOUNT,4);case 'O':return SH(T_MOUNT,0);
    default:return pxGrass(0,x,y);}
}
// ---------------------------------------------------------------- light: windows glow at night, creatures are tinted a little by the day
static u16 gfxSprBase[256];
static void gfxSprPal(int b){   // bank b: 1 body, 2 dark, 3 accent, 4 white, 5 body highlight, 6 body shade, 7 outline-dark
    for(int k=0;k<3;k++) gfxSprBase[b*16+1+k]=RGB15(famCol[b][k][0],famCol[b][k][1],famCol[b][k][2]); gfxSprBase[b*16+4]=RGB15(31,31,31);
    int r=famCol[b][0][0],g=famCol[b][0][1],c=famCol[b][0][2];
    gfxSprBase[b*16+5]=RGB15(r+(31-r)*2/5,g+(31-g)*2/5,c+(31-c)*2/5); gfxSprBase[b*16+6]=RGB15(r*6/10,g*6/10,c*6/10);
    gfxSprBase[b*16+7]=RGB15(3,4,5);
    for(int k=0;k<8;k++) SPRITE_PALETTE[b*16+k]=gfxSprBase[b*16+k]; SPRITE_PALETTE[b*16]=0;
}
static int gfxSprIdx(int s,int f,int x,int y){
    char ch=shpRow(s,f,y)[x]; if(ch=='e') return 4; if(ch=='a') return 3; if(ch=='d') return 2;
    if(ch!='b'){
        // A single dark contact edge makes bodies readable on the busy
        // ground without expanding sprite size or adding OAM entries.
        if(y>1&&y<7&&shpRow(s,f,y-1)[x]!='.'&&shpRow(s,f,y-1)[x]!='d')return 7;
        return 0;
    }
    int up=y>0?shpRow(s,f,y-1)[x]:'.', dn=y<7?shpRow(s,f,y+1)[x]:'.', lf=x>0?shpRow(s,f,y)[x-1]:'.', rt=x<7?shpRow(s,f,y)[x+1]:'.';
    int hi=(up=='.')||(lf=='.'), lo=(dn=='.')||(rt=='.'); if(hi&&!lo) return 5; if(lo&&!hi) return 6; return 1;
}
static int gfxCitizenRace(int shape){
    return shape==5?0:shape>=24&&shape<=30?shape-23:-1;
}
// Close views earn additional detail instead of only magnifying the 8x8
// OAM picture. This painter uses existing palette entries and a fixed 16x16
// footprint; feet stay on the final row through every walk phase/race.
static int gfxCitizenPix(int race,int frame,int variant,int x,int y){
    int shortBody=race==2||race==5||race==6;
    int hy=y-shortBody*2,dx=x-7;
    int faceWidth=race==3||race==7?3:2;
    if(race==6&&hy>=0&&hy<=2&&gAbs(dx)<=hy)return hy==0?8:3;
    if(hy>=1&&hy<=2&&gAbs(dx)<=faceWidth+(hy==2)){
        if(race==2)return hy==1?4:2;
        return hy==2&&x==8+(variant&1)?1:2;
    }
    if(hy>=3&&hy<=5&&gAbs(dx)<=faceWidth){
        if(hy==3&&(x==6||x==8))return 4;
        if(hy==3&&x==7)return 2;
        if(hy==5&&race==2)return 2;
        return x==7+faceWidth?6:1;
    }
    if(hy==4&&(race==1||race==4)&&(x==3||x==11))return 1;
    if(hy==6&&gAbs(dx)<=1)return race==2?2:1;
    int width=race==7?4:race==3?3:2;
    if(y>=7&&y<=11&&gAbs(dx)<=width){
        if(y==10)return x==7?4:2;
        if(x==7+width||y==11)return 9;
        if(x==7-width||(variant&1&&x==6))return 8;
        return 3;
    }
    int armY=7+((frame+(x<7))&1);
    if(gAbs(dx)==width+1&&y>=armY&&y<=armY+2)return y==armY+2?1:3;
    int left=5+(frame&1),right=9-(frame&1);
    if(y>=12&&y<=14&&((x>=left&&x<=left+1)||(x>=right&&x<=right+1)))return 2;
    if(y==15&&((x>=left-1&&x<=left+1)||(x>=right&&x<=right+2)))return 7;
    return 0;
}
static void gfxLight(int Lr,int Lg,int Lb){
    int night=Lr<150;   // windows: dull glass by day, warm light when it is dark
    BG_PALETTE[PX_WIN]=night?RGB15(31,26,12):RGB15(11*Lr>>8,22*Lg>>8,26*Lb>>8);
    int fr=(256+Lr)/2, fg=(256+Lg)/2, fb=(256+Lb)/2;
    for(int b=0;b<16;b++) for(int k=1;k<8;k++){ u16 c=gfxSprBase[b*16+k]; int r=(c&31)*fr>>8, g=((c>>5)&31)*fg>>8, bl=((c>>10)&31)*fb>>8; if(r>31) r=31; if(g>31) g=31; if(bl>31) bl=31; SPRITE_PALETTE[b*16+k]=RGB15(r,g,bl); }
}
