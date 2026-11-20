// Regression for the CLOSE/MICRO renderer. The expected terrain is drawn
// with the original scanline-major algorithm, then compared byte for byte
// against the tile-major path. Also check that the VRAM tiles actually match
// the framebuffer after incremental uploads. No emulator is needed.
// gcc -O2 -DNDSSHIM -DGG_ZOOM_TEST -Icompat -Isource -include unistd.h \
//     pc_zoom_perf.c -lm -o /tmp/gg_zoom_perf && /tmp/gg_zoom_perf
#define main ds_main
#include "source/main.c"
#undef main
#include <stdio.h>
static u8 zoomExpected[256*ZH], zoomSaved[256*ZH];
static int errors;
#define CK(ok,msg) do { if(!(ok)){ fprintf(stderr,"ZOOM FAIL: %s\n",msg); errors++; } } while(0)
void shimInput(int frame,int*held,int*tx,int*ty){ (void)frame; *held=*tx=*ty=0; }
void shimPresent(void){}

static void legacyTerrain(void){
    zScale(); int S=zS_, sh=zSh_, ox=zOx(), oy=zOy();
    int tx0=ox>>sh,tx1=(ox+255)>>sh;
    memset(gbuf,0,256*ZH);
    int r8=sh-3;
    for(int sy=0;sy<ZH;sy++){
        int v=oy+sy, ty=v>>sh;
        if(ty<0||ty>=WH) continue;
        int ry=(v&(S-1))>>r8;
        u8*d=gbuf+sy*256;
        for(int tx=tx0;tx<=tx1;tx++){
            if(tx<0||tx>=WW) continue;
            const u8*a=zArtOf(tx,ty)+ry*8;
            int sx=(tx<<sh)-ox;
            for(int i=0;i<8;i++){
                u8 c=a[i], dummy=c; (void)dummy;
                int x0=sx+(i<<r8);
                for(int q=0;q<(1<<r8);q++){
                    int xx=x0+q;
                    if((unsigned)xx<256) d[xx]=c;
                }
            }
        }
    }
}
static void checkTiles(void){
    const u8*tiles=(const u8*)BG_TILE_RAM(1);
    for(int ty=0;ty<ZH/8;ty++) for(int tx=0;tx<32;tx++){
        const u8*t=tiles+(ty*32+tx)*64;
        for(int y=0;y<8;y++){
            const u8*src=gbuf+(ty*8+y)*256+tx*8;
            if(memcmp(t+y*8,src,8)){
                fprintf(stderr,"VRAM differs at tile (%d,%d), row %d\n",tx,ty,y);
                errors++;return;
            }
        }
    }
}
static void testNear(int level){
    zSet(level); tick=200; zForce=1; zArtLookups=0;
    CK(zDraw()==1,"initial zoom draw missing");
    int lookups=zArtLookups;
    CK(lookups<=240,"art resolved once per tile row (rather than once per scanline)");
    memcpy(zoomSaved,gbuf,sizeof zoomSaved);
    legacyTerrain();
    CK(!memcmp(zoomSaved,gbuf,sizeof zoomSaved),"CLOSE/MICRO terrain differs from original pixel output");
    memcpy(gbuf,zoomSaved,sizeof zoomSaved);
    int uploaded=zUploadN, skipped=zSkipN;
    zFlush(); CK(zUploadN-uploaded==32*(ZH/8),"first zoom flush must populate every tile");
    checkTiles();
    uploaded=zUploadN; skipped=zSkipN;
    zFlush();
    CK(zUploadN==uploaded&&zSkipN-skipped==32*(ZH/8),"identical picture must skip every VRAM upload");
    tick=201; CK(zDraw()==0,"interleaved close frame should skip composition");
    tick=202; CK(zDraw()==1,"next close frame should refresh animation");
    uploaded=zUploadN; zFlush(); CK(zUploadN==uploaded,"unchanged image should need no VRAM uploads");
    zForce=1; CK(zDraw()==1,"forced edit must repaint immediately");
    zFlush();checkTiles();
    printf("zoom %s: %d art lookups, first upload 672, unchanged upload 0\n",zNm[level],lookups);
}
int main(void){
    mapPtr=(u16*)bgGetMapPtr(0);
    planetSize=192; rs=43897; makeSpecies(); makeItems(); newWorld();
    // Isolate terrain so exact image equivalence is testable; sprite overlays
    // are tested via the complete VRAM-vs-framebuffer check below.
    nCr=0; memset(occ,0,sizeof occ); for(int b=0;b<8;b++) boat[b].on=0;
    camX=WW*4; camY=WH*4; zl=Z_NORMAL; zKey=-1;
    testNear(Z_CLOSE);
    testNear(Z_MICRO);
    // Move the view, edit the ground and draw a creature to exercise the
    // partial-tile edges and upload signatures after scene changes.
    zcx+=13; zcy+=8; zClamp(); zSyncCam();
    {int tx=zcx/8,ty=zcy/8; world[ty][tx]=T_SAND;
     Cr*c=&cr[0]; memset(c,0,sizeof *c); c->sp=0; c->x=(coord_t)tx; c->y=(coord_t)ty; c->alive=1; nCr=1; occ[ty][tx]=1;}
    tick+=2; zForce=1; CK(zDraw()==1,"moving camera / changing world must repaint");
    zFlush();checkTiles();
    zSet(Z_FAR);tick+=4;CK(zDraw()==1,"far view failed");zFlush();checkTiles();
    printf(errors?"ZOOM PERF FAIL (%d errors)\n":"ZOOM PERF OK\n",errors);
    return errors?1:0;
}
