// Procedural biome and settlement decoration tests, strict SDK-free.
#include <stdio.h>
#include <stdlib.h>
enum {T_DEEP,T_WATER,T_SAND,T_GRASS,T_FOREST,T_HILL,T_MOUNT,
       T_SNOW,T_LAVA,T_DESERT,T_SWAMP,T_JUNGLE,T_DIRT,T_N};
enum {B_NONE,B_HUT,B_HOUSE,B_HALL,B_FARM,B_TOWER,B_RUIN,
       B_DOCK,B_TEMPLE,B_STATUE,B_LIBRARY,B_BORDER,B_N};
#include "3ds/film_scenery3d.h"
static void check(int okay,const char*why){
    if(!okay){fprintf(stderr,"3DS SCENERY FAIL: %s\n",why);exit(1);}
}
int main(void){
    int counts[16]={0};
    for(int y=31;y<53;y++)for(int x=19;x<41;x++){
        uint32_t a=ggFilmScenicHash(x,y,7);
        check(a==ggFilmScenicHash(x,y,7),"scenery changed after seek");
        check(a!=ggFilmScenicHash(x,y,8),"unseeded random props");
        int k=ggFilmScenicKind(T_FOREST,B_NONE,a);
        check(k>=GG_SCENE_NONE&&k<=GG_SCENE_STALL,"invalid forest prop");
        counts[k]++;
    }
    check(counts[GG_SCENE_OAK]>150&&counts[GG_SCENE_SHRUB]>100,
          "forest density remains sparse or uniform");
    // Reconstruct the actual 21x21 Q8 camera selection rule to guard
    // against saturating every visible object on the image's left.
    int left=0,right=0;
    for(int ty=55;ty<=75;ty++)for(int tx=54;tx<=74;tx++){
        uint32_t h=ggFilmScenicHash(tx,ty,7);
        if(((h>>24)&7u)>=5u)continue;
        int wx=tx*256+128+(int)((h>>12)&95u)-48;
        int wy=ty*256+128+(int)((h>>20)&95u)-48;
        int forward=wx-(64*256+128),side=wy-(65*256+128);
        if(forward<=128||forward>GG_SCENE_RADIUS*256||
           side<-forward*6/5||side>forward*6/5)continue;
        if(side<-100)left++;
        if(side>100)right++;
    }
    int difference=left-right;if(difference<0)difference=-difference;
    check(left>22&&right>22&&difference<20,
          "environment placement bias leaves an empty side of the frame");
    int farm=0,temple=0,town=0,ruin=0,coast=0;
    for(int i=0;i<16;i++){
        farm|=1<<ggFilmScenicKind(T_GRASS,B_FARM,(uint32_t)i);
        temple|=1<<ggFilmScenicKind(T_GRASS,B_TEMPLE,(uint32_t)i);
        town|=1<<ggFilmScenicKind(T_GRASS,B_HALL,(uint32_t)i);
        ruin|=1<<ggFilmScenicKind(T_GRASS,B_RUIN,(uint32_t)i);
        coast|=1<<ggFilmScenicKind(T_SWAMP,B_NONE,(uint32_t)i);
    }
    check(farm&(1<<GG_SCENE_CROPS)&&farm&(1<<GG_SCENE_FENCE),
          "farms lack fields and enclosures");
    check(temple&(1<<GG_SCENE_BRAZIER)&&
          temple&(1<<GG_SCENE_BANNER),"temple lacks ceremonial dressing");
    check(town&(1<<GG_SCENE_LAMP)&&town&(1<<GG_SCENE_STALL),
          "town looks like isolated blank building blocks");
    check(ruin&(1<<GG_SCENE_COLUMN)&&ruin&(1<<GG_SCENE_STONE),
          "ruins lost masonry and fallen debris");
    check(coast&(1<<GG_SCENE_REEDS),"wetlands lost reeds");
    for(int i=1;i<=GG_SCENE_STALL;i++)
        check(ggFilmScenicHeight(i)>=3&&
              ggFilmScenicHeight(i)<=23,"invalid prop height");
    check(GG_SCENE_RADIUS<=10&&GG_SCENE_LIMIT<=84,
          "unbounded per-frame neighborhood or draw budget");
    for(int radius=1;radius<=26;radius++)for(int band=0;band<9;band++){
        int width=ggFilmScenicCrownHalf(radius,band,0x92af116cu);
        check(width>=1&&width<=radius,
              "faceted foliage escapes bounded physical canopy");
    }
    check(ggFilmScenicCrownHalf(20,4,5u)>
          ggFilmScenicCrownHalf(20,0,5u),
          "near forest crowns still look like rectangular Atari blocks");
    check(sizeof(GgFilmScenicItem)*GG_SCENE_LIMIT<=2048,
          "scenic depth painter queue exceeds 2 KiB");
    GgFilmScenicItem samples[4]={
        {1,150,12,700,GG_SCENE_OAK,1},
        {2,150,12,1300,GG_SCENE_PINE,2},
        {3,150,12,1300,GG_SCENE_STONE,3},
        {4,150,12,400,GG_SCENE_SHRUB,4}
    };
    ggFilmScenicSort(samples,4);
    check(samples[0].depth==1300&&samples[0].key==2&&
          samples[1].depth==1300&&samples[1].key==3&&
          samples[2].depth==700&&samples[3].depth==400,
          "painter drew farther vegetation over nearer scenery or reordered ties");
    check(ggFilmScenicFloorDetail(T_DIRT,77,45)<
          ggFilmScenicFloorDetail(T_DIRT,120,45),
          "world-anchored cart ruts did not darken path");
    check(ggFilmScenicFloorDetail(T_GRASS,0,0)==
          ggFilmScenicFloorDetail(T_GRASS,0,0),
          "floor detail depends on frame time");
    int minGrain=100,maxGrain=-100,windLeft=0,windRight=0;
    // Offset samples from the tiny grass-stem lattice. Sampling
    // exclusively on exact 64-Q8 boundaries hit the same stem highlight
    // every time and incorrectly reported a flat floor.
    for(int y=39;y<1024;y+=64)for(int x=37;x<1024;x+=64){
        int g=ggFilmScenicFloorDetail(T_GRASS,x,y);
        if(g<minGrain)minGrain=g;
        if(g>maxGrain)maxGrain=g;
    }
    for(int frame=0;frame<700;frame+=14){
        int sway=ggFilmScenicWind(0x12af5783u,frame);
        if(sway<0)windLeft=1;
        if(sway>0)windRight=1;
        check(sway>=-1&&sway<=1&&
              sway==ggFilmScenicWind(0x12af5783u,frame),
              "world foliage flickers or drifts unpredictably");
    }
    check(maxGrain-minGrain>=13,
          "near world floor still a single flat, untextured color");
    check(windLeft&&windRight,
          "tree crowns never breathe or only sway one direction");
    puts("3DS SCENERY OK: biome-authored density, populated settlements, world-anchored textures and bounded scan");
    return 0;
}
