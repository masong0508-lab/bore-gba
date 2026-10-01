// BORE - GBA voxel creature creator (tech demo)
// Build space: 6 wide (X) x 4 long (Z) x 8 high (Y). Mode 3, no libraries.
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

static void cube(int sx,int sy,int ci){
    u16 T=sT[ci], L=sL[ci], R=sR[ci];
    for(int t=-CA;t<=CA;t++){
        int at=t<0?-t:t, hh=CB*(CA-at)/CA, x=sx+t, yt=sy+hh;
        vline(x,yt,yt+CC-1,t<0?L:R);
        vline(x,sy-hh,sy+hh,T);
        px(x,sy-hh,EDGE); px(x,sy+hh,EDGE); px(x,yt+CC-1,EDGE);
        if(t==-CA||t==0||t==CA) vline(x,yt,yt+CC-1,EDGE);
    }
}
static void proj(int x,int y,int z,int*ox,int*oy){ *ox=OX+(x-z)*CA; *oy=OY+(x+z)*CB-y*CC; }

// ---------- parts ----------
typedef struct { const char*name; u8 n,mirror,w,h,d; const u8 (*c)[4]; } Part;
static const u8 cHead[][4]={{0,0,0,1},{1,0,0,1},{0,1,0,1},{1,1,0,1},{0,0,1,1},{1,0,1,1},{0,1,1,1},{1,1,1,1}};
static const u8 cTorso[][4]={{0,0,0,6},{1,0,0,6},{0,1,0,6},{1,1,0,6},{0,0,1,6},{1,0,1,6},{0,1,1,6},{1,1,1,6}};
static const u8 cArm[][4]={{0,0,0,1},{0,1,0,1},{0,2,0,1}};
static const u8 cLeg[][4]={{0,0,0,7},{0,1,0,7},{0,2,0,7}};
static const u8 cEye[][4]={{0,0,0,3},{0,1,0,2}};
static const u8 cMouth[][4]={{0,0,0,3},{1,0,0,3}};
static const u8 cEar[][4]={{0,0,0,1},{0,1,0,1}};
static const u8 cHair[][4]={{0,0,0,5},{1,0,0,5},{0,0,1,5},{1,0,1,5},{0,1,0,5},{1,1,1,5}};
#define NPARTS 8
static const Part parts[NPARTS]={
 {"HEAD",8,0,2,2,2,cHead},{"TORSO",8,0,2,2,2,cTorso},{"ARM",3,1,1,3,1,cArm},{"LEG",3,1,1,3,1,cLeg},
 {"EYE",2,1,1,2,1,cEye},{"MOUTH",2,0,2,1,1,cMouth},{"EAR",2,1,1,2,1,cEar},{"HAIR",6,0,2,2,2,cHair}};

// ---------- state ----------
static u8 vox[H][D][W], ghost[H][D][W];
static int cx=2,cy=0,cz=1,part=0,size=1;

static void apply(int x0,int y0,int z0,int flip,int act,int pi,int s){
    const Part*p=&parts[pi];
    for(int i=0;i<p->n;i++)
      for(int a=0;a<s;a++)for(int b=0;b<s;b++)for(int c=0;c<s;c++){
        int X=p->c[i][0]*s+a; if(flip) X=p->w*s-1-X;
        int x=x0+X, y=y0+p->c[i][1]*s+b, z=z0+p->c[i][2]*s+c;
        if(x<0||x>=W||y<0||y>=H||z<0||z>=D) continue;
        if(act==0) ghost[y][z][x]=1; else if(act==1) vox[y][z][x]=p->c[i][3]; else vox[y][z][x]=0;
      }
}
static void doPart(int act,int pi,int s,int x,int y,int z){
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
    doPart(1,3,1,2,0,1);  // legs
    doPart(1,1,1,2,3,1);  // torso
    doPart(1,2,1,1,3,1);  // arms
    doPart(1,0,1,2,5,1);  // head
    doPart(1,4,1,2,5,3);  // eyes
    doPart(1,7,1,2,7,1);  // hair
}

// ---------- scene ----------
static void drawScene(int blink){
    setColors();
    for(int i=0;i<SW*SH;i++) fb[i]=(i%SW)<PANEL_X?SKY:PANEL;
    // floor grid
    u16 gc=RGB(13,18,22); int a,b,c,d;
    for(int i=0;i<=W;i++){ proj(i,0,0,&a,&b); proj(i,0,D,&c,&d); line(a,b,c,d,gc); }
    for(int j=0;j<=D;j++){ proj(0,0,j,&a,&b); proj(W,0,j,&c,&d); line(a,b,c,d,gc); }
    // voxels (back to front)
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){
        int ci=vox[y][z][x];
        if(ghost[y][z][x]&&blink) ci=8;
        if(!ci) continue;
        int sx,sy; proj(x,y+1,z,&sx,&sy); sy+=CB;   // top-face centre
        cube(sx,sy,ci);
    }
    // panel
    text(130,5,"BORE",RGB(31,26,6),2);
    text(130,17,"VOXEL DEMO",RGB(14,16,18),1);
    for(int i=0;i<NPARTS;i++){
        int y=28+i*8;
        if(i==part){ rect(128,y-1,108,7,RGB(6,16,8)); text(130,y,">",RGB(31,31,31),1); }
        text(137,y,parts[i].name,i==part?RGB(31,31,31):RGB(18,20,22),1);
    }
    text(130,94,"SIZE",RGB(18,20,22),1);
    const char*sn[3]={"S","M","L"};
    for(int i=0;i<3;i++){
        int x=156+i*16; rect(x,92,12,9,i==size-1?RGB(6,16,8):RGB(2,3,5));
        text(x+4,94,sn[i],RGB(31,31,31),1);
    }
    text(130,106,"X",RGB(18,20,22),1); num(136,106,cx,RGB(31,31,31));
    text(148,106,"Y",RGB(18,20,22),1); num(154,106,cy,RGB(31,31,31));
    text(166,106,"Z",RGB(18,20,22),1); num(172,106,cz,RGB(31,31,31));
    u16 hc=RGB(12,14,16);
    text(130,116,"DPAD MOVE X Z",hc,1);  text(130,122,"L R HEIGHT",hc,1);
    text(130,128,"A PLACE B ERASE",hc,1); text(130,134,"START SIZE",hc,1);
    text(130,140,"SEL TAP NEXT PART",hc,1); text(130,146,"SEL+A SKIN SEL+B HAIR",hc,1);
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
        } else {
            if(TRIG(K_RIGHT,4)){cx++;dirty=1;} if(TRIG(K_LEFT,5)){cx--;dirty=1;}
            if(TRIG(K_UP,6)){cz--;dirty=1;}     if(TRIG(K_DOWN,7)){cz++;dirty=1;}
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
            for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++) ghost[y][z][x]=0;
            doPart(0,part,size,cx,cy,cz);
            drawScene(blink); present();
            dirty=0; lastBlink=blink;
        } else vsync();
        frame++;
    }
}
