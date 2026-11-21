// Offline capture of the actual 3DS-only procedural ending cinema.
// Reuses the game's 400x240 single-buffer renderer and source script.
// The output is raw RGB frames for ffmpeg; never linked into a handheld ROM.
#define main ggExploreHostRegressionMain
#include "pc_explore3d.c"
#undef main
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"wb");
    if(!f)return 2;
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++)
        world[y][x]=(x+y)%29==0?T_FOREST:T_GRASS;
    for(int y=52;y<81;y+=4)for(int x=56;x<91;x+=5)
        bld[y][x]=(x+y)%3?B_HOUSE:B_HALL;
    ggExpBegin(65,65,0);
    ggChron.active=1;ggChron.act=3;ggChron.post=3;
    ggChron.px[0]=63;ggChron.py[0]=65;
    ggChron.px[1]=75;ggChron.py[1]=69;
    ggChron.px[2]=84;ggChron.py[2]=72;
    ggOrigin.complete=1;ggOrigin.active=0;
    ggOrigin.skin=2;ggOrigin.hair=3;ggOrigin.garment=4;
    ggReign.loaded=1;ggReign.menu=0;
    ggSaga.loaded=1;ggSaga.mode=GG_SAGA_OUTRO;
    // Four frames per beat, spaced across its real-time run. This is a
    // preview montage of all nine possible IN-GAME ending film branches.
    for(int e=0;e<GG_SAGA_ENDINGS;e++){
        ggSaga.ending=e;
        for(int beat=0;beat<GG_SAGA_END_BEATS;beat++)
            for(int n=0;n<4;n++){
                ggSaga.frame=beat*GG_SAGA_BEAT_TICKS+
                             n*GG_SAGA_BEAT_TICKS/4;
                ggSagaDraw(guarded.img);
                if(fwrite(guarded.img,1,sizeof guarded.img,f)!=
                   sizeof guarded.img){fclose(f);return 3;}
            }
    }
    fclose(f);
    printf("UNWRITTEN REIGN FILM: %d ending branches, %d frames\n",
           GG_SAGA_ENDINGS,GG_SAGA_ENDINGS*GG_SAGA_END_BEATS*4);
    return 0;
}
