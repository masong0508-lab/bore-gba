// Pre-rendered 128 x 72 films and matching dialogue cels.
#include "gglz.h"

typedef struct {
    const u8 *data;
    int frames;
    const char *title;
} FmvClip;

#include "story_fmv_data.h"
_Static_assert(FMV_W*2==256&&FMV_H*2==144,"FMVs must fit the letterbox");
_Static_assert(FMV_BLOCK_LIMIT<=sizeof uiNew,"FMV blocks must fit the transition buffer");

// Free slots between the sub-screen UI, kingdom, terrain and title palettes.
static const u8 fmvSlots[32]={32,33,34,35,36,37,38,39,56,57,58,59,60,61,62,63,
                              96,97,98,99,188,189,190,191,192,193,194,195,196,197,198,199};
static const u8 *fmvBlock, *fmvRead, *fmvBlockEnd;

static void fmvSetPalette(void){
    for(int i=0;i<32;i++) BG_PALETTE_SUB[fmvSlots[i]]=fmvPalette[i];
}

static void fmvDraw(const FmvClip *clip,int frame){
    if(frame==0){
        fmvSetPalette();
        uiRect(0,0,256,24,U_SHADOW);
        uiRect(0,168,256,24,U_SHADOW);
        uiTextC(128,9,clip->title,U_GOLD,1);
        uiTextR(248,178,"A / START  SKIP",U_DIM);
        fmvBlock=clip->data;
        fmvRead=fmvBlockEnd=uiNew;
    }

    // The transition buffer is idle throughout playback. The last decoded
    // picture stays in uiBuf while commands patch only the changed pixels.
    if(fmvRead==fmvBlockEnd){
        int size=fmvBlock[0]|(fmvBlock[1]<<8);
        fmvBlock=ggUnpack(fmvBlock+2,uiNew,size);
        fmvRead=uiNew;
        fmvBlockEnd=uiNew+size;
    }
    int pixel=0;
    while(*fmvRead){
        u8 command=*fmvRead++;
        if(command<128){
            pixel+=command;
        } else {
            int length=command-127;
            u8 color=fmvSlots[*fmvRead++];
            while(length){
                int x=pixel%FMV_W, y=pixel/FMV_W;
                int count=FMV_W-x;
                if(count>length) count=length;
                u8 *line=uiBuf+(24+y*2)*256+x*2;
                memset(line,color,count*2);
                memset(line+256,color,count*2);
                pixel+=count;
                length-=count;
            }
        }
    }
    fmvRead++;
    uiFlush();
}

static void fmvPortrait(int x,int y,int who,int cel){
    u8 pixels[FMV_PORTRAIT_SIZE*FMV_PORTRAIT_SIZE];
    int offset=fmvPortraitOffsets[who*FMV_PORTRAIT_CELS+cel];
    ggUnpack(fmvPortraits+offset,pixels,sizeof pixels);
    fmvSetPalette();
    for(int row=0;row<FMV_PORTRAIT_SIZE;row++){
        for(int col=0;col<FMV_PORTRAIT_SIZE;col++){
            const u8 color=pixels[row*FMV_PORTRAIT_SIZE+col];
            if(color!=255) uiRect(x+col*2,y+row*2,2,2,fmvSlots[color]);
        }
    }
}
