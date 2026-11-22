// Standalone 3DS world-stage geometry regression and CPU baseline.
// No generated movie data or Nintendo SDK is needed to validate projection,
// world occupancy, clipping, moving water, and depth-linked actor travel.
// Build: gcc -O2 -std=c11 -Wall -Wextra -D__3DS__ pc_world3d.c -o /tmp/ggworld && /tmp/ggworld
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
typedef uint8_t u8;
enum { FILM_TOP=24, FILM_BOTTOM=168, FILM_H=144, FMV_TICKS=5 };
enum { F_ORBIT,F_SHORE,F_STORM,F_VILLAGE,F_FORD,F_GATE,F_FORGE,
       F_HALL,F_ARCHIVE,F_HEARTH,F_THRONE,F_COUCH,F_PORCH,F_RUINS,F_SET_N };
enum { F_WIDE,F_NEAR,F_TRACK,F_HAND,F_INSERT };
enum { F_WISP,F_MIRA,F_GRUUL,F_ELOWEN,F_THORGRIM,F_HOLLOW,F_DAVE,
       F_ELDER,F_NESSA,F_SERIN,F_ACTOR_N,F_OFFSCREEN=255 };
enum { F_CUP,F_GOAT,F_HAMMER,F_SPOON,F_QUILL,F_LAMP,F_CROWN,F_LEAF,
       F_BREAD,F_MASK,F_CRUMB,F_SEED,F_PROP_N,F_NO_PROP=255 };
typedef struct { int place,camera,lead,other,prop; const char *line; } FilmBeat;
typedef struct { int leadX,otherX,leadFeet,otherFeet,leadHeight,otherHeight; } FilmBlocking;
static int filmEase(int elapsed,int total) {
    int u=elapsed>=total?256:(elapsed*256)/total;
    return (u*u*(768-2*u))>>16;
}
static int filmActorLeftOf(int a,int b){return a<b;}
static struct { u8 before[32],page[256*192],after[32]; } picture;
#define uiBuf picture.page
static u8 fmvSlots[32];
static int backgroundSpriteCalls;
static int backgroundFootX,backgroundFootY;
// Real firmware uses the decompressed 40x48 actor cels. Standalone geometry
// testing uses a minimal projected rectangle so it can verify the camera,
// animation timeline and memory bounds without depending on art generator.
static void filmDrawActor(int who,int pose,int x,int feet,int h,int flip){
    backgroundSpriteCalls++;
    backgroundFootX=x;
    backgroundFootY=feet;
    (void)pose;(void)flip;
    if(h<0||h>100)return;
    for(int y=feet-h;y<feet;y++)
        for(int px=x-h/5;px<x+h/5;px++)
            if(y>=FILM_TOP&&y<FILM_BOTTOM&&px>=0&&px<256)
                uiBuf[y*256+px]=(u8)(who+34);
}
#include "source/story_world3d.h"

static void require(int condition,const char *why){
    if(!condition){fprintf(stderr,"WORLD3D FAIL: %s\n",why);exit(1);}
}
static unsigned long hashFrame(void){
    unsigned long h=2166136261u;
    for(int i=FILM_TOP*256;i<FILM_BOTTOM*256;i++)
        h=(h^uiBuf[i])*16777619u;
    return h;
}
static void drawWorld(const FilmBeat *beat,int t,int move,int chapter,int roof,int winter){
    GgFilmCamera cam=ggFilmBeginOcclusion(ggFilmCameraAt(beat,move));
    ggFilmWorldDraw(cam,beat,ggFilmSceneTime(t),chapter,roof,winter);
    ggFilmWorldForeground(cam,beat,move,chapter);
}
static unsigned long frame(int place,int camera,int t){
    FilmBeat b={place,camera,F_MIRA,F_THORGRIM,F_NO_PROP};
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&b,t,filmEase(t,420),t/300,1,0);
    return hashFrame();
}
static int materialColors(const u8 *page){
    int used[256]={0};
    for(int i=FILM_TOP*256;i<FILM_BOTTOM*256;i++) used[page[i]]=1;
    int colors=0;
    for(int i=0;i<GG_FILM_MATERIALS;i++) colors+=used[ggFilmMaterialSlots[i]];
    return colors;
}
static void checkSlopingForeground(GgFilmCamera camera){
    memset(uiBuf,0,sizeof picture.page);
    camera=ggFilmBeginOcclusion(camera);
    // A wall slants away across the image. A performer at Z=650 can be
    // behind its left edge and in front of its right edge in the same shot.
    ggFilmQuad(camera,ggV3(-100,70,460),ggV3(180,70,860),
                ggV3(180,160,860),ggV3(-100,160,460),GG_STONE);
    const int y=105,left=91,right=165;
    require(uiBuf[y*256+left]==ggFilmInk(GG_STONE)&&
            uiBuf[y*256+right]==ggFilmInk(GG_STONE),"the sloping wall missed the test pixels");
    ggFilmCelDepth=ggFilmReciprocalDepth(650);
    ggFilmCelPixel(uiBuf+y*256,left,y,253);
    ggFilmCelPixel(uiBuf+y*256,right,y,253);
    ggFilmCelDepth=0;
    require(uiBuf[y*256+left]==ggFilmInk(GG_STONE)&&uiBuf[y*256+right]==253,
            "occlusion must distinguish the near and far edges of a sloping wall");
}
int main(void){
    memset(picture.before,0xa6,sizeof picture.before);
    memset(picture.after,0x6d,sizeof picture.after);
    for(int i=0;i<32;i++)fmvSlots[i]=(u8)(i+1);
    GgFilmCamera center={.page=uiBuf},moved={.x=64,.z=20,.yaw=18,.page=uiBuf};
    checkSlopingForeground(center);
    require(ggFilmSurfaceHeight(F_HALL,11,0,690)==5&&
            ggFilmSurfaceHeight(F_HALL,11,-246,690)==1,
            "actors must stand on the rug pile in the aisle and the tile beyond it");
    require(ggFilmSurfaceHeight(F_FORD,9,0,1240)>7&&
            ggFilmSurfaceHeight(F_FORD,9,0,1360)==1&&
            ggFilmSurfaceHeight(F_FORD,9,128,1240)==1,
            "the long bridge ramp must taper down to the bank without lifting the river margins");
    GgS2 hallProp=ggFilmPropSupport(center,F_HALL,6);
    GgS2 hallFloor=ggFilmProject(center,ggV3(0,5,760));
    require(hallProp.y<hallFloor.y-8,"the hall's props sit on the floor instead of a work table");
    require(ggFilmSurfaceHeight(F_VILLAGE,2,0,780)==3&&
            ggFilmSurfaceHeight(F_VILLAGE,2,-168,780)==0,
            "the village road and meadow incorrectly share one contact height");
    FilmBeat stormRoute={F_STORM,F_TRACK,F_HOLLOW,F_SERIN,F_LAMP,NULL};
    FilmBlocking stormCast={0};
    GgFilmFootprints stormArrival=ggFilmWorldBlocking(center,&stormRoute,0,4,&stormCast);
    require(ggFilmSurfaceHeight(F_STORM,4,-191,855)==0&&
            stormArrival.lead.z<839&&stormArrival.lead.y==3,
            "the storm walker starts in the surf at the height of dry sand");
    FilmBeat forgeTrack={F_FORGE,F_TRACK,F_NESSA,F_THORGRIM,F_HAMMER,NULL};
    FilmBlocking smithStart={0},smithEnd={0};
    GgFilmCamera smithCam=ggFilmCameraAt(&forgeTrack,0);
    GgFilmFootprints smithAtStart=ggFilmWorldBlocking(smithCam,&forgeTrack,0,5,&smithStart);
    GgFilmFootprints smithAtEnd=ggFilmWorldBlocking(smithCam,&forgeTrack,180,5,&smithEnd);
    require(smithAtStart.lead.z<GG_FILM_ANVIL_Z-GG_FILM_ANVIL_D/2&&
            smithAtEnd.lead.z<smithAtStart.lead.z,
            "a smith tracking with a hammer must stay on the front side of the anvil");
    GgS2 near=ggFilmProject(center,ggV3(120,0,560));
    GgS2 far=ggFilmProject(center,ggV3(120,0,1450));
    require(near.x>far.x&&far.x>128,"perspective does not reduce far displacement");
    require(near.y>far.y,"world-space floor has no depth gradient");
    require(ggFilmProject(moved,ggV3(120,0,560)).x!=near.x,
            "a camera track does not transform a point");
    // A full-size adult placed at the cottage threshold needs head and
    // shoulder clearance at both camera distances.
    const int cottageDepths[]={740,1050};
    for(unsigned i=0;i<sizeof cottageDepths/sizeof *cottageDepths;i++){
        int z=cottageDepths[i];
        memset(uiBuf,0,sizeof picture.page);
        ggFilmHouse(moved,0,z,216,196,1,1);
        GgS2 doorTop=ggFilmProject(moved,ggV3(0,144,z-68));
        GgS2 doorSide=ggFilmProject(moved,ggV3(-26,110,z-68));
        require(uiBuf[doorTop.y*256+doorTop.x]==ggFilmInk(25)&&
                uiBuf[doorSide.y*256+doorSide.x]==ggFilmInk(25),
                "the village cottage door cannot admit a standing adult");
    }
    memset(uiBuf,0,sizeof picture.page);
    unsigned long empty=hashFrame();
    int populated=0;
    for(int p=F_ORBIT;p<F_SET_N;p++){
        unsigned long h=frame(p,F_WIDE,0);
        populated+=(h!=empty);
        for(int i=FILM_TOP*256;i<FILM_BOTTOM*256;i++)
            require(uiBuf[i]!=0,"the world shot leaves part of its backdrop unpainted");
        require(picture.before[0]==0xa6&&picture.before[31]==0xa6&&
                picture.after[0]==0x6d&&picture.after[31]==0x6d,
                "world renderer wrote outside the 256x192 film surface");
    }
    require(populated==F_SET_N,"an original location lacks projected 3DS scenery");
    for(int i=0;i<F_SET_N;i++){
        frame(i,F_TRACK,420);
        require(materialColors(uiBuf)>=3,
                "a set still relies entirely on the shared actor and prop palette");
    }
    require(frame(F_PORCH,F_NEAR,0)!=frame(F_COUCH,F_NEAR,0),
            "the first-rain porch is still a generic couch stage");
    FilmBeat porch={F_PORCH,F_NEAR,F_MIRA,F_WISP,F_CUP};
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&porch,40,128,1,1,0);
    unsigned long rainPorch=hashFrame();
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&porch,40,128,14,1,0);
    require(hashFrame()!=rainPorch,
            "porch window never clears after the first-rain chapter");
    // The worksite has visibly exposed rafters before its completed roof.
    FilmBeat village={F_VILLAGE,F_WIDE,F_MIRA,F_THORGRIM,F_NO_PROP};
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&village,0,128,2,0,0);
    unsigned long roofOpen=hashFrame();
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&village,0,128,2,1,0);
    require(hashFrame()!=roofOpen,
            "the house roof did not change when construction finished");
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&village,44,128,14,1,1);
    unsigned long snowVillage=hashFrame();
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&village,44,128,14,1,0);
    require(snowVillage!=hashFrame(),
            "bonus winter stories have the same environment as summer");
    FilmBeat ford={F_FORD,F_WIDE,F_GRUUL,F_MIRA,F_CUP,NULL};
    GgFilmCamera fordCam=ggFilmCameraAt(&ford,128);
    FilmBlocking onFord={0},onBridge={0};
    GgFilmFootprints stone=ggFilmWorldBlocking(fordCam,&ford,180,3,&onFord);
    GgFilmFootprints marks=ggFilmWorldBlocking(fordCam,&ford,180,9,&onBridge);
    require(stone.other.y>3&&stone.other.y<marks.other.y,
            "the pre-bridge cast stands below its raised stepping stones");
    memset(uiBuf,0,sizeof picture.page);
    ggFilmFloor(fordCam,F_FORD,0,9,0);
    GgS2 nearBank=ggFilmProject(fordCam,ggV3(0,1,592));
    GgS2 farBank=ggFilmProject(fordCam,ggV3(0,1,1310));
    GgS2 channel=ggFilmProject(fordCam,ggV3(-200,0,780));
    require(uiBuf[nearBank.y*256+nearBank.x]==fmvSlots[18]&&
            uiBuf[farBank.y*256+farBank.x]==fmvSlots[17]&&
            uiBuf[channel.y*256+channel.x]==fmvSlots[13],
            "the projected crossing has to connect two dry banks across water");
    // The uphill end of the ramp is above the river and in front of the bank.
    // In the old render this point was empty without the water/backdrop pass.
    memset(uiBuf,0,sizeof picture.page);
    ggFilmBridge(fordCam,0,0);
    GgS2 ramp=ggFilmProject(fordCam,ggV3(0,20,1197));
    require(uiBuf[ramp.y*256+ramp.x]!=0,
            "the raised bridge ends without a ramp to the bank");
    FilmBeat farWorker={F_FORD,F_NEAR,F_MIRA,F_GRUUL,F_NO_PROP,NULL};
    GgFilmCamera workerCam=ggFilmCameraAt(&farWorker,128);
    GgS2 dryBank=ggFilmProject(workerCam,ggV3(165,1,1160));
    backgroundSpriteCalls=0;
    ggFilmWorldDraw(workerCam,&farWorker,0,10,1,0);
    require(backgroundSpriteCalls==1&&backgroundFootX>128&&backgroundFootY<dryBank.y,
            "the far-bank repair worker is standing in the river");
    GgS2 deck=ggFilmProject(fordCam,ggV3(-65,GG_FILM_BRIDGE_TOP,690));
    require(onBridge.otherX==deck.x&&onBridge.otherFeet==deck.y&&
            onBridge.otherFeet<onFord.otherFeet,
            "the sprite does not stand on its projected bridge deck");
    require(marks.other.y==GG_FILM_BRIDGE_TOP,
            "a bridge actor casts a shadow at river height");
    ggFilmCastShadow(fordCam,marks.other,F_FORD,9);
    require(uiBuf[deck.y*256+deck.x]==fmvSlots[24],
            "a bridge cast shadow lost its projection onto the deck");
    GgS2 windowShadow=ggFilmProject(center,ggV3(-24,2,638));
    memset(uiBuf,0,sizeof picture.page);
    ggFilmCastShadow(center,ggV3(0,1,700),F_PORCH,1);
    require(uiBuf[windowShadow.y*256+windowShadow.x]==ggFilmInk(24),
            "the porch shadow does not extend away from the window");
    GgS2 sunsetShadow=ggFilmProject(center,ggV3(42,3,568));
    memset(uiBuf,0,sizeof picture.page);
    ggFilmCastShadow(center,ggV3(0,2,690),F_GATE,9);
    require(uiBuf[sunsetShadow.y*256+sunsetShadow.x]==ggFilmInk(20),
            "the gate shadow does not extend away from the setting sun");
    memset(uiBuf,0,sizeof picture.page);
    ggFilmCastShadow(center,ggV3(0,2,690),F_SHORE,1);
    require(uiBuf[sunsetShadow.y*256+sunsetShadow.x]==ggFilmInk(26),
            "the shore shadow does not extend away from the setting sun");
    GgS2 window=ggFilmPropSupport(fordCam,F_PORCH,1);
    GgS2 tabletop=ggFilmProject(fordCam,ggV3(0,36,583));
    require(window.x==tabletop.x&&window.y==tabletop.y,
            "the cup loses its physical resting surface during camera movement");
    FilmBeat porchHand={F_PORCH,F_HAND,F_MIRA,F_THORGRIM,F_LAMP,NULL};
    GgFilmCamera porchCam=ggFilmCameraAt(&porchHand,128);
    FilmBlocking reach={0};
    GgFilmFootprints stand=ggFilmWorldBlocking(porchCam,&porchHand,120,2,&reach);
    porchCam=ggFilmBeginOcclusion(porchCam);
    ggFilmWorldDraw(porchCam,&porchHand,120,2,1,0);
    GgS2 top=ggFilmProject(porchCam,ggV3(-37,36,588));
    u8 wood=uiBuf[top.y*256+top.x];
    ggFilmCelDepth=ggFilmReciprocalDepth(stand.otherDepth);
    ggFilmCelPixel(uiBuf+top.y*256,top.x,top.y,253);
    ggFilmCelDepth=0;
    require(wood==ggFilmInk(GG_PLASTER)&&uiBuf[top.y*256+top.x]==wood,
            "an actor pixel can overwrite the porch tabletop");
    FilmBeat couch={F_COUCH,F_NEAR,F_DAVE,F_MIRA,F_NO_PROP,NULL};
    GgFilmCamera couchCam=ggFilmCameraAt(&couch,128);
    FilmBlocking seated={0};
    GgFilmFootprints sitter=ggFilmWorldBlocking(couchCam,&couch,0,12,&seated);
    GgS2 cushion=ggFilmProject(couchCam,ggV3(sitter.lead.x,32,879));
    require(abs(seated.leadFeet-seated.leadHeight*11/48-cushion.y)<=2,
            "the seated actor's hips miss the front of the couch cushion");
    require(frame(F_SHORE,F_NEAR,0)!=frame(F_SHORE,F_NEAR,180),
            "water is not animated within its world plane");
    require(frame(F_STORM,F_NEAR,0)!=frame(F_STORM,F_NEAR,180),
            "world-space rain did not animate");
    fordCam=ggFilmBeginOcclusion(fordCam);
    ggFilmWorldDraw(fordCam,&ford,120,9,1,0);
    GgS2 post=ggFilmProject(fordCam,ggV3(-103,50,500));
    ggFilmCelDepth=ggFilmReciprocalDepth(690);
    ggFilmCelPixel(uiBuf+post.y*256,post.x,post.y,253);
    ggFilmCelDepth=0;
    require(uiBuf[post.y*256+post.x]!=253,
            "the near bridge post cannot occlude a sprite behind it");

    static u8 alternate[256*192];
    memset(uiBuf,0x9b,sizeof picture.page);
    memset(alternate,0,sizeof alternate);
    fordCam.page=alternate;
    fordCam.depth=NULL;
    ggFilmWorldDraw(fordCam,&ford,40,9,1,0);
    require(alternate[FILM_TOP*256]!=0&&materialColors(alternate)>=3&&
            uiBuf[FILM_TOP*256]==0x9b,
            "an origin set overwrote the shared chapter film page");
    // Coverage continuity: two unrelated lines spoken by the SAME actor
    // should reuse the familiar three-quarter angle. A response from the
    // other side uses the companion angle; a wide two-shot restores the
    // establishing position. This avoids a jump-cut after every sentence.
    FilmBeat cutA={F_PORCH,F_NEAR,F_MIRA,F_THORGRIM,F_CUP};
    FilmBeat cutB={F_PORCH,F_NEAR,F_MIRA,F_THORGRIM,F_CUP};
    cutA.line="THE CUP IS STILL ON THE TABLE.";
    cutB.line="LOOK OUT THROUGH THE WINDOW.";
    GgFilmCamera camA=ggFilmCameraAt(&cutA,128);
    GgFilmCamera camB=ggFilmCameraAt(&cutB,128);
    require(camA.x==camB.x&&camA.z==camB.z,
            "new subtitle text unnecessarily changed the camera");
    FilmBeat reverse={F_PORCH,F_NEAR,F_THORGRIM,F_MIRA,F_CUP};
    GgFilmCamera reverseCam=ggFilmCameraAt(&reverse,128);
    require(camA.x!=reverseCam.x,
            "two-person conversation has no consistent reverse angle");
    FilmBeat anchor={F_PORCH,F_WIDE,F_MIRA,F_THORGRIM,F_CUP};
    GgFilmCamera wideCam=ggFilmCameraAt(&anchor,128);
    require(wideCam.x==0&&wideCam.z<camA.z,
            "returning wide shot did not restore the spatial anchor");
    FilmBeat tracking={F_PORCH,F_TRACK,F_MIRA,F_THORGRIM,F_NO_PROP};
    GgFilmCamera trackStart=ggFilmCameraAt(&tracking,0);
    GgFilmCamera trackEnd=ggFilmCameraAt(&tracking,256);
    require(trackStart.x!=trackEnd.x&&trackStart.z!=trackEnd.z,
            "motivated traveling shot never moves through the set");
    // The scene clock is not reset by changing dialogue takes.
    // Two different secondary performers are visible, but no one
    // duplicates the actual named actors delivering dialogue.
    FilmBeat life={F_VILLAGE,F_NEAR,F_ELOWEN,F_HOLLOW,F_NO_PROP,NULL};
    backgroundSpriteCalls=0;
    ggFilmWorldClock=280;
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&life,0,128,10,1,0);
    unsigned long firstStreet=hashFrame();
    require(backgroundSpriteCalls>=2,
            "town workers reverted to primitive polygon people");
    ggFilmWorldClock=460;
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&life,0,128,10,1,0);
    require(hashFrame()!=firstStreet,
            "market, foliage and villagers freeze when the shot time restarts");
    FilmBeat speaker={F_VILLAGE,F_NEAR,F_MIRA,F_THORGRIM,F_NO_PROP,NULL};
    backgroundSpriteCalls=0;
    drawWorld(&speaker,0,128,10,1,0);
    require(backgroundSpriteCalls==1,
            "same character duplicated as speaker and background worker");
    // The door has a real physical arc during the first rain sequence.
    ggFilmWorldClock=0;
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&porch,0,128,1,1,0);
    unsigned long entryClosed=hashFrame();
    ggFilmWorldClock=370;
    memset(uiBuf,0,sizeof picture.page);
    drawWorld(&porch,0,128,1,1,0);
    require(hashFrame()!=entryClosed,
            "first-rain porch door and curtains remain frozen");
    ggFilmWorldClock=-1;
    FilmBeat walk={F_VILLAGE,F_WIDE,F_MIRA,F_THORGRIM,F_NO_PROP};
    FilmBlocking a={85,171,156,156,45,45},b=a;
    ggFilmWorldBlocking(ggFilmCameraAt(&walk,0),&walk,0,2,&a);
    ggFilmWorldBlocking(ggFilmCameraAt(&walk,256),&walk,180,2,&b);
    require(a.leadHeight<b.leadHeight&&a.leadFeet<b.leadFeet,
            "walking actor never approaches in Z");
    unsigned long checksum=0;
    clock_t started=clock();
    for(int t=0;t<480;t+=3){
        FilmBeat heavy={t<160?F_FORD:t<320?F_VILLAGE:F_ARCHIVE,
                        F_TRACK,F_ELDER,F_HOLLOW,F_NO_PROP,NULL};
        drawWorld(&heavy,t,filmEase(t,480),t<160?9:14,1,0);
        checksum^=hashFrame();
    }
    double ms=1000.0*(clock()-started)/CLOCKS_PER_SEC/160.0;
    require(picture.before[17]==0xa6&&picture.after[17]==0x6d,"canaries damaged");
    printf("WORLD3D OK: %d populated sets, animated geometry, ground travel, "
           "160 frames / %.3f ms per frame on host CPU, checksum %lu\n",
           populated,ms,checksum);
    return 0;
}
