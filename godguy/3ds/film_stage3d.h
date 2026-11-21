// GodGuy 3DS film blocking: fixed 4-item painter queue and exclusive sides.
// Takes use one live-world center tile, up to two guests, and one grounded prop.
// No framebuffers, heap, sorting library, or per-frame world search.
#ifndef GG_FILM_STAGE3D_H
#define GG_FILM_STAGE3D_H
enum { GG_STAGE_HERO, GG_STAGE_GUEST_ONE, GG_STAGE_GUEST_TWO, GG_STAGE_PROP };
typedef struct {
    int kind,x,feet,size,depth;
} GgFilmStageItem;
// Return +1 for the right-hand tile, -1 for left, or 0 for offstage.
// The second guest has a side only if BOTH sides exist: no duplication
// when one side is blocked by citizens, buildings, water or a cliff.
static int ggFilmStageGuestSide(int plusValid,int minusValid,int slot){
    if(slot==0)return plusValid?1:minusValid?-1:0;
    return plusValid&&minusValid?-1:0;
}
// Static props must NEVER spawn under a guest's shoes. With two distinct
// guests both tiles are reserved; otherwise choose the free opposite tile.
static int ggFilmStagePropSide(int plusValid,int minusValid,int guests){
    if(guests>=2)return 0;
    if(guests<=0)return minusValid?-1:plusValid?1:0;
    int first=ggFilmStageGuestSide(plusValid,minusValid,0);
    if(first==1&&minusValid)return -1;
    if(first==-1&&plusValid)return 1;
    return 0;
}
// Film actors must share visually level ground, not merely traversable
// terrain. The gameplay climb limit is 19 world-height units, but allowing
// that in a close-up puts a guest on top of a tall rock tower with the
// protagonist standing below. A four-unit step remains subtle on screen.
static int ggFilmStageHeightOK(int stage,int neighbor){
    int delta=stage-neighbor;
    return delta>=-4&&delta<=4;
}
// A speaker looks toward their actual partner, not an arbitrary constant
// screen side. Signed 1px eye/head cues avoid adding costly rotated sprites.
static int ggFilmStageGaze(int ownSide,int targetSide){
    return targetSide>ownSide?1:targetSide<ownSide?-1:0;
}
static int ggFilmStageDepth(int worldX,int worldY,
                             int cameraX,int cameraY,int cosA,int sinA){
    return ((worldX-cameraX)*cosA+
            (worldY-cameraY)*sinA)/4096;
}
// Far-to-near painter order; stable for equal-depth actor ties.
// Count is always <=4, so insertion sort has at most six comparisons.
static void ggFilmStageSort(GgFilmStageItem *items,int count){
    for(int i=1;i<count;i++){
        GgFilmStageItem item=items[i];
        int j=i;
        while(j>0&&items[j-1].depth<item.depth){
            items[j]=items[j-1];
            j--;
        }
        items[j]=item;
    }
}
#endif
