// GodGuy step four: continuous player-world film journey.
// This is a tiny, read-only route through real tiles: settlement -> coast
// -> ridge when those landmarks exist. Camera position is expressed in
// Q8 game-map tiles, shared unchanged by DS, DSi and 3DS.
// The journey has one clock, no edit cuts, no procedural fake destinations,
// no save changes, heap, route mesh, or per-frame world search.
#ifndef GG_STORY_ROUTE_H
#define GG_STORY_ROUTE_H
typedef struct {
    int x[3],y[3],length[2],kind[3],total,count,duration;
} FilmRoute;
typedef struct {int x,y,dx,dy;} FilmRouteCamera;
static FilmRoute filmRoute;
static int filmRouteAbs(int x){return x<0?-x:x;}
static int filmRouteMax(int x,int y){return x>y?x:y;}
static int filmRouteClampi(int v,int low,int high){
    return v<low?low:v>high?high:v;
}
static void filmRouteCapture(int villageExists,int duration){
    FilmRoute *r=&filmRoute;
    r->count=r->total=r->duration=0;
    if(!villageExists||!filmVista[FILM_VISTA_TOWN].valid)return;
    r->x[0]=filmVista[FILM_VISTA_TOWN].x;
    r->y[0]=filmVista[FILM_VISTA_TOWN].y;
    r->kind[0]=FILM_VISTA_TOWN;
    r->count=1;
    // Each place must genuinely exist on the current planet.
    for(int kind=FILM_VISTA_COAST;kind<FILM_VISTA_N;kind++){
        if(!filmVista[kind].valid)continue;
        int x=filmVista[kind].x,y=filmVista[kind].y;
        int prev=r->count-1;
        int dx=filmRouteAbs(x-r->x[prev]);
        int dy=filmRouteAbs(y-r->y[prev]);
        int dist=filmRouteMax(dx,dy);
        // If already there, don't fly in circles or create an unnecessary
        // duplicate segment. A real coastline across the city still counts.
        if(dist<4)continue;
        r->length[prev]=dist;
        r->total+=dist;
        r->x[r->count]=x;
        r->y[r->count]=y;
        r->kind[r->count]=kind;
        if(++r->count==3)break;
    }
    if(r->count<2||r->total<4){
        r->count=r->total=0;
        return;
    }
    r->duration=duration;
}
static void filmRoutePosition(int t,int *x,int *y){
    const FilmRoute *r=&filmRoute;
    if(r->count<2||r->total<=0||r->duration<=0){
        *x=*y=0;return;
    }
    if(t<=0){*x=r->x[0]*256;*y=r->y[0]*256;return;}
    if(t>=r->duration){
        *x=r->x[r->count-1]*256;
        *y=r->y[r->count-1]*256;return;
    }
    // Distance-proportional movement keeps average travel speed constant
    // across each segment (not a sudden speed-up for the distant ridge).
    // Q8 distance retains sub-tile motion on every rendered frame;
    // moving one whole tile at a time would produce visible camera hops.
    int travel=(int)((long long)t*r->total*256/r->duration);
    if(r->count==3){
        // Blend the two straight flight legs with a quadratic corner
        // over at most four map tiles. This preserves the same town,
        // coast and cliff landmarks, but avoids a jarring instant
        // 180-degree camera turn at a hairpin in the real geography.
        int boundary=r->length[0]*256;
        int radius=filmRouteMax(0,
              filmRouteClampi(filmRouteMax(0,
                  r->length[0]<r->length[1]?
                  r->length[0]:r->length[1])*256/5,0,1024));
        if(radius>=128&&travel>=boundary-radius&&
           travel<=boundary+radius){
            int u=(travel-(boundary-radius))*256/(radius*2);
            int wx=r->x[1]*256,wy=r->y[1]*256;
            int sx=wx-(r->x[1]-r->x[0])*radius/r->length[0];
            int sy=wy-(r->y[1]-r->y[0])*radius/r->length[0];
            int ex=wx+(r->x[2]-r->x[1])*radius/r->length[1];
            int ey=wy+(r->y[2]-r->y[1])*radius/r->length[1];
            int ix=sx+(wx-sx)*u/256,iy=sy+(wy-sy)*u/256;
            int jx=wx+(ex-wx)*u/256,jy=wy+(ey-wy)*u/256;
            *x=ix+(jx-ix)*u/256;
            *y=iy+(jy-iy)*u/256;
            return;
        }
    }
    int segment=0;
    while(segment<r->count-2&&
          travel>=r->length[segment]*256){
        travel-=r->length[segment]*256;
        segment++;
    }
    int distance=r->length[segment];
    *x=r->x[segment]*256+
       (r->x[segment+1]-r->x[segment])*travel/distance;
    *y=r->y[segment]*256+
       (r->y[segment+1]-r->y[segment])*travel/distance;
}
static FilmRouteCamera filmRouteCameraAt(int t){
    FilmRouteCamera c;
    filmRoutePosition(t,&c.x,&c.y);
    int ax,ay,bx,by;
    // A rolling temporal lookahead turns gradually at the real coastline
    // rather than snapping into an unrelated director's angle.
    int delta=filmRoute.duration/35;
    if(delta<1)delta=1;
    filmRoutePosition(t-delta,&ax,&ay);
    filmRoutePosition(t+delta,&bx,&by);
    int dx=bx-ax,dy=by-ay;
    int scale=filmRouteMax(filmRouteAbs(dx),filmRouteAbs(dy));
    if(scale<=0){c.dx=256;c.dy=0;}
    else{c.dx=dx*256/scale;c.dy=dy*256/scale;}
    return c;
}
static int filmRouteTerrain(const FilmRouteCamera *camera,int f,int side,
                            int width,int height,int(*cell)(int,int,int),
                            int *building){
    // Rotate camera-relative tiles into actual planet coordinates. No
    // expensive float/trigonometry, no intermediate visible-world copy.
    int x=(camera->x+camera->dx*f-camera->dy*side+128)>>8;
    int y=(camera->y+camera->dy*f+camera->dx*side+128)>>8;
    x=filmRouteClampi(x,0,width-1);
    y=filmRouteClampi(y,0,height-1);
    *building=cell(x,y,1);
    return cell(x,y,0);
}
#endif
