// ALIVE tier 4: real poses for the Sim you control: WAVE (while talking), CHEER (the joy reaction) and SIT (sitting, or worn out).
// The baker (main.c, poseK) draws the body in the pose; only what DIFFERS from the standing frame is kept: runs of (skip, n, n palette bytes) over the
// 32 x SPH sprite, 0 = the standing pixel is gone. About 4 KB of EWRAM for all 3 poses x 4 views (full frames would be ~23 KB, tile patches ~11 KB).
// Baked once with the player (at the existing loading screen, same cache key), nothing is baked while playing, no extra frame buffer is needed.
#define POSE_POOL 3072
static u8 poseBuf[POSE_POOL] EWRAM_BSS;
static u16 poseOff[3][4], poseEnd[3][4]; static u16 poseTop; static u32 poseKey;
static int poseBx0=SPW, poseBx1=0, poseBy0=SPH, poseBy1=0;   // the box the poses reach (poseWiden: joins the blit box)
static void poseEncode(int p,const u8*st,const u8*ps,int v){   // the pose frame ps as runs over the standing frame st
    poseOff[p][v]=poseEnd[p][v]=poseTop; int n=SPW*SPH, i=0, last=0; u16 top=poseTop;
    while(i<n){ while(i<n&&st[i]==ps[i]) i++; if(i>=n) break;
        int j=i, e=i+1; while(e<n){ if(st[e]!=ps[e]){ e++; j=e; continue; } int g=0; while(e+g<n&&st[e+g]==ps[e+g]&&g<3) g++; if(e+g<n&&g<3&&st[e+g]!=ps[e+g]) e+=g; else break; }
        j=e; int len=j-i; if(len>255) len=255; j=i+len;
        int sk=i-last; while(sk>255){ if(top+2>POSE_POOL) return; poseBuf[top++]=255; poseBuf[top++]=0; sk-=255; }
        if(top+2+len>POSE_POOL){ poseEnd[p][v]=poseOff[p][v]; return; }   // pool full: this view has no pose (poseEnd == poseOff)
        poseBuf[top++]=(u8)sk; poseBuf[top++]=(u8)len; for(int k=0;k<len;k++) poseBuf[top++]=ps[i+k];
        int y0=i/SPW, y1=(j-1)/SPW; if(y0<poseBy0) poseBy0=y0; if(y1+1>poseBy1) poseBy1=y1+1;
        for(int k=i;k<j;k++){ int x=k%SPW; if(x<poseBx0) poseBx0=x; if(x+1>poseBx1) poseBx1=x+1; }
        last=j; i=j; }
    poseTop=top; poseEnd[p][v]=top;
}
static void poseBakeAll(void){   // the player is baked (standing in spr4): draw the three poses into spr4s one after another and keep their differences
    poseTop=0; poseBx0=SPW; poseBx1=0; poseBy0=SPH; poseBy1=0; poseKey=0;
    for(int p=0;p<3;p++){ poseK=p+1; bakeInto(spr4s); poseK=0; for(int v=0;v<4;v++) poseEncode(p,spr4[v],spr4s[v],v); }
    poseKey=bakeKey();
}
static void poseWiden(void){ if(!poseKey||poseBx0>=poseBx1) return; if(poseBx0<spBx0) spBx0=poseBx0; if(poseBx1>spBx1) spBx1=poseBx1; if(poseBy0<spBy0) spBy0=poseBy0; if(poseBy1>spBy1) spBy1=poseBy1; }
typedef void (*PoseEmit)(const u8*s,int a,int n,int x0,int y0);
static void poseWalk(int p,int v,PoseEmit em,int x0,int y0){   // the pose picture as pieces: standing pixels between the runs, the runs' own pixels
    const u8*st=spr4[v], *t=poseBuf+poseOff[p][v], *te=poseBuf+poseEnd[p][v]; int i=0, n=SPW*SPH;
    while(t<te){ int sk=t[0], len=t[1]; t+=2; if(sk) em(st+i,i,sk,x0,y0); i+=sk; if(len) em(t,i,len,x0,y0); i+=len; t+=len; }
    if(i<n) em(st+i,i,n-i,x0,y0);
}
static void poseDraw(const u8*s,int a,int n,int x0,int y0){   // n pixels from linear sprite index a, clipped like blit
    int x=a%SPW, y=a/SPW;
    for(int k=0;k<n;k++,x++){ if(x>=SPW){ x=0; y++; }
        u8 c=s[k]; if(!c||x<spBx0||x>=spBx1||y<spBy0||y>=spBy1) continue;
        int xx=x0+x, yy=y0+y; if((unsigned)(xx-cX0)>=cW||(unsigned)(yy-cY0)>=cH) continue; fb[yy*SW+xx]=sprPal[c]; }
}
static int poseSel(void){   // 0 none, 1 wave, 2 cheer, 3 sit: for the Sim you control, standing still, never on the board (the picture only)
    if(!poseKey||poseKey!=sprKey||lskate||ldead||lvx||lvy||lsp||plZ>plFh||lbailT>0) return 0;
    { int sp=slfPose(); if(sp) return sp; }   // self.h: the pose of the action you are doing
    if(alvPopK==1&&alvPopT>0) return 2;
    if(hhBubT) return 1;
    if(simAct==3||(sNrg<20&&hhStill>=240)) return 3;
    return 0;
}
static int poseBlit(int p,int v,int x0,int y0){ if(p<1||p>3||poseEnd[p-1][v]==poseOff[p-1][v]) return 0; poseWalk(p-1,v,poseDraw,x0,y0); return 1; }
