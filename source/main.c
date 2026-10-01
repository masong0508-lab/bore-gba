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
#define EWRAM_BSS __attribute__((section(".sbss")))

#define SW 240
#define SH 160
#define W 6
#define D 4
#define H 8
#define CA 10   // cube half width
#define CB 5    // cube half height of top face
#define CC 10   // cube side height
#define OX 50
#define OY 96
#define PANEL_X 124

enum { K_A=1, K_B=2, K_SEL=4, K_START=8, K_RIGHT=16, K_LEFT=32, K_UP=64, K_DOWN=128, K_R=256, K_L=512 };

#define RGB(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
static u16 fb[SW*SH] EWRAM_BSS;

// ---------- palette ----------
static const u16 skinTones[4] = { RGB(30,23,17), RGB(24,16,10), RGB(13,8,5), RGB(14,26,10) };
static const u16 hairTones[5] = { RGB(5,3,2), RGB(27,21,6), RGB(28,8,4), RGB(21,21,22), RGB(10,22,12) };
static u16 base[9], sT[9], sL[9], sR[9];
static u16 dL[4], dR[4];   // face-sprite palette (k w r s) pre-shaded for the left / right cube face
static int skinI = 0, hairI = 0;
#define EDGE RGB(3,2,5)
#define SKY  RGB(20,26,31)
#define PANEL RGB(5,6,9)

static u16 shade(u16 c, int n) {
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
static inline void px(int x,int y,u16 c){ if((unsigned)x<SW && (unsigned)y<SH) fb[y*SW+x]=c; }
static void vline(int x,int y0,int y1,u16 c){
    if((unsigned)x>=SW) return; if(y0<0)y0=0; if(y1>=SH)y1=SH-1;
    for(;y0<=y1;y0++) fb[y0*SW+x]=c;
}
static void rect(int x,int y,int w,int h,u16 c){ for(int j=0;j<h;j++)for(int i=0;i<w;i++)px(x+i,y+j,c); }
static void line(int x0,int y0,int x1,int y1,u16 c){
    int dx=x1>x0?x1-x0:x0-x1, dy=y1>y0?y0-y1:y1-y0, sx=x0<x1?1:-1, sy=y0<y1?1:-1, e=dx+dy;
    for(;;){ px(x0,y0,c); if(x0==x1&&y0==y1)break; int e2=2*e;
        if(e2>=dy){e+=dy;x0+=sx;} if(e2<=dx){e+=dx;y0+=sy;} }
}

// 3x5 font: A-Z, 0-9, '+', '>'
static const u8 F[38][5] = {
{2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},
{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{6,5,5,5,5},
{2,5,5,5,2},{6,5,6,4,4},{2,5,5,7,3},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},
{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7},
{7,5,5,5,7},{2,6,2,2,7},{6,1,2,4,7},{6,1,6,1,6},{5,5,7,1,1},{7,4,6,1,6},{3,4,7,5,7},{7,1,2,2,2},{7,5,7,5,7},{7,5,7,1,6},
{0,2,7,2,0},{4,6,7,6,4} };
static void text(int x,int y,const char*s,u16 c,int sc){
    for(;*s;s++,x+=4*sc){
        int i=-1; char ch=*s;
        if(ch>='A'&&ch<='Z') i=ch-'A'; else if(ch>='0'&&ch<='9') i=26+ch-'0';
        else if(ch=='+') i=36; else if(ch=='>') i=37;
        if(i<0) continue;
        for(int r=0;r<5;r++)for(int cc=0;cc<3;cc++)
            if((F[i][r]>>(2-cc))&1) rect(x+cc*sc,y+r*sc,sc,sc,c);
    }
}
static void num(int x,int y,int n,u16 c){ char s[2]={(char)('0'+n),0}; text(x,y,s,c,1); }

static u16 lite(u16 c,int n){
    int r=(c&31)*n/16, g=((c>>5)&31)*n/16, b=((c>>10)&31)*n/16;
    if(r>31)r=31; if(g>31)g=31; if(b>31)b=31; return RGB(r,g,b);
}
// Soft voxel: tonal outline only on the silhouette (no seams between joined blocks), lit rim, shaded base.
// shape 0 block, 1 slim limb, 2 hand, 3 leg. f: 1 block above, 2 block below, 16/32 coplanar neighbour at left/right edge.
static void cube(int sx,int sy,int ci,int shape,int f){
    int r=shape==0?CA:shape==1?7:shape==2?5:8, rb=r/2, ch=shape==2?7:CC;
    if(shape) f&=3;
    u16 T=sT[ci], L=sL[ci], R=sR[ci], eT=shade(T,9), eL=shade(L,9), eR=shade(R,9);
    for(int t=-r;t<=r;t++){
        int at=t<0?-t:t, hh=rb*(r-at)/r, x=sx+t, yt=sy+hh, yb=yt+ch-1;
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
static void drawDeco(int sx,int sy,u16 code,int face,int tint){
    const Spr*sp=&spr[(code&7)-1];
    int ci=(code>>3)&7, cj=(code>>6)&3, sz=((code>>8)&3)+1, fl=(code>>10)&1, aw=10*sp->wc-1;
    int wp=(aw+1)*sz-1, hp=10*sz-2;   // footprint size in px
    const u16*pal=face?dR:dL;
    for(int lr=cj?-1:0;lr<8;lr++){
        int ay=((cj*10+lr)*8)/hp; if(ay>7) ay=7;
        const char*row=sp->art[ay];
        for(int lc=ci?-1:0;lc<9;lc++){
            int ax=((ci*10+lc)*aw)/wp; if(ax>aw-1) ax=aw-1;
            if(fl) ax=aw-1-ax;
            char c=row[ax]; if(c=='.') continue;
            u16 col=tint?(face?sR[8]:sL[8]):pal[c=='k'?0:c=='w'?1:c=='r'?2:3];
            if(!face) px(sx-CA+1+lc, sy+(CB*(1+lc))/CA+1+lr, col);
            else      px(sx+1+lc,    sy+(CB*(CA-1-lc))/CA+1+lr, col);
        }
    }
}

static int cx,cy,cz,part,size;
#define OXC 60
#define OYC 121
static int view=0;   // 0..3 = 90 degree turns
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
static void drawScene(int blink){
    setColors();
    for(int i=0;i<SW*SH;i++) fb[i]=(i%SW)<PANEL_X?SKY:PANEL;
    // floor grid
    u16 gc=RGB(13,18,22); int a,b,c,d;
    for(int i=0;i<=W;i++){ projC(2*i-W,-D,0,&a,&b); projC(2*i-W,D,0,&c,&d); line(a,b,c,d,gc); }
    for(int j=0;j<=D;j++){ projC(-W,2*j-D,0,&a,&b); projC(W,2*j-D,0,&c,&d); line(a,b,c,d,gc); }
    int fv=view==0?0:view==3?1:-1;   // which cube face shows the +Z (front) face, -1 = turned away
    // voxels (back to front)
    for(int y=0;y<H;y++)for(int s=-8;s<=8;s++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){
        int u=2*x+1-W, w=2*z+1-D, ru, rw; rotUW(u,w,&ru,&rw);
        if(ru+rw!=s) continue;
        int raw=vox[y][z][x], ci=raw&15, shape=raw>>4;
        if(ghost[y][z][x]&&blink) ci=8;
        if(gdec[y][z][x]&&blink&&fv<0) ci=8;   // face turned away: flag the target voxel instead
        if(!ci) continue;
        int sx,sy; projC(u,w,y+1,&sx,&sy);   // top-face centre
        if(shape==1||shape==2){ int sg=u<0?1:-1, a2,b2; rotUW(sg,0,&a2,&b2); sx+=3*(a2-b2); sy+=(3*(a2+b2))/2; }   // hug the torso
        int f=(solid(x,y+1,z)?1:0)|(solid(x,y-1,z)?2:0)
             |(solid(x-dA[view][0],y,z-dA[view][1])?16:0)|(solid(x-dB[view][0],y,z-dB[view][1])?32:0);
        cube(sx,sy,ci,shape,f);
        u16 dc=dec[y][z][x]; int tint=0;
        if(gdec[y][z][x]&&blink){ dc=gdec[y][z][x]; tint=1; }
        if(dc&&fv>=0) drawDeco(sx,sy,dc,fv,tint);
    }
    // panel
    text(130,5,"BORE",RGB(31,26,6),2);
    text(130,17,"VOXEL DEMO",RGB(14,16,18),1);
    for(int i=0;i<NPARTS;i++){
        int y=28+i*8;
        if(i==part){ rect(128,y-1,108,7,RGB(6,16,8)); text(130,y,">",RGB(31,31,31),1); }
        text(137,y,parts[i].name,i==part?RGB(31,31,31):RGB(18,20,22),1);
        if(i==part&&parts[i].dk) text(190,y,spr[SPRID(parts[i].dk-1)].name,RGB(31,26,6),1);
    }
    text(130,94,"SIZE",RGB(18,20,22),1);
    const char*sn[3]={"S","M","L"};
    for(int i=0;i<3;i++){
        int x=156+i*16; rect(x,92,12,9,i==size-1?RGB(6,16,8):RGB(2,3,5));
        text(x+4,94,sn[i],RGB(31,31,31),1);
    }
    text(130,106,"X",RGB(18,20,22),1); num(136,106,cx,RGB(31,31,31));
    text(148,106,"Y",RGB(18,20,22),1); num(154,106,cy,RGB(31,31,31));
    text(166,106,"Z",RGB(18,20,22),1); num(172,106,cz,parts[part].dk?RGB(12,14,16):RGB(31,31,31));   // sprites ignore Z
    u16 hc=RGB(12,14,16);
    text(130,116,"DPAD X Z  L R HEIGHT",hc,1); text(130,122,"A PLACE B ERASE",hc,1);
    text(130,128,"START SIZE  SEL TAP PART",hc,1); text(130,134,"SEL+UP DOWN FACE STYLE",hc,1);
    text(130,140,"SEL+A SKIN SEL+B HAIR",hc,1); text(130,146,"SEL+LEFT RIGHT TURN VIEW",hc,1);
}
static void vsync(void){ while(REG_VCOUNT>=160); while(REG_VCOUNT<160); }
static void present(void){
    vsync();
    REG_DMA3SAD=(u32)(uintptr_t)fb; REG_DMA3DAD=VRAM_ADDR;
    REG_DMA3CNT=(SW*SH/2)|0x84000000u;
}

int main(void){
    REG_DISPCNT=0x0403;  // mode 3, BG2 on
    starter();
    u16 prev=0; int hold[10]={0}, frame=0, dirty=1, lastBlink=-1, comboUsed=0;
    for(;;){
        u16 k=(u16)(~REG_KEYINPUT)&0x3FF, pressed=k&~prev, released=prev&~k; prev=k;
        for(int i=0;i<10;i++) hold[i]=(k>>i&1)?hold[i]+1:0;
        #define TRIG(m,i) ((pressed&(m))||(hold[i]>14&&(hold[i]&3)==0))
        int sel=k&K_SEL;
        if(sel){
            if(pressed&K_A){ skinI=(skinI+1)%4; comboUsed=1; dirty=1; }
            if(pressed&K_B){ hairI=(hairI+1)%5; comboUsed=1; dirty=1; }
            if(pressed&K_R){ part=(part+1)%NPARTS; comboUsed=1; dirty=1; }
            if(pressed&K_L){ part=(part+NPARTS-1)%NPARTS; comboUsed=1; dirty=1; }
            if(pressed&K_RIGHT){ view=(view+1)&3; comboUsed=1; dirty=1; }
            if(pressed&K_LEFT){ view=(view+3)&3; comboUsed=1; dirty=1; }
            if(pressed&(K_UP|K_DOWN)){
                comboUsed=1;
                if(parts[part].dk){ int kd=parts[part].dk-1; sty[kd]=(sty[kd]+((pressed&K_UP)?1:2))%3; dirty=1; }
            }
        } else {
            if(TRIG(K_RIGHT,4)){moveView(1,0);dirty=1;} if(TRIG(K_LEFT,5)){moveView(-1,0);dirty=1;}
            if(TRIG(K_UP,6)){moveView(0,-1);dirty=1;}     if(TRIG(K_DOWN,7)){moveView(0,1);dirty=1;}
            if(TRIG(K_R,8)){cy++;dirty=1;}      if(TRIG(K_L,9)){cy--;dirty=1;}
            if(pressed&K_A){ doPart(1,part,size,cx,cy,cz); dirty=1; }
            if(pressed&K_B){ doPart(2,part,size,cx,cy,cz); dirty=1; }
        }
        if(released&K_SEL){ if(!comboUsed){ part=(part+1)%NPARTS; dirty=1; } comboUsed=0; }
        if(pressed&K_START){ size=size%3+1; dirty=1; }
        clampCursor();
        if(dirty) frame=16;   // restart blink with the ghost visible
        int blink=(frame>>4)&1;
        if(dirty||blink!=lastBlink){
            for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ ghost[y][z][x]=0; gdec[y][z][x]=0; }
            gAny=0;
            doPart(0,part,size,cx,cy,cz);
            if(parts[part].dk&&!gAny) ghost[cy][cz][cx]=1;   // nothing solid under the cursor: show a marker cube
            drawScene(blink); present();
            dirty=0; lastBlink=blink;
        } else vsync();
        frame++;
    }
}
