// GodGuy cutscene world-vista sampler.
// Samples actual terrain and building cells when a film begins, NOT a
// second procedural map. Up to three 25x9 windows (~1.4 KiB combined).
// DS/DSi/3DS all use these same snapshots, never new framebuffer assets.
// No save changes, heap, per-frame world scans, or camera randomization.
#ifndef GG_STORY_VISTAS_H
#define GG_STORY_VISTAS_H
enum { FILM_VISTA_TOWN, FILM_VISTA_COAST, FILM_VISTA_RIDGE,
       FILM_VISTA_N=3, FILM_VISTA_W=25, FILM_VISTA_H=9 };
typedef struct {
    u8 terrain[FILM_VISTA_W*FILM_VISTA_H];
    u8 buildings[FILM_VISTA_W*FILM_VISTA_H];
    unsigned short x,y;
    u8 valid,spacing;
} FilmVista;
static FilmVista filmVista[FILM_VISTA_N];

static int filmVistaRead(const FilmVista *v,int x,int y,int building){
    if(!v->valid||(unsigned)x>=FILM_VISTA_W||
       (unsigned)y>=FILM_VISTA_H)return 0;
    return building?v->buildings[y*FILM_VISTA_W+x]:
                    v->terrain[y*FILM_VISTA_W+x];
}
static int filmVistaPlace(int place){
    if(place==F_VILLAGE||place==F_GATE||place==F_HALL)
        return FILM_VISTA_TOWN;
    if(place==F_SHORE||place==F_STORM||place==F_FORD)
        return FILM_VISTA_COAST;
    if(place==F_RUINS)return filmVista[FILM_VISTA_RIDGE].valid?
                              FILM_VISTA_RIDGE:FILM_VISTA_TOWN;
    return -1; // never paint distant scenery through an interior wall
}
static int filmVistaIsWater(int t){
    return t==T_DEEP||t==T_WATER;
}
static int filmVistaIsRidge(int t){
    return t==T_HILL||t==T_MOUNT;
}
static void filmVistaSample(FilmVista *v,int cx,int cy,int spacing,
                            int width,int height,
                            int (*cell)(int,int,int)){
    v->valid=width>0&&height>0&&cx>=0&&cy>=0&&
             cx<width&&cy<height;
    v->x=(unsigned short)(cx<0?0:cx);
    v->y=(unsigned short)(cy<0?0:cy);
    v->spacing=(u8)spacing;
    for(int j=0;j<FILM_VISTA_H;j++)for(int i=0;i<FILM_VISTA_W;i++){
        int x=cx+(i-FILM_VISTA_W/2)*spacing;
        int y=cy+(j-FILM_VISTA_H/2)*spacing;
        int index=j*FILM_VISTA_W+i;
        if(x<0||x>=width||y<0||y>=height){
            v->terrain[index]=T_DEEP;
            v->buildings[index]=0;
        }else{
            v->terrain[index]=(u8)cell(x,y,0);
            v->buildings[index]=(u8)cell(x,y,1);
        }
    }
}
// Find a real coastal or mountain landmark near the chosen village and
// current game camera. Scores prefer an actual sand/water edge, a high
// ridge, and mountain-to-water cliffs when they exist. A missing landmark
// is flagged unavailable; the film does not fabricate geography.
static int filmVistaFind(int centerX,int centerY,int width,int height,
                         int kind,int(*cell)(int,int,int),
                         int *bestX,int *bestY){
    int best=-999999;
    for(int dy=-64;dy<=64;dy+=2)for(int dx=-64;dx<=64;dx+=2){
        int x=centerX+dx,y=centerY+dy;
        if(x<2||y<2||x>=width-2||y>=height-2)continue;
        int t=cell(x,y,0);
        int water=0;
        for(int j=-2;j<=2;j++)for(int i=-2;i<=2;i+=2)
            if(filmVistaIsWater(cell(x+i,y+j,0)))water++;
        int score=-((dx*dx+dy*dy)/12);
        if(kind==FILM_VISTA_COAST){
            if(!water||filmVistaIsWater(t))continue;
            if(t==T_SAND)score+=650;
            else if(t==T_GRASS||t==T_DIRT)score+=360;
            else if(filmVistaIsRidge(t))score+=220;
            else continue;
            score+=water*7;
        }else{
            if(!filmVistaIsRidge(t)&&t!=T_SNOW)continue;
            score+=t==T_MOUNT?560:t==T_HILL?320:200;
            if(water)score+=water*10+80;
        }
        if(score>best){best=score;*bestX=x;*bestY=y;}
    }
    return best>-999999;
}
static void filmVistaCapture(int width,int height,int townX,int townY,
                             int cameraX,int cameraY,int(*cell)(int,int,int)){
    int bx=townX,by=townY;
    filmVistaSample(&filmVista[FILM_VISTA_TOWN],bx,by,1,
                    width,height,cell);
    for(int kind=FILM_VISTA_COAST;kind<FILM_VISTA_N;kind++){
        int x=0,y=0,valid=filmVistaFind(bx,by,width,height,kind,cell,&x,&y);
        if(!valid)valid=filmVistaFind(cameraX,cameraY,width,height,
                                     kind,cell,&x,&y);
        filmVistaSample(&filmVista[kind],valid?x:-1,valid?y:-1,2,
                        width,height,cell);
    }
}
#endif
