// The Unwritten Reign — optional 3DS world stories. These twelve
// three-beat cinematics are encountered through the actual terrain,
// settlements, professions, political progress and completed journeys.
// Never gate movement/career/progression on viewing a scene. Persistent
// seen bits prevent repeats and a cooldown prevents cinematic spam.
#ifndef GG_REIGN_AMBIENT3D_H
#define GG_REIGN_AMBIENT3D_H
#ifndef GG_AMBIENT_SAVE_PATH
#define GG_AMBIENT_SAVE_PATH "sdmc:/godguy_world_encounters.dat"
#endif
enum { GG_AMBIENT_TOWN,GG_AMBIENT_FOREST,GG_AMBIENT_COAST,
       GG_AMBIENT_RUIN,GG_AMBIENT_MARKET,GG_AMBIENT_FARM,
       GG_AMBIENT_CLINIC,GG_AMBIENT_ARCHIVE,GG_AMBIENT_SQUAD,
       GG_AMBIENT_OATH,GG_AMBIENT_DELIVERY,GG_AMBIENT_MIRACLE };
typedef struct {
    u32 seen;
    int loaded,cooldown,lastX,lastY,lastSignature;
} GgAmbient3D;
static GgAmbient3D ggAmbient;
typedef struct {u32 magic,version,seen,checksum;} GgAmbientDisk;
static void ggAmbientSave(void){
    GgAmbientDisk d={0x47414D42u,1,ggAmbient.seen,0};
    d.checksum=d.magic^d.version^d.seen^0x6C3A4251u;
    FILE *f=fopen(GG_AMBIENT_SAVE_PATH,"wb");
    if(f){(void)fwrite(&d,sizeof d,1,f);fclose(f);}
}
static void ggAmbientBegin(void){
    if(!ggAmbient.loaded){
        ggAmbient.loaded=1;ggAmbient.seen=0;
        FILE *f=fopen(GG_AMBIENT_SAVE_PATH,"rb");
        if(f){
            GgAmbientDisk d;
            int ok=fread(&d,sizeof d,1,f)==1;
            fclose(f);
            if(ok&&d.magic==0x47414D42u&&d.version==1&&
               d.checksum==(d.magic^d.version^d.seen^0x6C3A4251u)&&
               !(d.seen&~((1u<<GG_AMBIENT_N)-1)))
                ggAmbient.seen=d.seen;
        }
    }
    ggAmbient.cooldown=180;
    ggAmbient.lastX=ggAmbient.lastY=-99;
    ggAmbient.lastSignature=-99;
}
static int ggAmbientBuildingNear(int x,int y,int building){
    for(int dy=-4;dy<=4;dy++)for(int dx=-4;dx<=4;dx++){
        int xx=x+dx,yy=y+dy;
        if((unsigned)xx<WW&&(unsigned)yy<WH&&
           bld[yy][xx]==building)return 1;
    }
    return 0;
}
static int ggAmbientCoastNear(int x,int y){
    for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){
        int xx=x+dx,yy=y+dy;
        if((unsigned)xx<WW&&(unsigned)yy<WH&&
           world[yy][xx]<=T_WATER)return 1;
    }
    return 0;
}
static int ggAmbientTick(void){
    if(ggAmbient.cooldown>0)ggAmbient.cooldown--;
    if(!ggChron.active||ggChron.won||ggChron.lost||
       ggOrigin.active||ggSaga.mode||ggQuest.talk||ggReign.menu||
       ggChron.briefTime>0)return 0;
    if((ggChron.frame&31)!=0||ggAmbient.cooldown)return 0;
    const int x=ggExplore.px>>8,y=ggExplore.py>>8;
    if((unsigned)x>=WW||(unsigned)y>=WH)return 0;
    for(int k=0;k<ggChron.foeN;k++)if(ggChron.foe[k].hp>0&&
       ggChronNear(x,y,ggChron.foe[k].x,ggChron.foe[k].y)<=7)
        return 0; // combat is never interrupted by sightseeing
    int signature=ggQuest.completed*3+ggReign.role*7+
                  ggReign.office*31+ggReign.squadN*11;
    if(x==ggAmbient.lastX&&y==ggAmbient.lastY&&
       signature==ggAmbient.lastSignature)return 0;
    ggAmbient.lastX=x;ggAmbient.lastY=y;
    ggAmbient.lastSignature=signature;
    int town=ggReignVillage(x,y,7);
    int eligible[GG_AMBIENT_N]={0};
    eligible[GG_AMBIENT_TOWN]=town>=0;
    eligible[GG_AMBIENT_FOREST]=world[y][x]==T_FOREST||
          world[y][x]==T_JUNGLE;
    eligible[GG_AMBIENT_COAST]=ggAmbientCoastNear(x,y);
    eligible[GG_AMBIENT_RUIN]=ggAmbientBuildingNear(x,y,B_RUIN);
    eligible[GG_AMBIENT_MARKET]=town>=0&&vil[town].lvl>=2&&
          ggReign.role==GG_ROLE_TRADER;
    eligible[GG_AMBIENT_FARM]=ggReign.role==GG_ROLE_FARMER&&
          ggAmbientBuildingNear(x,y,B_FARM);
    eligible[GG_AMBIENT_CLINIC]=town>=0&&
          ggReign.role==GG_ROLE_HEALER;
    eligible[GG_AMBIENT_ARCHIVE]=ggReign.role==GG_ROLE_SCHOLAR&&
          ggAmbientBuildingNear(x,y,B_LIBRARY);
    eligible[GG_AMBIENT_SQUAD]=ggReign.squadN>0&&town>=0;
    eligible[GG_AMBIENT_OATH]=town>=0&&ggReign.office>=1;
    eligible[GG_AMBIENT_DELIVERY]=town>=0&&ggQuest.completed>0;
    eligible[GG_AMBIENT_MIRACLE]=ggReign.role==GG_ROLE_MYSTIC&&
          ggAmbientBuildingNear(x,y,B_TEMPLE);
    // Specific discoveries take precedence over the generic town
    // arrival, but all routes are optional and independent of act order.
    static const u8 order[GG_AMBIENT_N]={
        GG_AMBIENT_ARCHIVE,GG_AMBIENT_RUIN,GG_AMBIENT_CLINIC,
        GG_AMBIENT_FARM,GG_AMBIENT_MARKET,GG_AMBIENT_MIRACLE,
        GG_AMBIENT_DELIVERY,GG_AMBIENT_SQUAD,GG_AMBIENT_OATH,
        GG_AMBIENT_COAST,GG_AMBIENT_FOREST,GG_AMBIENT_TOWN
    };
    for(int i=0;i<GG_AMBIENT_N;i++){
        int scene=order[i];
        if(!eligible[scene]||(ggAmbient.seen&(1u<<scene)))continue;
        ggAmbient.seen|=1u<<scene;
        ggAmbientSave();
        ggAmbient.cooldown=720;
        ggSaga.mode=GG_SAGA_INTERLUDE;ggSaga.frame=0;
        ggSaga.paused=0;
        ggSaga.event=scene;ggSaga.eventX=x;ggSaga.eventY=y;
        ggSaga.lastKeys=0; 
        return 1;
    }
    return 0;
}
#endif
