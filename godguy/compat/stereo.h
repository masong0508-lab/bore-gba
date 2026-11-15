// stereo.h - the 3DS top screen in 3D: two pictures, one per eye, made from the one the game draws and a depth for every pixel of it.
// Standalone C (no game types, no libctru), so the 3DS platform file and a PC test can both use it.
//   rgb: the finished top screen, 400 x 240, RGB, row by row (the game's 256 x 192 picture sits at (72,24)).  dep: a depth for each pixel of that 256 x 192 picture, in levels:
//   0 is the screen itself, positive comes out of it, negative goes behind it.  strength: 0 (flat) .. 32 (a level is a pixel of shift in each eye).
// A pixel that is d levels out moves to the right in the left eye's picture and to the left in the right eye's, by d * strength / 32 pixels. Pictures are painted far to near; the gaps that a shift opens are
// filled from the unshifted picture. Around the game's picture the screen is a starfield in three layers of depth (behind the screen), drifting slowly: that is what makes the 3D visible even on a still menu.
#ifndef GG_STEREO_H
#define GG_STEREO_H
#include <string.h>
#define ST_W 400
#define ST_H 240
#define ST_X0 72
#define ST_Y0 24
#define ST_MINL (-8)
#define ST_MAXL 20
static unsigned stHash(unsigned x){ x^=x>>16; x*=0x7feb352du; x^=x>>15; x*=0x846ca68bu; x^=x>>16; return x; }
static inline int stShift(int level,int strength){ int v=level*strength; return v>=0?(v+16)/32:-((-v+16)/32); }
static void stBackground(unsigned char*o,int eye,int frame,int strength){    // eye: +1 left, -1 right
    for(int y=0;y<ST_H;y++){ int b=6+y*14/ST_H; unsigned char*r=&o[y*ST_W*3]; for(int x=0;x<ST_W;x++){ r[x*3]=(unsigned char)(b/2); r[x*3+1]=(unsigned char)(b/2+1); r[x*3+2]=(unsigned char)b; } }
    for(int s=0;s<150;s++){ unsigned h=stHash((unsigned)s*7919u+13u); int layer=(int)(h%3u), lvl=-2-layer*2, sh=stShift(lvl,strength)*eye;      // layer 0 is nearest (-2), layer 2 the farthest (-6)
        int x=(int)((h>>4)%ST_W)+(frame*(1+layer))/90, y=(int)((h>>14)%ST_H); x=(x%ST_W+ST_W)%ST_W; x+=sh; if(x<0||x>=ST_W) continue;
        int tw=((frame>>3)+(int)(h>>24))&7; int br=(layer==0?200:layer==1?150:105)+(tw<2?40:0); if(br>255) br=255; unsigned char*p=&o[(y*ST_W+x)*3]; p[0]=(unsigned char)(br*9/10); p[1]=(unsigned char)(br*9/10); p[2]=(unsigned char)br;
        if(layer==0&&x+1<ST_W){ p[3]=(unsigned char)(br/2); p[4]=(unsigned char)(br/2); p[5]=(unsigned char)(br*3/4); } } }
// L and R are 400 x 240 x 3 each.
static void stereoTop(unsigned char*L,unsigned char*R,const unsigned char*rgb,const signed char*dep,int frame,int strength){
    if(strength<=0){ memcpy(L,rgb,ST_W*ST_H*3); memcpy(R,rgb,ST_W*ST_H*3); return; }
    for(int eye=1;eye>=-1;eye-=2){ unsigned char*o=eye>0?L:R; stBackground(o,eye,frame,strength);
        for(int y=0;y<192;y++) memcpy(&o[((ST_Y0+y)*ST_W+ST_X0)*3],&rgb[((ST_Y0+y)*ST_W+ST_X0)*3],256*3);      // the unshifted picture fills the gaps
        int hist[ST_MAXL-ST_MINL+1]; memset(hist,0,sizeof hist); for(int i=0;i<256*192;i++){ int d=dep[i]; if(d<ST_MINL) d=ST_MINL; if(d>ST_MAXL) d=ST_MAXL; hist[d-ST_MINL]++; }
        for(int l=ST_MINL;l<=ST_MAXL;l++){ if(!hist[l-ST_MINL]) continue; int sh=stShift(l,strength)*eye;
            for(int y=0;y<192;y++){ const signed char*dr=&dep[y*256]; const unsigned char*src=&rgb[((ST_Y0+y)*ST_W+ST_X0)*3]; unsigned char*dst=&o[((ST_Y0+y)*ST_W)*3];
                for(int x=0;x<256;x++){ int d=dr[x]; if(d<ST_MINL) d=ST_MINL; if(d>ST_MAXL) d=ST_MAXL; if(d!=l) continue; int nx=ST_X0+x+sh; if(nx<0||nx>=ST_W) continue; dst[nx*3]=src[x*3]; dst[nx*3+1]=src[x*3+1]; dst[nx*3+2]=src[x*3+2]; } } } } }
#endif
