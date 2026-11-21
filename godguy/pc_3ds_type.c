// Standalone 3DS cinematic typography regression; keeps new lettering
// independent from the gameplay-only font and checks clipping/antialiasing.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
typedef uint8_t u8;
static const u8 font5x7[64][7]={
    ['A'-32]={14,17,17,31,17,17,17},
    ['I'-32]={4,4,4,4,4,4,4},
    ['W'-32]={17,17,17,21,21,21,10},
    ['X'-32]={17,10,4,4,4,10,17}
};
#include "3ds/film_type3d.h"
static void check(int good,const char*message){
    if(!good){fprintf(stderr,"3DS TYPE FAIL: %s\n",message);exit(1);}
}
int main(void){
    static u8 storage[400*240*3+16];
    memset(storage,0xbc,sizeof(storage));
    u8 *out=storage+8;
    memset(out,18,400*240*3);
    check(ggFilmTypeGlyph('a')==ggFilmTypeGlyph('A'),
          "case folding fails for story dialogue");
    check(ggFilmTypeMeasure("III")<ggFilmTypeMeasure("WWW"),
          "typeface is still monospaced computer lettering");
    check(ggFilmTypeMeasure("")==0,"empty phrase takes screen space");
    check(ggFilmTypeWidth('I'-32)<ggFilmTypeWidth('W'-32),
          "glyph widths failed to follow character silhouettes");
    ggFilmText(out,40,40,"AX",220,198,170);
    int solid=0,soft=0;
    for(int y=40;y<47;y++)for(int x=36;x<70;x++){
        int v=out[(y*400+x)*3];
        if(v>=180)solid++;
        else if(v>18&&v<100)soft++;
    }
    check(solid>9&&soft>2,"anti-aliased diagonal lettering not visible");
    ggFilmText(out,397,230,"WWW WWW WWW",240,220,200);
    ggFilmText(out,-9,-4,"A",240,220,200);
    ggFilmText(out,10,260,"A",240,220,200);
    // Underlying RGB bytes at the edges remain valid; no secondary
    // framebuffer is allocated or needed for cinematic text smoothing.
    check(storage[0]==0xbc&&storage[7]==0xbc&&
          storage[400*240*3+8]==0xbc&&
          storage[400*240*3+15]==0xbc,
          "type renderer escaped the top-screen buffer");
    puts("3DS TYPE OK: proportionally spaced cinematic lettering, soft edges, safe clipping");
    return 0;
}
