// Export actual GodGuy 3DS film frames. Four deliberate story beats per
// chapter show the setup, response, physical action, and consequence -- not
// an arbitrary montage of zoomed-in props. Original game art and rendering.
// Video-only RGB24; the playable game retains its soundtrack and effects.
// Build: gcc -O2 -DNDSSHIM -D__3DS__ -Icompat -Isource -w
//        -include unistd.h pc_fmv_reel.c -lm -o /tmp/gg-reel
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <stdio.h>
#include <stdint.h>

enum { REEL_BAKED_FRAMES=24, REEL_TAKE_FRAMES=24, REEL_TAKES=4, REEL_CHAPTERS=3 };
static const u8 reelShots[REEL_CHAPTERS][REEL_TAKES]={
    {0,3,4,7}, // planet, Wisp's seed, planting, first village light
    {0,1,2,5}, // Mira waits, Wisp answers, rain fills cup, handoff
    {0,2,3,7}, // wall is built, goat intervenes, hammer, home light
};
static unsigned char reelRgb[256*192*3];
static int reelFrameCount;

void shimInput(int frame,int*held,int*tx,int*ty){
    (void)frame;*held=*tx=*ty=0;
}
void shimPresent(void){}

static int reelWrite(FILE *f){
    // The real 3DS software renderer composites the same uiBuf indexes
    // against BG_PALETTE_SUB. Convert that RGB555 image to RGB888 for video.
    for(int i=0;i<256*192;i++){
        unsigned c=BG_PALETTE_SUB[uiBuf[i]];
        reelRgb[i*3]=(unsigned char)(((c&31u)*255u+15u)/31u);
        reelRgb[i*3+1]=(unsigned char)((((c>>5)&31u)*255u+15u)/31u);
        reelRgb[i*3+2]=(unsigned char)((((c>>10)&31u)*255u+15u)/31u);
    }
    if(fwrite(reelRgb,1,sizeof reelRgb,f)!=sizeof reelRgb) return 0;
    reelFrameCount++;
    return 1;
}

int main(int argc,char **argv){
    if(argc!=2){fputs("usage: reel <out.rgb>\n",stderr);return 2;}
    FILE *out=fopen(argv[1],"wb");
    if(!out){perror(argv[1]);return 2;}
    uiPalette();
    fmvSetPalette();
    int film;
    for(film=0;film<REEL_CHAPTERS;film++){
        const FmvClip *clip=&fmvClips[film];
        int prev=-1,bakedCount=0;
        storyScore=filmOpeningScore[film];
        // Sequentially decode every compressed frame even when not sampled.
        for(int frame=0;frame<clip->frames;frame++){
            if(!fmvRenderAt(clip,frame*FMV_TICKS))goto invalid;
            int slot=frame*REEL_BAKED_FRAMES/clip->frames;
            if(slot!=prev){
                if(!reelWrite(out))goto invalid;
                prev=slot;bakedCount++;
            }
        }
        if(bakedCount!=REEL_BAKED_FRAMES)goto invalid;

        int shot=-1,lastTake=-1,lastSlot=-1,stagedCount=0;
        int baked=clip->frames*FMV_TICKS;
        // Render the full extended film in narrative order so each set is
        // loaded at the real cut. Only sample the four chosen complete takes.
        for(int elapsed=0;elapsed<fmvLength(clip)-baked;elapsed+=FILM_STAGE_TICKS){
            int beatElapsed=elapsed;
            const FilmBeat *beat=filmBeatAt(clip,&beatElapsed);
            if(!beat)goto invalid;
            if(!beatElapsed)shot++;
            if(!fmvRenderAt(clip,baked+elapsed))goto invalid;
            int take=-1;
            for(int i=0;i<REEL_TAKES;i++)
                if(shot==reelShots[film][i])take=i;
            if(take<0)continue;
            if(take!=lastTake){lastTake=take;lastSlot=-1;}
            int slot=beatElapsed*REEL_TAKE_FRAMES/(beat->seconds*FILM_SECOND);
            if(slot!=lastSlot){
                if(!reelWrite(out))goto invalid;
                lastSlot=slot;stagedCount++;
            }
        }
        if(stagedCount!=REEL_TAKES*REEL_TAKE_FRAMES)goto invalid;
        printf("REEL %s: %d intro + %d directed-action frames\n",
               clip->title,bakedCount,stagedCount);
    }
    if(fclose(out)){perror("close");return 2;}
    if(reelFrameCount!=REEL_CHAPTERS*(REEL_BAKED_FRAMES+REEL_TAKES*REEL_TAKE_FRAMES)){
        fputs("REEL FAIL: wrong frame count\n",stderr);return 1;
    }
    printf("REEL OK: %d authentic 3DS film frames, 30s at 12 fps (video only)\n",reelFrameCount);
    return 0;
invalid:
    fclose(out);
    fprintf(stderr,"REEL FAIL: extraction failed in film %d at frame %d\n",film,reelFrameCount);
    return 1;
}
