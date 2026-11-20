// Export a short preview made from ACTUAL GodGuy 3DS film pixels.
// Shows the first 24 rendered intro frames and the first three directed
// 7-second shots from each of the first three chapters (accelerated).
// The image is read from uiBuf/3DS palette exactly as the game draws it.
// This exports video-only RGB24; audio remains in the playable game.
// Build (host): gcc -O2 -DNDSSHIM -D__3DS__ -Icompat -Isource -w
//               -include unistd.h pc_fmv_reel.c -lm -o /tmp/gg-reel
// Output: /tmp/gg-reel out/first-three-chapters.rgb  (216 frames at 256x192)
//
// All source images, cast sprites, text, and animation come from main.c's
// fmvRenderAt(). No non-game artwork or video generators are involved.
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <stdio.h>
#include <stdint.h>

enum { REEL_BAKED_FRAMES=24, REEL_STAGE_FRAMES=48, REEL_CHAPTERS=3 };
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
    for(int film=0;film<REEL_CHAPTERS;film++){
        const FmvClip *clip=&fmvClips[film];
        int prev=-1, sampleCount=0;
        storyScore=filmOpeningScore[film];
        // Delta-compressed baked frames have to be decoded sequentially.
        // Store 24 representative original animation frames in each scene.
        for(int frame=0;frame<clip->frames;frame++){
            if(!fmvRenderAt(clip,frame*FMV_TICKS)) goto invalid;
            int slot=frame*REEL_BAKED_FRAMES/clip->frames;
            if(slot!=prev){
                if(!reelWrite(out))goto invalid;
                prev=slot;sampleCount++;
            }
        }
        if(sampleCount!=REEL_BAKED_FRAMES)goto invalid;
        prev=-1;sampleCount=0;
        // 21 seconds of new in-game scripted shots (three opening beats).
        // Run every actual 3DS render tick to preserve backdrop loading,
        // actor movement, caption timing and stage animation. Capture just
        // 48 representative frames as a short video teaser.
        const int reelTicks=21*FILM_SECOND;
        for(int elapsed=0;elapsed<reelTicks;elapsed+=FILM_STAGE_TICKS){
            if(!fmvRenderAt(clip,clip->frames*FMV_TICKS+elapsed))goto invalid;
            int slot=elapsed*REEL_STAGE_FRAMES/reelTicks;
            if(slot!=prev){
                if(!reelWrite(out))goto invalid;
                prev=slot;sampleCount++;
            }
        }
        if(sampleCount!=REEL_STAGE_FRAMES)goto invalid;
        printf("REEL %s: %d baked + %d authored stage frames\n",
                clip->title,REEL_BAKED_FRAMES,REEL_STAGE_FRAMES);
    }
    if(fclose(out)){perror("close");return 2;}
    if(reelFrameCount!=REEL_CHAPTERS*(REEL_BAKED_FRAMES+REEL_STAGE_FRAMES)){
        fputs("REEL FAIL: wrong frame count\n",stderr);return 1;
    }
    printf("REEL OK: %d authentic 3DS GodGuy frames, 18.0s at 12 fps (video only)\n",reelFrameCount);
    return 0;
invalid:
    fclose(out);
    fprintf(stderr,"REEL FAIL: frame extraction failed in film %d at frame %d\n",film,reelFrameCount);
    return 1;
}
