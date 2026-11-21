// Deterministic 3DS film walk-ins in the existing live-world Q8 grid.
// Geometry is solved once per two-line shot; interpolation allocates nothing.
#ifndef GG_FILM_WALK3D_H
#define GG_FILM_WALK3D_H
static int ggFilmWalkProgress(int beat,int local,int beatTicks){
    if(beatTicks<=0)return 256;
    int elapsed=(beat&1)?beatTicks:local*2;
    if(elapsed<0)elapsed=0;
    if(elapsed>beatTicks)elapsed=beatTicks;
    int u=elapsed*256/beatTicks;
    return (u*u*(768-2*u))/(256*256);
}
static inline void ggFilmWalkWorldPosition(int stageX,int stageY,
                                     int approachX,int approachY,
                                     int cosA,int sinA,int beat,
                                     int local,int beatTicks,
                                     int *worldX,int *worldY){
    int u=ggFilmWalkProgress(beat,local,beatTicks);
    if(approachX!=stageX||approachY!=stageY){
        *worldX=approachX*256+128+(stageX-approachX)*u;
        *worldY=approachY*256+128+(stageY-approachY)*u;
    }else{
        // A blocked approach must never place a performer inside an
        // occupied neighboring cell. Stay within the cleared stage tile.
        int remaining=256-u;
        *worldX=stageX*256+128-(cosA*64/4096)*remaining/256;
        *worldY=stageY*256+128-(sinA*64/4096)*remaining/256;
    }
}
#endif
