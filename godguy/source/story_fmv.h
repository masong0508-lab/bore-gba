// Pre-rendered 128 x 72 clips, doubled into the existing indexed UI layer.
#include "story_fmv_data.h"

static void fmvDraw(const u16 *clip, int frame){
    // The sub palette leaves 32..39 and 56..63 free between UI and kingdom colors.
    static const u8 slots[16]={32,33,34,35,36,37,38,39,56,57,58,59,60,61,62,63};
    for(int i=0;i<16;i++) BG_PALETTE_SUB[slots[i]]=fmvPalette[i];
    uiRect(0,0,256,24,U_SHADOW);
    uiRect(0,168,256,24,U_SHADOW);
    const u16 *rows=clip+frame*FMV_H;
    for(int y=0;y<FMV_H;y++){
        u8 *line=uiBuf+(24+y*2)*256;
        const u8 *run=fmvRows+rows[y];
        for(int x=0;x<FMV_W*2;){
            const u8 packet=*run++;
            const int length=2*((packet>>4)+1);
            memset(line+x,slots[packet&15],length);
            x+=length;
        }
        memcpy(line+256,line,256);
    }
    uiTextR(248,178,"A / START  SKIP",U_DIM);
    uiFlush();
}
