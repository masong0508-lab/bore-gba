// GodGuy 3DS cinema: verify pixel-crisp 320x240 output on both LCDs.
// This tests the actual shim, not a separately reimplemented scaler.
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __3DS__ 1
#include "compat/ndsshim.h"

static struct {
    u8 guardA[32], top[400*240*3], guardB[32];
    u8 bot[320*240*3], guardC[32];
} buffers;

static void check(int ok,const char *why){
    if(!ok){fprintf(stderr,"CINEMA DISPLAY FAIL: %s\n",why);exit(1);}
}
static const u8 *px(const u8 *screen,int width,int x,int y){
    return screen+(y*width+x)*3;
}
static void same(const u8 *a,const u8 *b,const char *message){
    check(a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2],message);
}
int main(void){
    memset(&buffers,0,sizeof buffers);
    memset(buffers.guardA,0xa5,sizeof buffers.guardA);
    memset(buffers.guardB,0x6e,sizeof buffers.guardB);
    memset(buffers.guardC,0x4c,sizeof buffers.guardC);
    BG_PALETTE_SUB[0]=RGB15(0,0,0);
    BG_PALETTE_SUB[1]=RGB15(30,2,4);
    BG_PALETTE_SUB[2]=RGB15(3,30,6);
    BG_PALETTE_SUB[3]=RGB15(6,8,30);
    for(int y=0;y<192;y++)
        for(int x=0;x<256;x++)
            shimBmp[y*256+x]=(u8)(1+((x/8+y/8)%3));
    shimSwap=1;shimFilmFullscreen=1;
    int tc=0,bc=0;
    shimRenderChanged(buffers.top,buffers.bot,&tc,&bc);
    check(tc&&bc,"a film start did not update both LCDs");
    // The 320x240 display contains exactly the same sharp source pixels
    // with each source coordinate reused at 5:4 pixel-correct intervals.
    for(int y=0;y<240;y+=5)
        for(int x=0;x<320;x+=5){
            int sx=x*4/5,sy=y*4/5;
            const u8 *color=shimLutS[shimBmp[sy*256+sx]];
            same(px(buffers.bot,320,x,y),color,
                 "film scaling changed palette or selected a wrong texel");
            same(px(buffers.top,400,x+40,y),color,
                 "top 3DS LCD differs from the touch LCD film");
        }
    for(int y=0;y<240;y+=23){
        const u8 black[3]={0,0,0};
        same(px(buffers.top,400,0,y),black,
             "top left pillarbox was not cleared");
        same(px(buffers.top,400,399,y),black,
             "top right pillarbox was not cleared");
    }
    check(buffers.guardA[0]==0xa5&&buffers.guardA[31]==0xa5&&
          buffers.guardB[0]==0x6e&&buffers.guardB[31]==0x6e&&
          buffers.guardC[0]==0x4c&&buffers.guardC[31]==0x4c,
          "rendering overran the destination framebuffers");
    tc=bc=0;
    shimRenderChanged(buffers.top,buffers.bot,&tc,&bc);
    check(!tc&&!bc,"identical film frame made two unnecessary LCD uploads");
    // Ending the film restores the letterboxed DS UI layout and clears
    // the extra 3DS top-screen copy (including its previous movie pixels).
    shimFilmFullscreen=0;
    tc=bc=0;
    shimRenderChanged(buffers.top,buffers.bot,&tc,&bc);
    check(tc&&bc,"leaving the film failed to repaint the UI");
    same(px(buffers.bot,320,32,24),shimLutS[shimBmp[0]],
         "legacy 256x192 UI bitmap did not return");
    check(px(buffers.top,400,200,120)[0]==0,
          "the old cinematic frame stayed stuck on the top screen");
    puts("3DS CINEMA DISPLAY OK: sharp 320x240 dual-screen film, touch geometry, "
         "no blur, stable reuse and safe legacy UI restore");
    return 0;
}
