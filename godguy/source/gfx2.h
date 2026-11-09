// gfx2.h - GODGUY graphics pass 1 (included once from main.c, right before bldPix).
// Richer ground tiles (tufts, meadows, shaded trees, snow-capped peaks, dunes, cracked lava, waves), a calmer mottling of the ground over large areas,
// buildings with shaded roofs and windows that light up at night, and creatures with a lit top edge and a shaded underside.
// Integer maths only (the DS has no FPU). Tile and palette layout is unchanged: ground is still terrain*4+variant, buildings still BT0+type*8+race.
// New palette entries (main background): 104..114 details (flowers, hot lava, cactus, cattail), 144..159 roofs (dark, light), 160 window glass.
#define GFX_PAL_N 168
#define GFX_MEADOW0 108          // 16 spare tile slots (108..123): grass with flowers
#define GFX_MEADOW_N 16
enum { PX_FLW=104, PX_LAVAHOT=110, PX_LAVAWHITE=111, PX_CACT=112, PX_CACTD=113, PX_CATTAIL=114, PX_SNOWS1=115, PX_SNOWS2=116, PX_ROOFD=144, PX_ROOFL=152, PX_WIN=160 };
static const u8 gfxDet[13][3]={ {31,31,27},{31,28,5},{30,13,19},{29,5,5},{21,11,29},{31,19,4},{31,26,6},{31,31,22},{7,19,7},{3,10,4},{15,8,3},{22,26,31},{26,29,31} };
static void gfxBgPal(void){
    for(int i=0;i<13;i++) BG_PALETTE[PX_FLW+i]=RGB15(gfxDet[i][0],gfxDet[i][1],gfxDet[i][2]);
    for(int r=0;r<8;r++){ int a=roofCol[r][0],b=roofCol[r][1],c=roofCol[r][2];
        BG_PALETTE[PX_ROOFD+r]=RGB15(a*5/8,b*5/8,c*5/8);
        int la=a*13/10+3, lb=b*13/10+3, lc=c*13/10+3; if(la>31) la=31; if(lb>31) lb=31; if(lc>31) lc=31; BG_PALETTE[PX_ROOFL+r]=RGB15(la,lb,lc); }
    BG_PALETTE[PX_WIN]=RGB15(12,17,24);
}
#define SH(t,k) ((t)*8+((k)<0?0:(k)>7?7:(k)))
static inline int gAbs(int a){ return a<0?-a:a; }

// ---------------------------------------------------------------- grass and meadows
static int pxGrass(int v,int x,int y){
    int base=v==2?5:v==3?3:4, s=base; int h=(int)(hsh(x+v*8,y+v*5,31)&31);
    if(h<2) s=base-1; else if(h<6) s=base+1;
    for(int k=0;k<2+(v&1);k++){ int tx=(int)((hsh(v,k,41)>>4)&7), ty=(int)((hsh(v,k,43)>>4)%6);   // little tufts: a light tip over a dark root
        if(x==tx&&y==ty) s=base+2; else if(x==tx&&y==ty+1) s=base-2; else if(k==0&&x==((tx+1)&7)&&y==ty+1) s=base+1; }
    return SH(T_GRASS,s);
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
    int hh=(int)(hsh(x*5+cx2,y*3+cy2,71)&7); if(hh==0) s--; else if(hh==1) s++;
    if(d2>(r2-2)*(r2-2)&&r2>2){ if(dx+dy>0) s=0; else s--; }
    return s<0?0:s>n-1?n-1:s;
}
static int pxForest(int t,int v,int x,int y){   // forest (T_FOREST) and jungle (T_JUNGLE)
    static const u8 RF[7]={T_FOREST*8+1,T_FOREST*8+3,T_FOREST*8+4,T_FOREST*8+5,T_FOREST*8+7,T_GRASS*8+3,T_GRASS*8+4};
    static const u8 RJ[7]={T_JUNGLE*8+1,T_JUNGLE*8+3,T_JUNGLE*8+4,T_JUNGLE*8+5,T_JUNGLE*8+7,T_GRASS*8+4,T_GRASS*8+6};
    const u8*R=t==T_JUNGLE?RJ:RF; int s=-1, floor=t*8+1+((hsh(x+v*8,y,81)&7)==0);
    switch(v){
    case 0: s=gBall(x,y,7,6,7,7); if(s<0&&y==7&&(x==3||x==4)) return T_DIRT*8+2; break;
    case 1: s=gBall(x,y,5,5,5,7); if(s<0) s=gBall(x,y,11,10,5,7); if(s<0&&((x==2&&y>=6)||(x==5&&y==7))) return T_DIRT*8+2; break;
    case 2: s=gBall(x,y,9,7,6,7); if(s<0) s=gBall(x,y,3,12,3,7); if(s<0&&x==4&&y==7) return T_DIRT*8+2; break;
    default: s=gBall(x,y,4,4,4,7); if(s<0) s=gBall(x,y,12,5,4,7); if(s<0) s=gBall(x,y,8,12,4,7); if(s<0&&(x==1&&y==3)) return T_DIRT*8+2; break; }
    if(s>=0){ if(t==T_JUNGLE&&v==3&&x==5&&y==2) return PX_FLW+3; if(t==T_JUNGLE&&v==2&&(x==7||x==0)&&y==5) return PX_FLW+5; return R[s]; }
    return floor;
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
    int base=v==2?6:v==3?4:5, s=base, h=(int)(hsh(x+v*8,y,31)&15);
    if(h==0) s=base-1; else if(h==1) s=base+1;
    for(int k=0;k<2;k++){ int dx=(int)((hsh(v,k,111)>>3)&7), dy=(int)((hsh(v,k,112)>>3)%6)+1; int ex=(x-dx)&7;   // wind ripples: a dark dash with a bright one under it
        if(y==dy&&ex<3) s=base-2; else if(y==dy+1&&ex>=1&&ex<=3) s=base+2; }
    return SH(T_SAND,s);
}
static int pxDesert(int v,int x,int y){
    static const signed char W[8]={0,1,1,0,-1,-1,0,1};
    int h=(int)(hsh(x+v*8,y,31)&15), ridge=3+W[(x+v*2)&7]+(v&1), s=y<ridge?5:y==ridge?7:y<=ridge+2?3:4;
    if(h==0) s--; else if(h==1) s++;
    if(v==3&&x==5&&y>=3&&y<=6){ return y>=4&&(x==5)?PX_CACT:PX_CACT; }
    if(v==3&&((x==4&&y==4)||(x==6&&y==3))) return PX_CACTD;
    return SH(T_DESERT,s);
}
static int pxDirt(int v,int x,int y){
    int base=v==2?5:v==3?3:4, s=base, h=(int)(hsh(x+v*8,y,31)&15);
    if(h<3) s=base-1; else if(h<5) s=base+1;
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
    int deep=t==T_DEEP, base=deep?(v==2?3:v==3?1:2):(v==2?4:v==3?2:3); int h=(int)(hsh(x+v*8,y+t*16,t)&15); int s=base+(h<2?1:0)-(h==15&&!deep?1:0);
    for(int k=0;k<(deep?1:2);k++){ int x0=(int)((hsh(v,k,61)>>3)&7), yy=(int)((hsh(v,k+2,62)>>3)%6)+1; int dx=(x-x0)&7; if(dx>=4) continue;
        int wy=yy+((dx==0||dx==3)?1:0); if(y==wy) return deep?SH(t,dx==1||dx==2?5:4):SH(t,dx==1||dx==2?7:6); if(!deep&&y==wy+1&&(dx==1||dx==2)) s=base-1; }
    return SH(t,s);
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
// ---------------------------------------------------------------- buildings: the old shapes, with lit and shaded roofs, shaded walls and windows (glowing at night)
static int bldPixOld(int type,int race,int x,int y);
static int bldPix(int type,int race,int x,int y){
    int p=bldPixOld(type,race,x,y);
    if(p==128+race){ return x<4?PX_ROOFL+race:(y==3&&type==B_HOUSE)?PX_ROOFD+race:p; }
    if(p==136){ if(x>=6||(type==B_HUT&&x>=5)) return SH(T_SAND,3); return SH(T_SAND,5+(y==4)); }
    if(p==137){ if(type!=B_FARM) return PX_WIN; return p; }
    if(p==139){ if(x>=5||y==7) return SH(T_MOUNT,3); if(y<=2) return SH(T_MOUNT,7); return SH(T_MOUNT,5); }
    if(p==138) return SH(T_DIRT,2);
    if(type!=B_FARM&&type!=B_RUIN&&p!=PX_WIN&&p>=0&&p<104&&(x==7||y==7)&&bldPixOld(type,race,x-1,y-1)>=104) return SH(T_GRASS,1);   // a soft shadow on the grass to the lower right
    return p;
}
// ---------------------------------------------------------------- light: windows glow at night, creatures are tinted a little by the day
static u16 gfxSprBase[256];
static void gfxSprPal(int b){   // bank b: 1 body, 2 dark, 3 accent, 4 white, 5 body highlight, 6 body shade, 7 outline-dark
    for(int k=0;k<3;k++) gfxSprBase[b*16+1+k]=RGB15(famCol[b][k][0],famCol[b][k][1],famCol[b][k][2]); gfxSprBase[b*16+4]=RGB15(31,31,31);
    int r=famCol[b][0][0],g=famCol[b][0][1],c=famCol[b][0][2];
    gfxSprBase[b*16+5]=RGB15(r+(31-r)*2/5,g+(31-g)*2/5,c+(31-c)*2/5); gfxSprBase[b*16+6]=RGB15(r*6/10,g*6/10,c*6/10);
    for(int k=0;k<8;k++) SPRITE_PALETTE[b*16+k]=gfxSprBase[b*16+k]; SPRITE_PALETTE[b*16]=0;
}
static int gfxSprIdx(int s,int f,int x,int y){
    char ch=shpRow(s,f,y)[x]; if(ch=='e') return 4; if(ch=='a') return 3; if(ch=='d') return 2; if(ch!='b') return 0;
    int up=y>0?shpRow(s,f,y-1)[x]:'.', dn=y<7?shpRow(s,f,y+1)[x]:'.', lf=x>0?shpRow(s,f,y)[x-1]:'.', rt=x<7?shpRow(s,f,y)[x+1]:'.';
    int hi=(up=='.')||(lf=='.'), lo=(dn=='.')||(rt=='.'); if(hi&&!lo) return 5; if(lo&&!hi) return 6; return 1;
}
static void gfxLight(int Lr,int Lg,int Lb){
    int night=Lr<150;   // windows: dull glass by day, warm light when it is dark
    BG_PALETTE[PX_WIN]=night?RGB15(31,26,9):RGB15(12*Lr>>8,17*Lg>>8,24*Lb>>8);
    int fr=(256+Lr)/2, fg=(256+Lg)/2, fb=(256+Lb)/2;
    for(int b=0;b<16;b++) for(int k=1;k<8;k++){ u16 c=gfxSprBase[b*16+k]; int r=(c&31)*fr>>8, g=((c>>5)&31)*fg>>8, bl=((c>>10)&31)*fb>>8; if(r>31) r=31; if(g>31) g=31; if(bl>31) bl=31; SPRITE_PALETTE[b*16+k]=RGB15(r,g,bl); }
}
