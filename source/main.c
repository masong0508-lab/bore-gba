// BORE - GBA voxel creature creator (tech demo)
// Build space: 6 wide (X) x 4 long (Z) x 8 high (Y). Mode 3, no libraries.
// EYE and MOUTH are 2D sprites painted straight onto the front (+Z) face of the voxel under the cursor.
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;

#define REG_DISPCNT (*(volatile u16*)0x04000000)
#define REG_VCOUNT  (*(volatile u16*)0x04000006)
#define REG_KEYINPUT (*(volatile u16*)0x04000130)
#define REG_DMA3SAD (*(volatile u32*)0x040000D4)
#define REG_DMA3DAD (*(volatile u32*)0x040000D8)
#define REG_DMA3CNT (*(volatile u32*)0x040000DC)
#define VRAM_ADDR 0x06000000u
#define EWRAM_BSS __attribute__((section(".sbss"), aligned(4)))
// Hot loops run as ARM code from IWRAM (32-bit, zero-wait bus) instead of Thumb from the 16-bit ROM bus.
#define IWRAM_CODE __attribute__((section(".iwram"), target("arm"), long_call))
#define REG_WAITCNT (*(volatile u16*)0x04000204)

#define SW 240
#define SH 160
#define W 6
#define D 4
#define H 8
// Voxel size: change CA (and CC) to resize everything; the starter model, sprites and limbs all scale with them.
#define CA 8    // cube half width (was 10)
#define CB (CA/2)   // cube half height of top face
#define CC 8    // cube side height (was 10)
#define HUG ((CA*3+5)/10)   // limb inset toward the torso (was 3 at CA 10)
#define OX 50
#define OY 96
#define PANEL_X 124

enum { K_A=1, K_B=2, K_SEL=4, K_START=8, K_RIGHT=16, K_LEFT=32, K_UP=64, K_DOWN=128, K_R=256, K_L=512 };

#define RGB(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
static u16 fb[SW*SH] EWRAM_BSS;
#define SFX_MAX 124000   // RAM for the decoded sound effect (also used as the title backdrop before the game starts)
static u8 sfxRam[SFX_MAX] EWRAM_BSS;
#define tfb ((u16*)sfxRam)   // pre-rendered title backdrop (only needed while the title screen shows)

// ---------- palette ----------
static const u16 skinTones[4] = { RGB(30,23,17), RGB(24,16,10), RGB(13,8,5), RGB(14,26,10) };
static const u16 hairTones[5] = { RGB(5,3,2), RGB(27,21,6), RGB(28,8,4), RGB(21,21,22), RGB(10,22,12) };
static u16 base[9], sT[9], sL[9], sR[9];
static u16 dL[4], dR[4];   // face-sprite palette (k w r s) pre-shaded for the left / right cube face
static int skinI = 0, hairI = 0;
#define EDGE RGB(3,2,5)
#define SKY  RGB(20,26,31)
#define PANEL RGB(5,6,9)

static inline __attribute__((always_inline)) u16 shade(u16 c, int n) {
    int r=c&31, g=(c>>5)&31, b=(c>>10)&31;
    return RGB(r*n/16, g*n/16, b*n/16);
}
static void setColors(void) {
    base[1]=skinTones[skinI]; base[2]=RGB(31,31,31); base[3]=RGB(3,3,6);
    base[4]=RGB(29,12,16);    base[5]=hairTones[hairI];
    base[6]=RGB(8,20,22);     base[7]=RGB(8,9,20); base[8]=RGB(31,30,16);
    for (int i=1;i<9;i++){ sT[i]=base[i]; sL[i]=shade(base[i],12); sR[i]=shade(base[i],9); }
    u16 dc[4]={ base[3], base[2], base[4], shade(base[1],11) };   // k dark, w white, r red, s lid shadow
    for (int i=0;i<4;i++){ dL[i]=shade(dc[i],12); dR[i]=shade(dc[i],9); }
}

// ---------- drawing ----------
static inline __attribute__((always_inline)) void px(int x,int y,u16 c){ if((unsigned)x<SW && (unsigned)y<SH) fb[y*SW+x]=c; }
IWRAM_CODE static void vline(int x,int y0,int y1,u16 c){
    if((unsigned)x>=SW) return; if(y0<0)y0=0; if(y1>=SH)y1=SH-1;
    for(;y0<=y1;y0++) fb[y0*SW+x]=c;
}
IWRAM_CODE static void rect(int x,int y,int w,int h,u16 c){ for(int j=0;j<h;j++)for(int i=0;i<w;i++)px(x+i,y+j,c); }
IWRAM_CODE static void line(int x0,int y0,int x1,int y1,u16 c){
    int dx=x1>x0?x1-x0:x0-x1, dy=y1>y0?y0-y1:y1-y0, sx=x0<x1?1:-1, sy=y0<y1?1:-1, e=dx+dy;
    for(;;){ px(x0,y0,c); if(x0==x1&&y0==y1)break; int e2=2*e;
        if(e2>=dy){e+=dy;x0+=sx;} if(e2<=dx){e+=dx;y0+=sy;} }
}

// 3x5 font: A-Z, 0-9, '+', '>'
static const u8 F[39][5] = {
{2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},
{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{6,5,5,5,5},
{2,5,5,5,2},{6,5,6,4,4},{2,5,5,7,3},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},
{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7},
{7,5,5,5,7},{2,6,2,2,7},{6,1,2,4,7},{6,1,6,1,6},{5,5,7,1,1},{7,4,6,1,6},{3,4,7,5,7},{7,1,2,2,2},{7,5,7,5,7},{7,5,7,1,6},
{0,2,7,2,0},{4,6,7,6,4},{2,2,2,0,2} };
IWRAM_CODE static void text(int x,int y,const char*s,u16 c,int sc){
    for(;*s;s++,x+=4*sc){
        int i=-1; char ch=*s;
        if(ch>='A'&&ch<='Z') i=ch-'A'; else if(ch>='0'&&ch<='9') i=26+ch-'0';
        else if(ch=='+') i=36; else if(ch=='>') i=37; else if(ch=='!') i=38;
        if(i<0) continue;
        for(int r=0;r<5;r++)for(int cc=0;cc<3;cc++)
            if((F[i][r]>>(2-cc))&1) rect(x+cc*sc,y+r*sc,sc,sc,c);
    }
}
static void num(int x,int y,int n,u16 c){ char s[2]={(char)('0'+n),0}; text(x,y,s,c,1); }

static inline __attribute__((always_inline)) u16 lite(u16 c,int n){
    int r=(c&31)*n/16, g=((c>>5)&31)*n/16, b=((c>>10)&31)*n/16;
    if(r>31)r=31; if(g>31)g=31; if(b>31)b=31; return RGB(r,g,b);
}
// Soft voxel: tonal outline only on the silhouette (no seams between joined blocks), lit rim, shaded base.
// shape 0 block, 1 slim limb, 2 hand, 3 leg. f: 1 block above, 2 block below, 16/32 coplanar neighbour at left/right edge.
static const u8 rTab[4]={CA,CA*7/10,CA/2,CA*4/5};   // block, slim limb, hand, leg half widths
static u8 hhT[4][CA+1];   // hhT[shape][|t|] = (r/2)*(r-|t|)/r, filled once in initTables (no division in the hot loop)
IWRAM_CODE static void cube(int sx,int sy,int ci,int shape,int f){
    int r=rTab[shape], ch=shape==2?CC*7/10:CC; const u8*hhp=hhT[shape];
    if(shape) f&=3;
    u16 T=sT[ci], L=sL[ci], R=sR[ci], eT=shade(T,9), eL=shade(L,9), eR=shade(R,9);
    for(int t=-r;t<=r;t++){
        int at=t<0?-t:t, hh=hhp[at], x=sx+t, yt=sy+hh, yb=yt+ch-1;
        u16 sc=t<0?L:R, ec=t<0?eL:eR;
        vline(x,yt,yb,sc);
        vline(x,sy-hh,sy+hh,T);
        if(!(f&1)){ px(x,yt+1,lite(sc,19)); px(x,sy-hh,eT); }
        if(!(f&2)){ px(x,yb-1,shade(sc,13)); px(x,yb,ec); }
        if((t==-r&&!(f&16))||(t==r&&!(f&32))) vline(x,sy-hh,yb,ec);
    }
}

// ---------- face sprites ----------
// One cell = 9 x 8 px of art on a cube face. Wider sprites span cells: width = 10*cells-1 (the seam column is art too).
// Palette: k dark, w white, r red, s lid shadow (skin), . clear
#define B9  "........."
#define B19 "..................."
static const char* const aHalf[8] ={ B9, "..sssss..", ".kkkkkkk.", ".wwkkkww.", "..wkkkw..", "...www...", B9, B9 };
static const char* const aRound[8]={ B9, "..kkkkk..", ".kwwwwwk.", ".kwkkkwk.", ".kwkkkwk.", ".kwwwwwk.", "..kkkkk..", B9 };
static const char* const aHappy[8]={ B9, B9, "...kkk...", "..k...k..", ".k.....k.", B9, B9, B9 };
static const char* const mFlat[8] ={ B19, B19, B19, ".....kkkkkkkkk.....", B19, B19, B19, B19 };
static const char* const mSmile[8]={ B19, B19, "...k...........k...", "....k.........k....", ".....kkkkkkkkk.....", B19, B19, B19 };
static const char* const mOh[8]   ={ B19, ".......kkkkk.......", "......krrrrrk......", "......krrrrrk......", ".......kkkkk.......", B19, B19, B19 };
typedef struct { const char*name; u8 wc; const char* const*art; } Spr;   // wc = width in cells
static const Spr spr[6]={ {"HALF",1,aHalf},{"ROUND",1,aRound},{"HAPPY",1,aHappy},
                          {"FLAT",2,mFlat},{"SMILE",2,mSmile},{"OH",2,mOh} };
static int sty[2];                     // chosen style per kind: 0 = eye, 1 = mouth
#define SPRID(k) ((k)*3+sty[k])

// code (0 = none): bits 0-2 sprite+1, 3-5 cell column, 6-7 cell row (from top), 8-9 size-1, 10 mirrored
// face: 0 = left cube face (+Z seen from view 0), 1 = right cube face (+Z seen from view 3)
IWRAM_CODE static void drawDeco(int sx,int sy,u16 code,int face,int tint){
    const Spr*sp=&spr[(code&7)-1];
    int ci=(code>>3)&7, cj=(code>>6)&3, sz=((code>>8)&3)+1, fl=(code>>10)&1, aw=10*sp->wc-1;   // aw = art width in chars
    int wp=CA*sp->wc*sz-1, hp=CC*sz-2;   // footprint size in px (scales with the voxel size)
    const u16*pal=face?dR:dL;
    int lc0=ci?-1:0, lr0=cj?-1:0, axT[CA+1];
    for(int lc=lc0;lc<CA-1;lc++){ int ax=((ci*CA+lc)*aw)/wp; if(ax>aw-1) ax=aw-1; if(fl) ax=aw-1-ax; axT[lc+1]=ax; }
    for(int lr=lr0;lr<CC-2;lr++){
        int ay=((cj*CC+lr)*8)/hp; if(ay>7) ay=7;
        const char*row=sp->art[ay];
        for(int lc=lc0;lc<CA-1;lc++){
            char c=row[axT[lc+1]]; if(c=='.') continue;
            u16 col=tint?(face?sR[8]:sL[8]):pal[c=='k'?0:c=='w'?1:c=='r'?2:3];
            if(!face) px(sx-CA+1+lc, sy+((1+lc)>>1)+1+lr, col);
            else      px(sx+1+lc,    sy+((CA-1-lc)>>1)+1+lr, col);
        }
    }
}

static int cx,cy,cz,part,size;
#define OXC 60
#define OYC 121
static int view=0;   // 0..3 = 90 degree turns
static int noGrid=0;   // sprite baking draws the character without the floor grid
static void rotUW(int u,int w,int*ru,int*rw){
    switch(view){ case 0:*ru=u;*rw=w;break; case 1:*ru=-w;*rw=u;break; case 2:*ru=-u;*rw=-w;break; default:*ru=w;*rw=-u; }
}
// u,w = doubled grid coords relative to the build-space centre
static void projC(int u,int w,int yy,int*ox,int*oy){
    int a,b; rotUW(u,w,&a,&b); *ox=OXC+(a-b)*CA/2; *oy=OYC+(a+b)*CB/2-yy*CC;
}
static void moveView(int sx,int sz){   // screen-relative step -> grid step
    int gx,gz;
    switch(view){ case 0:gx=sx;gz=sz;break; case 1:gx=sz;gz=-sx;break; case 2:gx=-sx;gz=-sz;break; default:gx=-sz;gz=sx; }
    cx+=gx; cz+=gz;
}

static u8 vox[H][D][W], ghost[H][D][W];
static u16 dec[H][D][W], gdec[H][D][W];   // face sprites per voxel (+Z face) and their cursor preview
static int gAny;


// ---------- parts ----------
typedef struct { const char*name; u8 n,mirror,w,h,d; const u8 (*c)[4]; u8 dk; } Part;   // dk: 0 voxel part, 1 eye sprite, 2 mouth sprite
static const u8 cHead[][4]={{0,0,0,1},{1,0,0,1},{0,1,0,1},{1,1,0,1},{0,0,1,1},{1,0,1,1},{0,1,1,1},{1,1,1,1}};
static const u8 cTorso[][4]={{0,0,0,6},{1,0,0,6},{0,1,0,6},{1,1,0,6},{0,0,1,6},{1,0,1,6},{0,1,1,6},{1,1,1,6}};
static const u8 cArm[][4]={{0,0,0,1|(2<<4)},{0,1,0,1|(1<<4)},{0,2,0,6|(1<<4)}};   // hand, forearm, sleeve (high nibble = shape)
static const u8 cLeg[][4]={{0,0,0,7|(3<<4)},{0,1,0,7|(3<<4)},{0,2,0,7|(3<<4)}};
static const u8 cEar[][4]={{0,0,0,1},{0,1,0,1}};
static const u8 cHair[][4]={{0,0,0,5},{1,0,0,5},{0,0,1,5},{1,0,1,5},{0,1,0,5},{1,1,1,5}};
#define NPARTS 8
#define NENT (NPARTS+2)   // part list + "GO LIVE LIFE!" + "EDIT MAP"
static const Part parts[NPARTS]={
 {"HEAD",8,0,2,2,2,cHead,0},{"TORSO",8,0,2,2,2,cTorso,0},{"ARM",3,1,1,3,1,cArm,0},{"LEG",3,1,1,3,1,cLeg,0},
 {"EYE",0,1,1,1,1,0,1},{"MOUTH",0,0,2,1,1,0,2},{"EAR",2,1,1,2,1,cEar,0},{"HAIR",6,0,2,2,2,cHair,0}};

// ---------- state ----------


static void apply(int x0,int y0,int z0,int flip,int act,int pi,int s){
    const Part*p=&parts[pi];
    for(int i=0;i<p->n;i++)
      for(int a=0;a<s;a++)for(int b=0;b<s;b++)for(int c=0;c<s;c++){
        int X=p->c[i][0]*s+a; if(flip) X=p->w*s-1-X;
        int x=x0+X, y=y0+p->c[i][1]*s+b, z=z0+p->c[i][2]*s+c;
        if(x<0||x>=W||y<0||y>=H||z<0||z>=D) continue;
        if(act==0) ghost[y][z][x]=1; else if(act==1) vox[y][z][x]=p->c[i][3]; else { vox[y][z][x]=0; dec[y][z][x]=0; }
      }
}
// Sprites snap to the front-most solid voxel in the cursor's column, so Z does not matter.
static int snapZ(int x,int y){ for(int z=D-1;z>=0;z--) if(vox[y][z][x]) return z; return -1; }
static void applyDeco(int x0,int y0,int act,int pi,int s,int flip){
    int id=SPRID(parts[pi].dk-1), fw=spr[id].wc*s;   // footprint: fw cells wide, s cells tall
    for(int j=0;j<s;j++)for(int i=0;i<fw;i++){
        int x=x0+i, y=y0+s-1-j;
        if(x<0||x>=W||y<0||y>=H) continue;
        if(act==2){ for(int z=0;z<D;z++) dec[y][z][x]=0; continue; }
        int z=snapZ(x,y); if(z<0) continue;
        u16 code=(u16)((id+1)|(i<<3)|(j<<6)|((s-1)<<8)|(flip<<10));
        if(act==0){ gdec[y][z][x]=code; gAny=1; } else dec[y][z][x]=code;
    }
}
static void doDeco(int act,int pi,int s,int x,int y){
    int fw=spr[SPRID(parts[pi].dk-1)].wc*s;
    applyDeco(x,y,act,pi,s,0);
    if(parts[pi].mirror){ int mx=W-x-fw; if(mx!=x) applyDeco(mx,y,act,pi,s,1); }
}
static void doPart(int act,int pi,int s,int x,int y,int z){
    if(parts[pi].dk){ doDeco(act,pi,s,x,y); return; }
    apply(x,y,z,0,act,pi,s);
    if(parts[pi].mirror){ int mx=W-x-parts[pi].w*s; if(mx!=x) apply(mx,y,z,1,act,pi,s); }
}
static void clampCursor(void){
    if(part>=NPARTS) return;   // "GO LIVE LIFE!" entry has no cursor
    const Part*p=&parts[part];
    int mx=W-p->w*size, my=H-p->h*size, mz=D-p->d*size;
    if(mx<0)mx=0; if(my<0)my=0; if(mz<0)mz=0;
    if(cx>mx)cx=mx; if(cy>my)cy=my; if(cz>mz)cz=mz;
    if(cx<0)cx=0; if(cy<0)cy=0; if(cz<0)cz=0;
}
static void starter(void){
    cx=2;cy=0;cz=1;part=0;size=1;
    doPart(1,3,1,2,0,1);  // legs
    doPart(1,1,1,2,3,1);  // torso
    doPart(1,2,1,1,2,1);  // arms (shoulder level with the torso top)
    doPart(1,0,1,2,5,1);  // head
    doPart(1,4,1,2,6,0);  // eyes (sprites on the head's front face)
    doPart(1,5,1,2,5,0);  // mouth
    doPart(1,7,1,2,7,1);  // hair
}

// ---------- scene ----------
static int solid(int x,int y,int z){ return x>=0&&x<W&&y>=0&&y<H&&z>=0&&z<D&&vox[y][z][x]; }
static const signed char dA[4][2]={{1,0},{0,-1},{-1,0},{0,1}}, dB[4][2]={{0,1},{1,0},{0,-1},{-1,0}};   // screen +a / +b in grid x,z per view
// The screen is two independent regions: the 3D scene (columns 0..PANEL_X-1) and the side panel.
// Both are cleared with 32-bit stores, and the panel is only redrawn when something it shows changed.
#define SCENE_W (PANEL_X/2)
#define ROW_W   (SW/2)
IWRAM_CODE static void fillCols(int w0,int w1,u16 c){
    u32 v=c|((u32)c<<16), *row=(u32*)fb;
    for(int y=0;y<SH;y++,row+=ROW_W) for(int w=w0;w<w1;w++) row[w]=v;
}
static u8 ord[4][W*D];   // per view: cells (x | z<<4) sorted back to front, so the draw loop needs no search
static void initTables(void){
    for(int sh=0;sh<4;sh++){ int r=rTab[sh]; for(int at=0;at<=r;at++) hhT[sh][at]=(u8)((r/2)*(r-at)/r); }
    int sv=view;
    for(int v=0;v<4;v++){
        view=v; int key[W*D], n=0;
        for(int z=0;z<D;z++)for(int x=0;x<W;x++){
            int ru,rw; rotUW(2*x+1-W,2*z+1-D,&ru,&rw);
            int k=((ru+rw+8)<<8)|(z<<4)|x, j=n++;
            while(j>0&&key[j-1]>k){ key[j]=key[j-1]; j--; }
            key[j]=k;
        }
        for(int i=0;i<n;i++) ord[v][i]=(u8)(key[i]&0xFF);
    }
    view=sv;
}
IWRAM_CODE static void drawPanel(void){
    fillCols(SCENE_W,ROW_W,PANEL);
    text(130,5,"BORE",RGB(31,26,6),2);
    text(130,17,"VOXEL DEMO",RGB(14,16,18),1);
    for(int i=0;i<NENT;i++){
        int y=28+i*7, go=(i>=NPARTS);
        if(i==part){ rect(128,y-1,108,6,go?RGB(16,10,2):RGB(6,16,8)); text(130,y,">",RGB(31,31,31),1); }
        text(137,y,go?(i==NPARTS?"GO LIVE LIFE!":"EDIT MAP"):parts[i].name,i==part?(go?RGB(31,26,6):RGB(31,31,31)):(go?RGB(24,20,6):RGB(18,20,22)),1);
        if(i==part&&!go&&parts[i].dk) text(190,y,spr[SPRID(parts[i].dk-1)].name,RGB(31,26,6),1);
    }
    text(130,101,"SIZE",RGB(18,20,22),1);
    const char*sn[3]={"S","M","L"};
    for(int i=0;i<3;i++){
        int x=156+i*16; rect(x,99,12,9,i==size-1?RGB(6,16,8):RGB(2,3,5));
        text(x+4,101,sn[i],RGB(31,31,31),1);
    }
    text(130,113,"X",RGB(18,20,22),1); num(136,113,cx,RGB(31,31,31));
    text(148,113,"Y",RGB(18,20,22),1); num(154,113,cy,RGB(31,31,31));
    text(166,113,"Z",RGB(18,20,22),1); num(172,113,cz,(part<NPARTS&&parts[part].dk)?RGB(12,14,16):RGB(31,31,31));   // sprites ignore Z
    u16 hc=RGB(12,14,16);
    if(part>=NPARTS){ text(130,123,"PRESS A TO OPEN",RGB(31,26,6),1); text(130,131,"SELECT PICKS NEXT ENTRY",hc,1); }
    else {
    text(130,123,"DPAD X Z  L R HEIGHT",hc,1); text(130,129,"A PLACE B ERASE",hc,1);
    text(130,135,"START SIZE  SEL TAP PART",hc,1); text(130,141,"SEL+UP DOWN FACE STYLE",hc,1);
    text(130,147,"SEL+A SKIN SEL+B HAIR",hc,1); text(130,153,"SEL+LEFT RIGHT TURN VIEW",hc,1);
    }
}
IWRAM_CODE static void drawScene(int blink,int full){
    fillCols(0,SCENE_W,SKY);
    // floor grid
    u16 gc=RGB(13,18,22); int a,b,c,d;
    if(!noGrid) for(int i=0;i<=W;i++){ projC(2*i-W,-D,0,&a,&b); projC(2*i-W,D,0,&c,&d); line(a,b,c,d,gc); }
    if(!noGrid) for(int j=0;j<=D;j++){ projC(-W,2*j-D,0,&a,&b); projC(W,2*j-D,0,&c,&d); line(a,b,c,d,gc); }
    int fv=view==0?0:view==3?1:-1;   // which cube face shows the +Z (front) face, -1 = turned away
    // voxels (back to front)
    for(int y=0;y<H;y++)for(int i=0;i<W*D;i++){
        int x=ord[view][i]&15, z=ord[view][i]>>4;
        int raw=vox[y][z][x], ci=raw&15, shape=raw>>4;
        if(ghost[y][z][x]&&blink) ci=8;
        if(gdec[y][z][x]&&blink&&fv<0) ci=8;   // face turned away: flag the target voxel instead
        if(!ci) continue;
        int u=2*x+1-W, w=2*z+1-D;
        int sx,sy; projC(u,w,y+1,&sx,&sy);   // top-face centre
        if(shape==1||shape==2){ int sg=u<0?1:-1, a2,b2; rotUW(sg,0,&a2,&b2); sx+=HUG*(a2-b2); sy+=(HUG*(a2+b2))/2; }   // hug the torso
        int f=(solid(x,y+1,z)?1:0)|(solid(x,y-1,z)?2:0)
             |(solid(x-dA[view][0],y,z-dA[view][1])?16:0)|(solid(x-dB[view][0],y,z-dB[view][1])?32:0);
        cube(sx,sy,ci,shape,f);
        u16 dc=dec[y][z][x]; int tint=0;
        if(gdec[y][z][x]&&blink){ dc=gdec[y][z][x]; tint=1; }
        if(dc&&fv>=0) drawDeco(sx,sy,dc,fv,tint);
    }
    if(full) drawPanel();
}
static void vsync(void){ while(REG_VCOUNT>=160); while(REG_VCOUNT<160); }
static void present(void){
    vsync();
    REG_DMA3SAD=(u32)(uintptr_t)fb; REG_DMA3DAD=VRAM_ADDR;
    REG_DMA3CNT=(SW*SH/2)|0x84000000u;
}

// Copy only the scene columns (blink-only redraws leave the panel untouched).
static void presentScene(void){
    vsync();
    for(int y=0;y<SH;y++){
        REG_DMA3SAD=(u32)(uintptr_t)(fb+y*SW); REG_DMA3DAD=VRAM_ADDR+(u32)(y*SW*2); REG_DMA3CNT=SCENE_W|0x84000000u;
    }
}
// Row-by-row DMA of a rectangle (32-bit columns w0..w1-1, rows y0..y1-1): src buffer -> dst base address.
static void dmaRows(const u16*src,u32 dst,int w0,int w1,int y0,int y1){
    for(int y=y0;y<y1;y++){
        int o=y*SW+w0*2;
        REG_DMA3SAD=(u32)(uintptr_t)(src+o); REG_DMA3DAD=dst+(u32)(o*2); REG_DMA3CNT=(u32)(w1-w0)|0x84000000u;
    }
}

// ---------- title screen ----------
#include "titleimg.h"
static void buildTitle(void){
    for(int y=0;y<80;y++)for(int x=0;x<120;x++){
        char c=titleArt[y][x]; u16 col=titlePal[c<='9'?c-'0':c-'a'+10];
        u16*o=&fb[(y*2)*SW+x*2]; o[0]=o[1]=o[SW]=o[SW+1]=col;
    }
    for(int y=118;y<SH;y++)for(int x=0;x<SW;x++){ u16 c=fb[y*SW+x]; fb[y*SW+x]=shade(c,7); }   // dim strip for the prompt
    u16 ink=RGB(4,3,6), gold=RGB(31,27,6), grn=RGB(12,28,8);
    for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++) text(10+dx,10+dy,"BORE",ink,5);   // outline
    text(10,10,"BORE",gold,5);
    text(12,40,"A VOXEL LIFE SIM",ink,1); text(11,39,"A VOXEL LIFE SIM",grn,1);
    text(14,126,"PUFF PUFF PASS THE CONTROLLER",RGB(16,22,12),1);
    for(int i=0;i<SW*SH;i++) tfb[i]=fb[i];
}
static void smoke(int frame){
    static const signed char wob[16]={0,1,2,3,3,3,2,1,0,-1,-2,-3,-3,-3,-2,-1};
    for(int i=0;i<9;i++){
        int t=(frame+i*14)&127;
        int x=164+wob[(t/4+i*5)&15]+t/5, y=82-t*7/8, r=1+t/24;
        for(int dy=-r;dy<=r;dy++)for(int dx=-r;dx<=r;dx++){
            if(dx*dx+dy*dy>r*r+1) continue;
            if(t>64&&((x+dx+y+dy)&1)) continue;       // fade out by dithering
            if(t>100&&((x+dx)&1)) continue;
            px(x+dx,y+dy,RGB(27,28,29));
        }
    }
}
#define SM_W0 76   // smoke stays inside columns 152..203, rows 0..89 (checked over its whole 128-frame loop)
#define SM_W1 102
#define SM_Y1 90
#define TX_W0 47    // "PRESS START" box
#define TX_W1 70
#define TX_Y0 141
#define TX_Y1 147
static void titleScreen(void){
    buildTitle();                          // leaves the finished backdrop in both fb and tfb
    vsync(); dmaRows(fb,VRAM_ADDR,0,ROW_W,0,SH);
    int shown=0;
    for(int frame=0;;frame++){
        if((~REG_KEYINPUT)&K_START) break;
        dmaRows(tfb,(u32)(uintptr_t)fb,SM_W0,SM_W1,0,SM_Y1);   // wipe last frame's smoke only
        smoke(frame);
        int on=(frame>>4)&1, tx=(on!=shown);
        if(tx){ dmaRows(tfb,(u32)(uintptr_t)fb,TX_W0,TX_W1,TX_Y0,TX_Y1); if(on) text(94,141,"PRESS START",RGB(31,31,31),1); shown=on; }
        vsync();
        dmaRows(fb,VRAM_ADDR,SM_W0,SM_W1,0,SM_Y1);
        if(tx) dmaRows(fb,VRAM_ADDR,TX_W0,TX_W1,TX_Y0,TX_Y1);
    }
    while((~REG_KEYINPUT)&K_START) vsync();   // wait for release so START doesn't also change size
}

// ---------- LIFE MODE: fixed isometric "sim" room + Tony-Hawk-style skating (placeholder) ----------
// Pick "GO LIVE LIFE!" in the part list and press A. SELECT+START returns to the editor.
// Controls: D-pad L/R steer (grounded) or spin (airborne) | hold A push | D-pad down brake | B ollie / kickflip in air
// Land spins in half-turns (180/360) for points, a bad angle is a bail. Land on a yellow rail to grind it.
typedef int32_t s32;
static void lifeMode(int ed);
static void mapEditor(void);
#define MW 14
#define MH 14
#define LOX 120   // screen x of the map's top corner
#define LOY 24
#define SPW 32   // baked at half size so the skater is ~2 tiles tall in the room
#define SPH 44
#define SPX0 (OXC-32)
#define SPY0 (OYC-80)   // capture window top; feet sit at row 40 of the half-size sprite
#define MAPNAME "THE MAN BASE"   // name of the (placeholder) map
// w = low wall, # = 2-block crate, = = grind rail, . = floor
static const char* const mapDef[MH]={   // default room
"wwwwwwwwwwwwww","w...........Fw","w.....====..Fw","w............w","w..##.....B..w","w..##........w","w..P.........w",
"w.......##...w","w.......##...w","w.====.......w","w............D","w............D","w..........TTw","wwwwwwwwwwwwww" };
static const short cosT[16]={256,237,181,98,0,-98,-181,-237,-256,-237,-181,-98,0,98,181,237};   // sin(a)=cosT[(a+12)&15]
static u16 spr4[4][SPW*SPH] EWRAM_BSS;
static s32 lfx,lfy,lz,lvz,lvx,lvy; static int lskate, lhave, lfr;   // lskate: 0 on foot, 1 skateboard; lhave: picked up the board
static char lifeMap[MH][MW+1];   // the room being played / edited (starts as mapDef, or the copy saved in SRAM)
static int bdx=10, bdy=4, spx=3, spy=6;   // skateboard tile and spawn tile, found by mapScan (B and P tiles)
#define BDX bdx
#define BDY bdy
static int lsp,lhd,lspin,lflip,lgrind,lscore,lstun,lairF,lpts,lnoteT; static const char*lnote;

static int lfood, lbl, lnear;   // hunger (100 = full), bladder (100 = bursting), what is in reach (1 fridge, 2 toilet)
static int lmaxz, lplay, ldead, lbumpCd;   // peak height this jump, air sound played, dead, bump cooldown

// ---------- sound effects: 4-bit IMA-ADPCM @ 6554 Hz, decoded on the fly into RAM, played by Direct Sound A (DMA1 + Timer0) ----------
// source/sfx/*.adp (made by tools/encode_sfx.py) are baked into the ROM with .incbin; paths are relative to the project root.
// Timer1 counts Timer0 overflows = samples played, so a clip stops exactly at its end whatever the frame rate is.
#define R_SNDCNT_L (*(volatile u16*)0x04000080)
#define R_SNDCNT_H (*(volatile u16*)0x04000082)
#define R_SNDCNT_X (*(volatile u16*)0x04000084)
#define R_DMA1SAD (*(volatile u32*)0x040000BC)
#define R_DMA1DAD (*(volatile u32*)0x040000C0)
#define R_DMA1CNT (*(volatile u32*)0x040000C4)
#define R_TM0D    (*(volatile u16*)0x04000100)
#define R_TM0CNT  (*(volatile u16*)0x04000102)
#define R_TM1D    (*(volatile u16*)0x04000104)
#define R_TM1CNT  (*(volatile u16*)0x04000106)
#define SFX_TIMER (65536-2560)   // 16777216/2560 = 6553.6 Hz
__asm__(".pushsection .rodata\n.balign 4\n"
 ".global sfx_hit\nsfx_hit:\n.incbin \"source/sfx/hit.adp\"\n.balign 4\n"
 ".global sfx_gasp\nsfx_gasp:\n.incbin \"source/sfx/gasp.adp\"\n.balign 4\n"
 ".global sfx_scream\nsfx_scream:\n.incbin \"source/sfx/scream.adp\"\n.balign 4\n"
 ".global sfx_cry\nsfx_cry:\n.incbin \"source/sfx/cry.adp\"\n.balign 4\n"
 ".global sfx_groan\nsfx_groan:\n.incbin \"source/sfx/groan.adp\"\n.balign 4\n"
 ".global sfx_instant\nsfx_instant:\n.incbin \"source/sfx/instant.adp\"\n.balign 4\n"
 ".popsection\n");
extern const u8 sfx_hit[],sfx_gasp[],sfx_scream[],sfx_cry[],sfx_groan[],sfx_instant[];
enum { SFX_BONK, SFX_HIT, SFX_GASP, SFX_SCREAM, SFX_CRY, SFX_GROAN, SFX_NEARLY, SFX_DEATH, SFX_INSTANT, SFX_N };
// effects that share a source file share one blob in the ROM
static const u8* const sfxTab[SFX_N]={ sfx_hit,sfx_hit,sfx_gasp,sfx_scream,sfx_cry,sfx_groan,sfx_scream,sfx_scream,sfx_instant };
static const u16 stepT[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const signed char idxT[8]={-1,-1,-1,-1,2,4,6,8};
static const u8 *ssrc; static u32 sn, sdone, swraps; static int spred, sidx, sfxOn; static u16 slast;
static void sfxStop(void){ R_DMA1CNT=0; R_TM0CNT=0; R_TM1CNT=0; sfxOn=0; }
IWRAM_CODE static void sfxDecode(int cnt){   // decode the next cnt samples into sfxRam (signed 8-bit)
    u32 i=sdone, e=sdone+(u32)cnt; if(e>sn) e=sn;
    int pred=spred, idx=sidx; signed char*out=(signed char*)sfxRam;
    for(;i<e;i++){
        int v=ssrc[i>>1]; v=(i&1)?(v>>4):(v&15);
        int step=stepT[idx], diff=step>>3;
        if(v&1) diff+=step>>2; if(v&2) diff+=step>>1; if(v&4) diff+=step;
        pred+=(v&8)?-diff:diff; if(pred>32767) pred=32767; if(pred<-32768) pred=-32768;
        idx+=idxT[v&7]; if(idx<0) idx=0; if(idx>88) idx=88;
        out[i]=(signed char)(pred>>8);
    }
    spred=pred; sidx=idx; sdone=e;
    if(sdone>=sn) for(int k=0;k<256;k++) out[sn+k]=0;   // silence after the end, so the DMA read-ahead plays nothing
}
static void sfxPlay(int id){   // a new sound replaces whatever is playing
    sfxStop();
    const u8*b=sfxTab[id]; sn=*(const u32*)b; if(sn>SFX_MAX-256) sn=SFX_MAX-256;
    ssrc=b+4; sdone=0; spred=0; sidx=0;
    sfxDecode(1024);                             // a head start; sfxTick decodes the rest while it plays
    R_SNDCNT_X=0x80; R_SNDCNT_L=0;
    R_SNDCNT_H=0x0B04;                           // Direct Sound A: 100% vol, L+R, Timer0, reset FIFO
    R_DMA1SAD=(u32)(uintptr_t)sfxRam; R_DMA1DAD=0x040000A0u;
    R_DMA1CNT=0xB6400000u;                       // enable, FIFO timing, repeat, 32-bit, fixed dest
    R_TM0D=SFX_TIMER; R_TM0CNT=0x80;
    swraps=0; slast=0; R_TM1D=0; R_TM1CNT=0x84;  // Timer1 counts Timer0 overflows (samples played)
    sfxOn=1;
}
static void sfxTick(void){   // call once per frame
    if(!sfxOn) return;
    u16 t=R_TM1D; if(t<slast) swraps++; slast=t;
    if(sdone<sn) sfxDecode(512);
    if(swraps*65536u+t>=sn) sfxStop();
}
static u32 lrng=12345;
static int rnd8(void){ lrng=lrng*1664525u+1013904223u; return (int)(lrng>>24); }

// Getting hurt. sev grows with fall height, speed and a bad landing. kind: 0 clean landing, 1 bail, 2 wall hit.
static void die(int snd){ ldead=1; lstun=2; lsp=0; lgrind=0; sfxPlay(snd); lnote="YOU DIED"; lnoteT=0x7fff; }
static void hurt(int sev,int kind){
    if(sev>=40) die(SFX_INSTANT);                                                                   // instant death
    else if(sev>=30){                                                                                // life or death
        if(rnd8()<128){ lstun=240; lsp=0; lgrind=0; sfxPlay(SFX_NEARLY); lnote="CLOSE CALL"; lnoteT=120; }
        else die(SFX_DEATH);
    }
    else if(sev>=18){ lstun=150; lsp=0; lgrind=0; sfxPlay(SFX_GROAN); lnote="OW"; lnoteT=90; }     // groaning, struggling up
    else if(kind==1){ lstun=60; sfxPlay(SFX_CRY); }                                                  // minor bail: crying
    else if(kind==2){ lstun=20; sfxPlay(SFX_HIT); lnote="OOF"; lnoteT=30; }                          // grunts and hits
}

static int tileH(int tx,int ty){   // surface height in px
    if(tx<0||ty<0||tx>=MW||ty>=MH) return 99;
    char c=lifeMap[ty][tx];
    return (c=='#'||c=='F')?2*CC: (c=='w'||c=='T')?CC: c=='='?6:0;
}
static void bakeSprites(void){   // render the built character once per view (4 turns), then just blit it
    int sv=view; noGrid=1;
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ ghost[y][z][x]=0; gdec[y][z][x]=0; }
    for(int v=0;v<4;v++){
        view=v; drawScene(0,0);
        for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++) spr4[v][y*SPW+x]=fb[(SPY0+y*2)*SW+SPX0+x*2];
    }
    noGrid=0; view=sv;
}
IWRAM_CODE static void blit(const u16*s,int x0,int y0){
    for(int y=0;y<SPH;y++){ int yy=y0+y; if((unsigned)yy>=SH) continue;
        for(int x=0;x<SPW;x++){ u16 c=s[y*SPW+x]; if(c!=SKY) px(x0+x,yy,c); } }
}
static void numText(int x,int y,int n,u16 c){
    char b[10]; int i=9; b[i]=0; if(n<=0) b[--i]='0';
    while(n>0&&i>0){ int q=n/10; b[--i]=(char)('0'+n-q*10); n=q; }
    text(x,y,b+i,c,1);
}
// ---------- map data: reset / scan / save ----------
static const char palCh[9]={'.','w','#','=','F','T','D','B','P'};
static const char* const palNm[9]={"FLOOR","WALL","CRATE","RAIL","FRIDGE","TOILET","DOOR","BOARD","SPAWN"};
static const u16 palCol[9]={RGB(26,21,14),RGB(8,20,22),RGB(8,9,20),RGB(31,30,16),RGB(31,31,31),RGB(30,28,18),RGB(14,9,5),RGB(26,10,6),RGB(28,10,8)};
static int palIdx(char c){ for(int i=0;i<9;i++) if(palCh[i]==c) return i; return -1; }
static void mapReset(void){ for(int y=0;y<MH;y++){ for(int x=0;x<MW;x++) lifeMap[y][x]=mapDef[y][x]; lifeMap[y][MW]=0; } }
static void mapScan(void){   // find the skateboard (B) and the spawn point (P); fall back to sane defaults
    int fx=-1, fy=-1; bdx=bdy=spx=spy=-1;
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){ char c=lifeMap[y][x];
        if(c=='B'){ bdx=x; bdy=y; } if(c=='P'){ spx=x; spy=y; }
        if(fx<0&&c=='.'){ fx=x; fy=y; } }
    if(spx<0){ if(fx<0){ lifeMap[1][1]='P'; fx=fy=1; } spx=fx; spy=fy; }
}
#define SRAM_BASE ((volatile u8*)0x0E000000)
static const char sramTag[] __attribute__((used)) = "SRAM_V113";   // tells emulators / flash carts to give the game battery saves
static void mapSave(void){ volatile u8*m=SRAM_BASE; m[0]='B'; m[1]='M'; m[2]='1'; for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) m[3+y*MW+x]=(u8)lifeMap[y][x]; }
static int mapSaved(void){ volatile u8*m=SRAM_BASE; if(m[0]!='B'||m[1]!='M'||m[2]!='1') return 0;
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) if(m[3+y*MW+x]!=(u8)lifeMap[y][x]) return 0; return 1; }
static int mapLoad(void){   // returns 1 if a valid saved map was loaded
    volatile u8*m=SRAM_BASE; if(m[0]!='B'||m[1]!='M'||m[2]!='1') return 0;
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) if(palIdx((char)m[3+y*MW+x])<0) return 0;
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) lifeMap[y][x]=(char)m[3+y*MW+x];
    return 1; }
static void mapPlace(int x,int y,char c){
    if(c=='B'||c=='P'){ for(int j=0;j<MH;j++)for(int i=0;i<MW;i++) if(lifeMap[j][i]==c) lifeMap[j][i]='.'; }
    lifeMap[y][x]=c; }

// ---------- small UI kit: one menu style, one help style, one toast ----------
#define GOLD RGB(31,26,6)
#define DIMC RGB(18,20,22)
#define WHITE RGB(31,31,31)
static u16 keyNow(void){ return (u16)(~REG_KEYINPUT)&0x3FF; }
static void box(int x,int y,int w,int h){ rect(x-1,y-1,w+2,h+2,GOLD); rect(x,y,w,h,RGB(3,4,7)); }
static int menu(const char*title,const char*const*it,int n){   // UP/DOWN + A to choose, B or START to cancel (returns -1)
    int sel=0, w=116, h=26+n*10, x=(SW-w)/2, y=(SH-h)/2; u16 prev=keyNow();
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&K_DOWN) sel=(sel+1)%n;
        if(pr&K_UP) sel=(sel+n-1)%n;
        if(pr&K_A) return sel;
        if(pr&(K_B|K_START)) return -1;
        box(x,y,w,h); text(x+6,y+5,title,GOLD,1);
        for(int i=0;i<n;i++){ int yy=y+16+i*10;
            if(i==sel){ rect(x+3,yy-2,w-6,9,RGB(6,16,8)); text(x+6,yy,">",WHITE,1); }
            text(x+13,yy,it[i],i==sel?WHITE:DIMC,1); }
        text(x+6,y+h-9,"A OK  B BACK",RGB(12,14,16),1);
        present();
    }
}
static void helpScreen(const char*title,const char*const*ln,int n){   // lines starting with > are headings
    u16 prev=keyNow();
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&(K_A|K_B|K_START)) return;
        box(3,1,234,157); text(14,10,title,GOLD,1);
        for(int i=0;i<n;i++){ const char*l=ln[i]; if(l[0]=='>') text(14,22+i*8,l+1,GOLD,1); else text(18,22+i*8,l,WHITE,1); }
        text(14,144,"PRESS A TO CLOSE",DIMC,1);
        present();
    }
}
static void toast(const char*msg){ int w=(int)(4*0); const char*p=msg; while(*p){ w+=4; p++; } w+=16;
    box((SW-w)/2,66,w,22); text((SW-w)/2+8,74,msg,WHITE,1); for(int i=0;i<45;i++){ present(); } }
static const char* const lifeHelp[12]={">ON FOOT","DPAD WALK  B RUN  A HOP","R FRIDGE OR TOILET","L GET ON THE BOARD",">ON THE BOARD","A PUSH  DPAD STEER  B OLLIE","IN AIR DPAD SPINS  B KICKFLIP","LAND CLEAN FOR POINTS","HIGH FALLS AND WALLS HURT",">KEEP YOURSELF GOING","WATCH THE FOOD AND WC BARS","START OPENS THE MENU"};
static const char* const mapHelp[12]={">PAINT YOUR ROOM","DPAD MOVE THE CURSOR","A PLACE  B ERASE","HOLD A OR B AND MOVE TO PAINT","L R CHANGE TILE","SELECT PICKS THE TILE UNDER IT",">SPECIAL TILES","SPAWN TILE IS WHERE YOU START","BOARD TILE IS THE SKATEBOARD","DOOR IS A PLACEHOLDER ENTRANCE","FRIDGE EATS  TOILET RELIEVES",">START OPENS PLAY AND SAVE"};

static void lifeInit(void){
    mapScan(); bakeSprites();
    lfx=spx*256+128; lfy=spy*256+128; lz=lvz=0; lsp=0; lhd=0; lspin=0; lflip=0; lgrind=0; lscore=0; lstun=0; lairF=0; lpts=0; lnoteT=0; lnote=""; lskate=0; lhave=(bdx<0); lfr=0; lvx=lvy=0; ldead=0; lmaxz=0; lplay=0; lbumpCd=0; lfood=100; lbl=0; lnear=0; sfxStop();
}
static const signed char hdT[3][3]={{10,12,14},{8,-1,0},{6,4,2}};   // [sign dy+1][sign dx+1] -> heading (16 steps), -1 = keep
static void lifeStep(u16 k,u16 pr,int fr){
    int fh=tileH(lfx>>8,lfy>>8)<<8;
    if(ldead){   // dead: frozen until A
        lstun=2;
        if(pr&K_A){ ldead=0; lstun=0; lfx=spx*256+128; lfy=spy*256+128; lz=0; lvz=0; lskate=0; lsp=0; lgrind=0; lspin=0; lflip=0; lairF=0; lmaxz=0; lplay=0; lnoteT=0; lfood=100; lbl=0; sfxStop(); }
    }
    if(lstun>0){ lstun--; lsp=0; lvx=lvy=0; }
    else {
        if((pr&K_L)&&!lhave){ lnote="FIND A BOARD"; lnoteT=40; }
        if((pr&K_L)&&lhave&&lz<=fh){   // L: swap between on-foot (walk/run) and skateboard
            lskate=!lskate; lsp=0; lgrind=0; lspin=0; lflip=0; lnote=lskate?"SKATE":"ON FOOT"; lnoteT=40;
        }
        if(lskate){
            if(lz<=fh){                                        // on the ground (or on a rail)
                if((fr&3)==0){ if(k&K_LEFT) lhd=(lhd+15)&15; if(k&K_RIGHT) lhd=(lhd+1)&15; }
                if(k&K_A){ if((fr&3)==0&&lsp<24) lsp++; } else if(lsp>0&&(fr&7)==0) lsp--;   // push / coast
                if((k&K_DOWN)&&lsp>0&&(fr&1)==0) lsp--;                                       // brake
                if(lgrind&&lsp<12) lsp=12;                                                     // rails keep you rolling
                if(pr&K_B){ lvz=0x380; lgrind=0; }                                             // ollie
            } else {                                                                           // airborne
                if((fr&3)==0){ if(k&K_LEFT) lspin--; if(k&K_RIGHT) lspin++; }                  // spin: 16 steps = 360 deg
                if((pr&K_B)&&!lflip){ lflip=1; lnote="KICKFLIP"; lnoteT=40; }
            }
            lvx=(lsp*cosT[lhd])/256; lvy=(lsp*cosT[(lhd+12)&15])/256;
        } else {
            // on foot: D-pad moves relative to the screen (up = away from camera), B held = run, A = hop
            int ux=((k&K_RIGHT)?1:0)-((k&K_LEFT)?1:0), uy=((k&K_DOWN)?1:0)-((k&K_UP)?1:0);
            int dx=ux+uy, dy=uy-ux, spd=(k&K_B)?10:5;
            if(ux&&uy) spd=(spd*3)/4;                          // diagonals cover the same ground
            lvx=dx*spd; lvy=dy*spd; lsp=(dx||dy)?spd:0;
            if(dx||dy){ int h=hdT[(dy>0)-(dy<0)+1][(dx>0)-(dx<0)+1]; if(h>=0) lhd=h; }
            if((pr&K_A)&&lz<=fh) lvz=0x300;
        }
    }
    int zp=(int)(lz>>8);
    s32 nx=lfx+lvx, ny=lfy+lvy;   // move per axis so walls slide
    int bump=0, sp0b=lsp;
    if(tileH(nx>>8,lfy>>8)<=zp+3) lfx=nx; else bump=1;
    if(tileH(lfx>>8,ny>>8)<=zp+3) lfy=ny; else bump=1;
    if(bump){
        lsp=(lsp*2)/3;
        if(lbumpCd==0&&sp0b>=(lskate?12:10)){ lbumpCd=40; if(lskate) hurt(sp0b+(rnd8()>>4),2); else sfxPlay(SFX_BONK); }   // skating into a wall hurts, running into one bonks
    }
    if(lbumpCd>0) lbumpCd--;
    fh=tileH(lfx>>8,lfy>>8)<<8;
    if(lz<fh){ lz=fh; if(lvz<0) lvz=0; }
    if(lz>fh||lvz>0){ lz+=lvz; lvz-=0x40; if(lz<=fh&&lvz<=0){ lz=fh; lvz=0; } }   // gravity
    int air=lz>fh;
    if(air){
        int zz=(int)(lz>>8); if(zz>lmaxz) lmaxz=zz;
        if(!lplay&&lvz<0){ int hi=lmaxz-(int)(fh>>8);
            if(hi>=34){ sfxPlay(SFX_SCREAM); lplay=1; }                 // falling from way up
            else if((lspin&7)&&hi>=10){ sfxPlay(SFX_GASP); lplay=1; }   // landing is going wrong
        }
    }
    if(lairF&&!air){                                   // just landed
        int a=lspin<0?-lspin:lspin, pts=(a>>3)*180+(lflip?100:0);
        int drop=lmaxz-(int)(lz>>8), sp0=lsp, bail=(lspin&7)!=0;
        if(bail){ lnote="BAIL"; lnoteT=60; lsp=0; lstun=45; lgrind=0; }
        else{
            if(pts){ lscore+=pts; lpts=pts; lnote="NICE"; lnoteT=60; }
            if(lskate&&tileH(lfx>>8,lfy>>8)==6){ lgrind=1; lnote="GRIND"; lnoteT=30; }
        }
        if(bail) hurt(drop/2+sp0+(rnd8()>>5),1);        // bad landing: harder/faster/higher = worse
        else if(drop>24) hurt(drop-24+(rnd8()>>5),0);   // big drops hurt even landed clean
        lspin=0; lflip=0;
    }
    if(!air){ lmaxz=(int)(lz>>8); lplay=0; }
    lairF=air;
    if(lgrind){ if(air||tileH(lfx>>8,lfy>>8)!=6) lgrind=0; else if((fr&3)==0){ lscore+=3; lnote="GRIND"; lnoteT=10; } }
    if(!lhave&&lz<(8<<8)&&(lfx>>8)==BDX&&(lfy>>8)==BDY){ lhave=1; lnote="GOT A SKATEBOARD"; lnoteT=90; }   // walk over it to pick it up
    if(!ldead){   // needs: hunger and bladder
        if(lfr%120==0&&lfood>0) lfood--;
        if(lfr%100==0&&lbl<100) lbl++;
        if(lfood==0&&lfr%300==0){ lfood=15; lstun=120; lsp=0; lgrind=0; sfxPlay(SFX_GROAN); lnote="FAINTED FROM HUNGER"; lnoteT=90; }
        if(lbl>=100){ lbl=0; lstun=90; lsp=0; lgrind=0; lscore=lscore>100?lscore-100:0; sfxPlay(SFX_CRY); lnote="ACCIDENT"; lnoteT=90; }
        int nf=0, nt=0;
        for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){ int tx=(lfx>>8)+dx, ty=(lfy>>8)+dy; if(tx<0||ty<0||tx>=MW||ty>=MH) continue;
            char c=lifeMap[ty][tx]; if(c=='F') nf=1; if(c=='T') nt=1; }
        lnear=nf?1:(nt?2:0);
        if((pr&K_R)&&lnear&&lstun<=0&&lz<=fh){
            if(lnear==1){   // fridge: eat
                if(lfood>=95){ lnote="FULL"; lnoteT=40; }
                else { lfood+=35; if(lfood>100) lfood=100; lbl+=10; if(lbl>99) lbl=99; lstun=30; lsp=0; lnote="YUM"; lnoteT=50; }
            } else {        // toilet: relieve yourself
                if(lbl<15){ lnote="LATER"; lnoteT=40; }
                else { lbl=0; lstun=70; lsp=0; lgrind=0; lnote="AHH"; lnoteT=60; }
            }
        }
    }
    lfr++;
    if(lnoteT>0) lnoteT--;
    sfxTick();
}
static void tileTop(int sx,int sy,u16 c){ for(int t=-CA;t<=CA;t++){ int at=t<0?-t:t, hh=hhT[0][at]; vline(sx+t,sy-hh,sy+hh,c); } }
static int ecx=6, ecy=6, efr;   // map editor cursor (tile) and frame counter
static void drawRoom(int ed){   // the room, drawn back to front; ed=1: editor view (no player, markers + cursor)
    fillCols(0,ROW_W,RGB(4,5,8));
    u16 cA=RGB(26,21,14), cB=RGB(23,18,11);
    for(int ty=0;ty<MH;ty++)for(int tx=0;tx<MW;tx++){ char c=lifeMap[ty][tx]; if(c=='w'||c=='#'||c=='F'||c=='T') continue;
        tileTop(LOX+(tx-ty)*CA,LOY+(tx+ty+1)*CB,c=='D'?RGB(14,9,5):(((tx^ty)&1)?cA:cB)); }
    int ss=(int)((lfx>>8)+(lfy>>8)), psx=LOX+(int)((lfx-lfy)>>5), psy=LOY+(int)((lfx+lfy)>>6);
    for(int s=0;s<MW+MH-1;s++){
        for(int tx=0;tx<MW;tx++){ int ty=s-tx; if(ty<0||ty>=MH) continue;
            char c=lifeMap[ty][tx]; int sx=LOX+(tx-ty)*CA, sy=LOY+(tx+ty+1)*CB;
            if(c=='w'||c=='#'){ int h=c=='#'?2:1; for(int j=1;j<=h;j++) cube(sx,sy-j*CC,c=='#'?7:6,0,(j<h?1:0)|(j>1?2:0)); }
            else if(c=='F'){ for(int j=1;j<=2;j++) cube(sx,sy-j*CC,2,0,(j<2?1:0)|(j>1?2:0)); }   // fridge: white, 2 blocks tall
            else if(c=='T') cube(sx,sy-CC,8,0,0);                                                  // toilet: pale, 1 block
            else if(c=='=') cube(sx,sy-6,8,1,2);
            if((ed&&c=='B')||(!ed&&!lhave&&tx==BDX&&ty==BDY)){   // the skateboard pickup, bobbing
                int by=sy-3-(ed?0:((lfr>>4)&1));
                rect(sx-6,by-1,12,3,RGB(26,10,6)); rect(sx-5,by-2,10,1,RGB(31,20,8)); rect(sx-5,by+2,2,2,RGB(3,3,6)); rect(sx+3,by+2,2,2,RGB(3,3,6));
            }
            if(ed&&c=='P'){ rect(sx-2,sy-9,5,7,RGB(28,10,8)); rect(sx-2,sy-13,5,4,RGB(30,23,17)); }   // little person = spawn
        }
        if(!ed&&s==ss){
            int fhp=tileH(lfx>>8,lfy>>8), zp=(int)(lz>>8), vsel=((lhd+lspin+66)>>2)&3;
            rect(psx-3,psy-fhp-1,7,2,RGB(10,8,5)); rect(psx-1,psy-fhp-2,3,4,RGB(10,8,5));   // shadow
            if(lskate){ rect(psx-6,psy-zp-1,12,2,RGB(26,10,6)); rect(psx-5,psy-zp+1,2,2,RGB(3,3,6)); rect(psx+3,psy-zp+1,2,2,RGB(3,3,6)); }   // board under the feet
            blit(spr4[vsel],psx-16,psy-40-zp);
        }
    }
    if(ed){   // blinking diamond on the tile under the cursor
        int sx=LOX+(ecx-ecy)*CA, sy=LOY+(ecx+ecy+1)*CB-tileH(ecx,ecy); u16 cc=(efr&8)?WHITE:GOLD;
        for(int t=-CA;t<=CA;t++){ int at=t<0?-t:t, hh=hhT[0][at]; px(sx+t,sy-hh,cc); px(sx+t,sy-hh-1,cc); px(sx+t,sy+hh,cc); px(sx+t,sy+hh+1,cc); }
    }
}
static void lifeDraw(void){
    drawRoom(0);
    u16 gold=GOLD, dim=DIMC;
    text(2,2,"SCORE",dim,1); numText(24,2,lscore,gold);
    text(2,10,"SPEED",dim,1); rect(24,10,lsp,5,RGB(8,24,10));
    text(150,2,MAPNAME,RGB(14,16,18),1);
    text(150,10,"FOOD",dim,1); rect(172,10,lfood/2,5,lfood<20?RGB(28,8,6):RGB(10,24,8));
    text(150,18,"WC",dim,1); rect(172,18,lbl/2,5,lbl>80?RGB(28,8,6):RGB(26,22,6));
    if(lnear&&!ldead) text(2,132,lnear==1?"R OPEN FRIDGE":"R USE TOILET",gold,1);
    text(60,2,lskate?"SKATE":(lsp>5?"RUN":"WALK"),gold,1);
    if(ldead) text(2,25,"PRESS A TO RESPAWN",RGB(31,12,8),1);
    if(lnoteT>0){ text(2,18,lnote,RGB(31,31,31),1); if(lpts&&lnote[0]=='N'){ text(2,25,"+",gold,1); numText(6,25,lpts,gold); } }
    text(2,146,lskate?"A PUSH B OLLIE DPAD STEER L WALK":(lhave?"DPAD WALK B RUN A HOP L SKATE":"DPAD WALK B RUN A HOP FIND A BOARD"),RGB(12,14,16),1);
    text(2,153,"START MENU",RGB(12,14,16),1);
}
static const char* const lifeItems[4]={"RESUME","HOW TO PLAY","EDIT MAP","BACK TO CREATURE"};
static const char* const lifeItemsEd[3]={"RESUME","HOW TO PLAY","BACK TO EDITOR"};
static void lifeMode(int ed){   // ed=1: test play started from the map editor
    lifeInit(); u16 prev=keyNow();
    for(int fr=0;;fr++){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if((k&K_SEL)&&(k&K_START)) break;
        if(pr&K_START){   // pause menu
            sfxStop();
            int c=menu("PAUSED",ed?lifeItemsEd:lifeItems,ed?3:4);
            if(c==1) helpScreen("HOW TO PLAY",lifeHelp,12);
            else if(c==2&&!ed){ mapEditor(); lifeInit(); }
            else if((c==2&&ed)||c==3) break;
            prev=keyNow(); continue;
        }
        lifeStep(k,pr,fr); lifeDraw(); present();
    }
    sfxStop();
    while((~REG_KEYINPUT)&0x3FF) vsync();   // wait for release so the caller doesn't see the exit keys
}

// ---------- map editor ----------
static const char* const mapItems[5]={"PLAY TEST","SAVE MAP","RESET MAP","HOW TO EDIT","BACK"};
static const char* const yesNo[2]={"NO","YES RESET"};
static void mapEditor(void){
    int ts=0, hold[4]={0}; u16 prev=keyNow();
    static const u16 dirK[4]={K_RIGHT,K_LEFT,K_UP,K_DOWN};
    for(efr=0;;efr++){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        int tr[4];
        for(int i=0;i<4;i++){ hold[i]=(k&dirK[i])?hold[i]+1:0; tr[i]=(hold[i]==1)||(hold[i]>14&&(hold[i]&3)==0); }
        int ux=tr[0]-tr[1], uy=tr[3]-tr[2];
        if(ux||uy){   // screen-relative like walking: up = away from the camera
            int dx=ux+uy, dy=uy-ux; dx=(dx>0)-(dx<0); dy=(dy>0)-(dy<0);
            ecx+=dx; ecy+=dy; if(ecx<0)ecx=0; if(ecy<0)ecy=0; if(ecx>=MW)ecx=MW-1; if(ecy>=MH)ecy=MH-1;
            if(k&K_A) mapPlace(ecx,ecy,palCh[ts]); else if(k&K_B) mapPlace(ecx,ecy,'.');
        }
        if(pr&K_A) mapPlace(ecx,ecy,palCh[ts]);
        if(pr&K_B) mapPlace(ecx,ecy,'.');
        if(pr&K_R) ts=(ts+1)%9;
        if(pr&K_L) ts=(ts+8)%9;
        if(pr&K_SEL){ int i=palIdx(lifeMap[ecy][ecx]); if(i>=0) ts=i; }
        if(pr&K_START){
            int c=menu("MAP MENU",mapItems,5);
            if(c==0){ mapScan(); lifeMode(1); }
            else if(c==1){ mapSave(); toast(mapSaved()?"MAP SAVED":"SAVE NOT SUPPORTED HERE"); }
            else if(c==2){ if(menu("RESET THE MAP",yesNo,2)==1){ mapReset(); toast("MAP RESET"); } }
            else if(c==3) helpScreen("HOW TO EDIT",mapHelp,12);
            else if(c==4){ mapSave(); break; }
            prev=keyNow(); continue;
        }
        drawRoom(1);
        text(2,2,"MAP EDITOR",GOLD,1); text(2,10,palNm[ts],WHITE,1);
        for(int i=0;i<9;i++){ int x=2+i*13; rect(x,136,12,8,i==ts?WHITE:RGB(3,4,7)); rect(x+1,137,10,6,palCol[i]); }
        text(2,146,"DPAD MOVE A PLACE B ERASE",RGB(12,14,16),1);
        text(2,153,"L R TILE SEL PICK START MENU",RGB(12,14,16),1);
        present();
    }
    while((~REG_KEYINPUT)&0x3FF) vsync();
}

int main(void){
    REG_WAITCNT=0x4317;  // ROM 3/1 waits + prefetch (power-on default is 4/2, no prefetch)
    REG_DISPCNT=0x0403;  // mode 3, BG2 on
    initTables(); setColors();
    titleScreen();
    starter();
    mapReset(); mapLoad();   // default room, or the one saved to SRAM
    u16 prev=0; int hold[10]={0}, frame=0, dirty=1, lastBlink=-1, comboUsed=0;
    for(;;){
        u16 k=(u16)(~REG_KEYINPUT)&0x3FF, pressed=k&~prev, released=prev&~k; prev=k;
        for(int i=0;i<10;i++) hold[i]=(k>>i&1)?hold[i]+1:0;
        #define TRIG(m,i) ((pressed&(m))||(hold[i]>14&&(hold[i]&3)==0))
        int sel=k&K_SEL;
        if(sel){
            if(pressed&K_A){ skinI=(skinI+1)%4; setColors(); comboUsed=1; dirty=1; }
            if(pressed&K_B){ hairI=(hairI+1)%5; setColors(); comboUsed=1; dirty=1; }
            if(pressed&K_R){ part=(part+1)%NENT; comboUsed=1; dirty=1; }
            if(pressed&K_L){ part=(part+NENT-1)%NENT; comboUsed=1; dirty=1; }
            if(pressed&K_RIGHT){ view=(view+1)&3; comboUsed=1; dirty=1; }
            if(pressed&K_LEFT){ view=(view+3)&3; comboUsed=1; dirty=1; }
            if(pressed&(K_UP|K_DOWN)){
                comboUsed=1;
                if(part<NPARTS&&parts[part].dk){ int kd=parts[part].dk-1; sty[kd]=(sty[kd]+((pressed&K_UP)?1:2))%3; dirty=1; }
            }
        } else {
            if(TRIG(K_RIGHT,4)){moveView(1,0);dirty=1;} if(TRIG(K_LEFT,5)){moveView(-1,0);dirty=1;}
            if(TRIG(K_UP,6)){moveView(0,-1);dirty=1;}     if(TRIG(K_DOWN,7)){moveView(0,1);dirty=1;}
            if(TRIG(K_R,8)){cy++;dirty=1;}      if(TRIG(K_L,9)){cy--;dirty=1;}
            if(pressed&K_A){ if(part==NPARTS) lifeMode(0); else if(part==NPARTS+1) mapEditor(); else doPart(1,part,size,cx,cy,cz); dirty=1; }
            if(pressed&K_B){ if(part<NPARTS) doPart(2,part,size,cx,cy,cz); dirty=1; }
        }
        if(released&K_SEL){ if(!comboUsed){ part=(part+1)%NENT; dirty=1; } comboUsed=0; }
        if(pressed&K_START){ size=size%3+1; dirty=1; }
        clampCursor();
        if(dirty) frame=16;   // restart blink with the ghost visible
        int blink=(frame>>4)&1;
        if(dirty||blink!=lastBlink){
            for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ ghost[y][z][x]=0; gdec[y][z][x]=0; }
            gAny=0;
            if(part<NPARTS){ doPart(0,part,size,cx,cy,cz);
            if(parts[part].dk&&!gAny) ghost[cy][cz][cx]=1; }   // nothing solid under the cursor: show a marker cube
            drawScene(blink,dirty); if(dirty) present(); else presentScene();   // blink-only: scene columns only
            dirty=0; lastBlink=blink;
        } else vsync();
        frame++;
    }
}
