// GodGuy 3DS: physically grounded two-person cinematic choreography.
// One authored dialogue cue and one cached partner per paired shot.
// No new assets, allocations, floating point, or per-frame terrain search.
#ifndef GG_FILM_PARTNER3D_H
#define GG_FILM_PARTNER3D_H
enum {GG_FILM_PAIR_NONE,GG_FILM_PAIR_COMFORT,
      GG_FILM_PAIR_BOOK,GG_FILM_PAIR_PARCEL};
static int ggFilmPairCue(const char *first,const char *second){
    // Contradictory coercion/seizure language must never be directed as
    // a friendly handoff or reassuring clasp; story consequences matter.
    if(strstr(first,"seize")||strstr(second,"seize")||
       strstr(first,"assault")||strstr(second,"assault")||
       strstr(first,"betray")||strstr(second,"betray")||
       strstr(first,"not trust")||strstr(second,"not trust")||
       strstr(first,"secret pages")||strstr(second,"secret pages"))
       return GG_FILM_PAIR_NONE;
    // Share a specific physical object only if the story establishes
    // giving/public access or food provision. Seizing or conquering
    // supplies must not accidentally play a friendly handover.
    int publicAct=strstr(first,"public")||strstr(second,"public")||
        strstr(first,"preserve")||strstr(second,"preserve")||
        strstr(first,"rescued")||strstr(second,"rescued")||
        strstr(first,"remain where")||strstr(second,"remain where")||
        strstr(first,"copied")||strstr(second,"copied")||
        strstr(first,"opened for")||strstr(second,"opened for");
    int book=strstr(first,"records")||strstr(second,"records")||
             strstr(first,"names")||strstr(second,"names")||
             strstr(first,"pages")||strstr(second,"pages");
    if(publicAct&&book)return GG_FILM_PAIR_BOOK;
    if((strstr(first,"feed")||strstr(second,"feed")||
        strstr(first,"delivered")||strstr(second,"delivered"))&&
       (strstr(first,"wagon")||strstr(second,"wagon")||
        strstr(first,"provisions")||strstr(second,"provisions")||
        strstr(first,"supplies")||strstr(second,"supplies")))
       return GG_FILM_PAIR_PARCEL;
    if(strstr(first,"families")||strstr(second,"families")||
       strstr(first,"children")||strstr(second,"children")||
       strstr(first,"wounded")||strstr(second,"wounded")||
       strstr(first,"peace")||strstr(second,"peace")||
       strstr(first,"home")||strstr(second,"home")||
       strstr(first,"trust")||strstr(second,"trust")||
       strstr(first,"oath")||strstr(second,"oath"))
       return GG_FILM_PAIR_COMFORT;
    return GG_FILM_PAIR_NONE;
}
static int ggFilmPairSmooth(int t){
    if(t<0)t=0;
    if(t>256)t=256;
    return (t*t*(768-2*t))/(256*256);
}
// Interaction starts AFTER the hero's existing one-tile entrance,
// holds across a real two-line dialogue boundary, then retreats smoothly.
static int ggFilmPairInset(int beat,int local,int beatTicks){
    if(beatTicks<=0)return 0;
    if(local<0)local=0;
    if(local>beatTicks)local=beatTicks;
    int t=(beat&1)*beatTicks+local;
    int start=beatTicks/2,arrive=beatTicks-8;
    int leave=beatTicks+beatTicks/3,end=beatTicks*2-8;
    if(t<=start||t>=end)return 0;
    if(t<arrive)return 96*ggFilmPairSmooth((t-start)*256/(arrive-start))/256;
    if(t<=leave)return 96;
    return 96*(256-ggFilmPairSmooth((t-leave)*256/(end-leave)))/256;
}
// Each participant moves <=96/256th of their OWN floor tile.
// Even at maximum reach they cannot enter the partner's reserved cell.
static void ggFilmPairPosition(int stageX,int stageY,
                               int guestX,int guestY,int inset,
                               int hero,int *worldX,int *worldY){
    int dx=(guestX-stageX)/256,dy=(guestY-stageY)/256;
    if(hero){
        *worldX=stageX+dx*inset;*worldY=stageY+dy*inset;
    }else{
        *worldX=guestX-dx*inset;*worldY=guestY-dy*inset;
    }
}
static int ggFilmPairDirection(int fromX,int toX){
    return toX>fromX?1:toX<fromX?-1:0;
}
static int ggFilmPairTouch(int ax,int ay,int bx,int by){
    int dx=ax-bx,dy=ay-by;
    return dx>=-18&&dx<=18&&dy>=-14&&dy<=14;
}
// One object exists for a shot. The receiver becomes its owner only
// after hand contact: no duplicated hand props or teleporting parcels.
static int ggFilmPairObjectOwner(int kind,int touched,int transferred){
    if(kind!=GG_FILM_PAIR_BOOK&&kind!=GG_FILM_PAIR_PARCEL)return -1;
    return touched||transferred?1:0;
}
#endif
