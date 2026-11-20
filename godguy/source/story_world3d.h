// GodGuy 3DS chapter-film world stage.
// Fixed-camera perspective projection of authored world-space geometry.
// No GPU dependency, heap allocation, texture cache, z-buffer or new surface:
// triangles rasterize straight into the existing 256x192 indexed film page.
// Fixed draw order: far landscape -> structures -> terrain details -> cast.
// DS/DSi compile the original painted-plate renderer unchanged.
#ifndef GG_STORY_WORLD3D_H
#define GG_STORY_WORLD3D_H
#ifdef __3DS__

typedef struct { short x,y,z; } GgV3;
typedef struct { int x,y; } GgS2;
typedef struct { int x,z; } GgFilmCamera;

static GgV3 ggV3(int x,int y,int z){
    GgV3 v={(short)x,(short)y,(short)z}; return v;
}
static GgFilmCamera ggFilmCameraAt(const FilmBeat *beat,int move){
    GgFilmCamera c={0,0};
    if(beat->camera==F_TRACK){
        c.x=(move-128)/3;                 // 85 world units, lateral tracking
        c.z=(move-128)/5;
    } else if(beat->camera==F_WIDE){
        c.x=(move-128)/12;
        c.z=(move-128)/3;                 // physical dolly in depth
    }
    return c;
}
static GgS2 ggFilmProject(GgFilmCamera cam,GgV3 p){
    int depth=p.z-cam.z;
    if(depth<120)depth=120;               // no division by zero or behind-eye flip
    GgS2 q={128+((int)p.x-cam.x)*260/depth,
            68+(220-(int)p.y)*260/depth};
    return q;
}
// Integer scanline triangles with a per-row span; bounded to the film viewport.
// No per-pixel division and no temporary framebuffer.
static void ggFilmTri(GgS2 a,GgS2 b,GgS2 c,u8 ink){
    int miny=a.y,maxy=a.y;
    if(b.y<miny)miny=b.y; if(c.y<miny)miny=c.y;
    if(b.y>maxy)maxy=b.y; if(c.y>maxy)maxy=c.y;
    if(miny<FILM_TOP)miny=FILM_TOP;
    if(maxy>=FILM_BOTTOM)maxy=FILM_BOTTOM-1;
    if(miny>maxy)return;
    GgS2 p[3]={a,b,c};
    for(int y=miny;y<=maxy;y++){
        int xs[3],n=0;
        for(int i=0;i<3;i++){
            GgS2 v=p[i],w=p[(i+1)%3];
            if((y>=v.y&&y<w.y)||(y>=w.y&&y<v.y)){
                xs[n++]=v.x+(w.x-v.x)*(y-v.y)/(w.y-v.y);
            }
        }
        if(n<2)continue;
        int l=xs[0],r=xs[1];
        if(l>r){int tmp=l;l=r;r=tmp;}
        if(l<0)l=0;
        if(r>255)r=255;
        if(l<=r)memset(uiBuf+y*256+l,fmvSlots[ink],(size_t)(r-l+1));
    }
}
static void ggFilmQuad(GgFilmCamera cam,GgV3 a,GgV3 b,GgV3 c,GgV3 d,u8 ink){
    GgS2 aa=ggFilmProject(cam,a),bb=ggFilmProject(cam,b);
    GgS2 cc=ggFilmProject(cam,c),dd=ggFilmProject(cam,d);
    ggFilmTri(aa,bb,cc,ink);
    ggFilmTri(aa,cc,dd,ink);
}
static void ggFilmBox(GgFilmCamera cam,int x,int y,int z,int w,int h,int d,
                      u8 front,u8 side,u8 roof){
    int l=x-w/2,r=x+w/2,f=z-d/2,b=z+d/2,t=y+h;
    ggFilmQuad(cam,ggV3(l,y,b),ggV3(l,y,f),ggV3(l,t,f),ggV3(l,t,b),side);
    ggFilmQuad(cam,ggV3(r,y,f),ggV3(r,y,b),ggV3(r,t,b),ggV3(r,t,f),side);
    ggFilmQuad(cam,ggV3(l,y,f),ggV3(r,y,f),ggV3(r,t,f),ggV3(l,t,f),front);
    ggFilmQuad(cam,ggV3(l,t,f),ggV3(r,t,f),ggV3(r,t,b),ggV3(l,t,b),roof);
}
static void ggFilmBeam(GgFilmCamera cam,int x,int y,int z,int w,int h,int d,
                       u8 face){ggFilmBox(cam,x,y,z,w,h,d,face,24,25);}

// Facade is an actual set of surfaces at one depth. A real door is inset,
// and each shutter has a raised side and lintel, not scribbled-on pixels.
static void ggFilmHouse(GgFilmCamera cam,int x,int z,int w,int height,int lit){
    int f=z-43,top=height;
    ggFilmBox(cam,x,0,z,w,height,86,21,20,24);
    // Pitched roof: two slopes around a continuous ridge, plus gable.
    ggFilmQuad(cam,ggV3(x-w/2-8,top,f-7),ggV3(x,top+45,f-7),
                   ggV3(x,top+45,z+48),ggV3(x-w/2-8,top,z+48),24);
    ggFilmQuad(cam,ggV3(x,top+45,f-7),ggV3(x+w/2+8,top,f-7),
                   ggV3(x+w/2+8,top,z+48),ggV3(x,top+45,z+48),25);
    GgS2 l=ggFilmProject(cam,ggV3(x-w/2-8,top,f-7));
    GgS2 m=ggFilmProject(cam,ggV3(x,top+45,f-7));
    GgS2 r=ggFilmProject(cam,ggV3(x+w/2+8,top,f-7));
    ggFilmTri(l,m,r,20);
    // Eaves, braces and roof tile rows are world-space, obey camera dolly.
    for(int i=1;i<=3;i++){
        int tileZ=f-7+i*13;
        ggFilmQuad(cam,ggV3(x-w/2-8,top,tileZ),ggV3(x,top+45,tileZ),
                       ggV3(x,top+46,tileZ+2),ggV3(x-w/2-8,top+1,tileZ+2),26);
    }
    ggFilmBeam(cam,x-w/2+8,0,f-3,5,height,7,25);
    ggFilmBeam(cam,x+w/2-8,0,f-3,5,height,7,25);
    ggFilmBeam(cam,x,top-9,f-3,w-12,7,7,25);
    // Framed door, threshold, crossbar, shutters, warm window light.
    ggFilmQuad(cam,ggV3(x-14,0,f-5),ggV3(x+14,0,f-5),
                   ggV3(x+14,68,f-5),ggV3(x-14,68,f-5),2);
    ggFilmBeam(cam,x,64,f-8,34,6,5,24);
    ggFilmBeam(cam,x-17,0,f-8,5,72,5,24);
    ggFilmBeam(cam,x+17,0,f-8,5,72,5,24);
    for(int k=-1;k<=1;k+=2){
        int wx=x+k*(w/3);
        ggFilmQuad(cam,ggV3(wx-12,76,f-5),ggV3(wx+12,76,f-5),
                       ggV3(wx+12,101,f-5),ggV3(wx-12,101,f-5),lit?10:13);
        ggFilmBeam(cam,wx,87,f-8,3,28,4,25);
        ggFilmBeam(cam,wx,99,f-8,30,3,4,25);
        ggFilmBeam(cam,wx-15,75,f-8,4,30,5,24);
        ggFilmBeam(cam,wx+15,75,f-8,4,30,5,24);
    }
    // Chimney behind the near roof plane, with a 3-D lip.
    ggFilmBox(cam,x+w/4,top+20,z+21,14,39,18,20,21,25);
}
static void ggFilmTree(GgFilmCamera cam,int x,int z,int size,int t,int n){
    // Foliage has tapering facets, actual rear/front separation and subtle
    // anchored canopy flex. Trunk never teleports or follows the camera.
    ggFilmBox(cam,x,0,z,10,size/2,12,20,24,26);
    int sway=((t/31+n*5)%7)-3;
    for(int tier=0;tier<3;tier++){
        int y=size/3+tier*size/5;
        int spread=(size*3/5)-tier*size/8;
        GgV3 a=ggV3(x-spread/2,y,z-14);
        GgV3 b=ggV3(x+spread/2,y,z-14);
        GgV3 peak=ggV3(x+sway*(tier+1),y+size/3,z+6);
        ggFilmTri(ggFilmProject(cam,a),ggFilmProject(cam,b),
                  ggFilmProject(cam,peak),tier&1?17:18);
        ggFilmQuad(cam,a,peak,ggV3(x+sway*(tier+1),y+size/3,z+24),
                   ggV3(x-spread/2,y,z+24),16);
    }
}
static void ggFilmArch(GgFilmCamera cam,int x,int z,int broken){
    int h=broken?120:169;
    ggFilmBox(cam,x-82,0,z,35,h,42,21,20,26);
    ggFilmBox(cam,x+82,0,z,35,broken?100:h,42,21,20,26);
    if(!broken)ggFilmBox(cam,x,h-24,z,204,36,45,21,24,26);
    for(int i=0;i<4;i++){
        ggFilmBeam(cam,x-82,22+i*32,z-25,43,4,4,25);
        ggFilmBeam(cam,x+82,22+i*25,z-25,43,4,4,25);
    }
    if(!broken){
        ggFilmBox(cam,x,h+11,z,218,10,50,25,24,26);
        // Recessed panel, stone crown and key block.
        ggFilmBox(cam,x,h-12,z-25,28,32,7,24,25,26);
    }
}
static void ggFilmFloor(GgFilmCamera cam,int p,int t){
    int wet=(p==F_FORD||p==F_SHORE||p==F_STORM);
    u8 base=wet?13:(p==F_HEARTH||p==F_VILLAGE?20:21);
    ggFilmQuad(cam,ggV3(-850,0,330),ggV3(850,0,330),
                   ggV3(850,0,1800),ggV3(-850,0,1800),base);
    // Receding perspective seams: narrow planes in 3D, not screen stripes.
    for(int i=0;i<5;i++){
        int z=480+i*165;
        ggFilmQuad(cam,ggV3(-850,1,z),ggV3(850,1,z),
                       ggV3(850,1,z+3),ggV3(-850,1,z+3),wet?14:25);
    }
    for(int x=-480;x<=480;x+=160){
        ggFilmQuad(cam,ggV3(x,1,370),ggV3(x+3,1,370),
                       ggV3(x+3,1,1650),ggV3(x,1,1650),wet?13:24);
    }
    if(wet){
        // The water moves in plane at a stable height; far ripples have
        // naturally smaller screen spans than near ones.
        for(int i=0;i<9;i++){
            int z=510+i*93, x=((i*193+t/4)%700)-350;
            ggFilmQuad(cam,ggV3(x,2,z),ggV3(x+45+i%3*12,2,z),
                           ggV3(x+45+i%3*12,2,z+5),ggV3(x,2,z+5),
                           i&1?14:13);
        }
    }
}
// A complete timber crossing has raised deck boards, stone abutments,
// diagonal bracing, load-bearing pillars and hand rails. It occupies the
// same ford coordinates from shot to shot, and appears only after the story
// has actually introduced Nessa's engineering work.
static void ggFilmBridge(GgFilmCamera cam,int damaged,int t){
    for(int i=0;i<8;i++){
        int z=510+i*88;
        int sway=damaged&&i>4?(i-4)*3:0;
        ggFilmBox(cam,sway,23,z,192,11,86,24,20,25);
        // Raised outer strakes and individually supported tread boards.
        for(int x=-77;x<=77;x+=22)
            ggFilmBox(cam,x+sway,34,z,17,3,77,26,24,25);
        for(int side=-1;side<=1;side+=2){
            ggFilmBox(cam,side*99+sway,0,z,16,53,14,20,24,26);
            ggFilmBeam(cam,side*94+sway,63,z,10,12,92,25);
            ggFilmQuad(cam,ggV3(side*100+sway,17,z+32),
                ggV3(side*86+sway,42,z+32),
                ggV3(side*86+sway,50,z+37),
                ggV3(side*100+sway,17,z+37),20);
        }
    }
    // Continuous handrails, not disconnected decorative lines.
    for(int side=-1;side<=1;side+=2){
        ggFilmBox(cam,side*95,73,832,12,9,700,25,24,26);
        ggFilmBox(cam,side*103,0,510,28,28,80,21,20,25);
        ggFilmBox(cam,side*103,0,1150,28,28,80,21,20,25);
    }
    // Rescue lamps are rigidly mounted to the bridge rather than flying
    // through the camera plane. One lamp flickers on the far parapet.
    ggFilmBox(cam,-99,79,784,12,19,12,20,24,25);
    ggFilmBox(cam,-99,94,784,16,17,16,(t/20)&1?10:9,24,26);
}
// People walking or repairing within a location are small 3D figures:
// heads, tunics, arms and articulated legs all share a world-space base.
static void ggFilmBackgroundWorker(GgFilmCamera cam,int x,int z,int t,int id){
    int travel=((t/12+id*19)%89)-44;
    x+=travel;
    int stride=((t/17+id)&1)?8:-8;
    int stance=0;
    ggFilmBeam(cam,x-10,stance,z,13,47+stride/2,13,24);
    ggFilmBeam(cam,x+11,stance,z,13,47-stride/2,13,24);
    ggFilmBox(cam,x,47,z,37,57,22,id&1?17:13,20,26);
    // Polygonal neck/head, set within a darker hood.
    ggFilmBox(cam,x,104,z-1,26,31,24,26,24,25);
    ggFilmBox(cam,x,123,z+1,36,13,26,id&1?20:16,24,25);
    ggFilmBeam(cam,x-28,58,z,12,42+stride/2,10,20);
    ggFilmBeam(cam,x+28,58,z,12,42-stride/2,10,20);
    if(id==1){
        // The engineer's tool moves about a fixed hand, never floats.
        int lift=((t/23)&1)?9:0;
        ggFilmBeam(cam,x+32,85+lift,z-8,7,43,7,24);
        ggFilmBox(cam,x+32,119+lift,z-8,28,11,13,21,24,26);
    }
}
static void ggFilmInterior(GgFilmCamera cam,int p,int t){
    // Walls, cornice and corridor all converge on the same perspective.
    ggFilmQuad(cam,ggV3(-700,0,1600),ggV3(700,0,1600),
                   ggV3(700,390,1600),ggV3(-700,390,1600),22);
    ggFilmQuad(cam,ggV3(-700,0,350),ggV3(-700,0,1600),
                   ggV3(-700,390,1600),ggV3(-700,390,350),20);
    ggFilmQuad(cam,ggV3(700,0,1600),ggV3(700,0,350),
                   ggV3(700,390,350),ggV3(700,390,1600),21);
    ggFilmFloor(cam,p,t);
    ggFilmBox(cam,0,235,1562,820,13,40,25,24,26);
    for(int x=-460;x<=460;x+=230){
        ggFilmBox(cam,x,0,1190,28,270,32,21,24,26);
        ggFilmBox(cam,x,238,1186,49,22,42,25,24,26);
    }
    if(p==F_ARCHIVE){
        for(int x=-320;x<=320;x+=320){
            ggFilmBox(cam,x,15,1440,195,220,44,20,24,25);
            for(int y=54;y<=204;y+=53){
                ggFilmBox(cam,x,y,1408,199,7,11,25,24,26);
                for(int n=0;n<7;n++){
                    int bk=x-78+n*24;
                    ggFilmBox(cam,bk,y+7,1398,15,31+(n*11)%18,13,
                              n&1?16:10,24,25);
                }
            }
        }
    }else if(p==F_FORGE){
        // Glowing forge vent, stable chimney and an anvil with shaped horns.
        ggFilmBox(cam,245,0,1350,152,180,75,20,24,25);
        ggFilmBox(cam,245,43,1306,84,77,7,(t/18)&1?9:10,24,25);
        ggFilmBox(cam,-155,0,820,46,78,75,24,20,25);
        ggFilmBox(cam,-155,73,820,133,20,57,26,24,25);
        GgS2 tip=ggFilmProject(cam,ggV3(-253,80,797));
        GgS2 a=ggFilmProject(cam,ggV3(-220,72,797));
        GgS2 b=ggFilmProject(cam,ggV3(-219,93,797));
        ggFilmTri(tip,a,b,25);
    }else if(p==F_COUCH||p==F_HEARTH||p==F_PORCH){
        ggFilmBox(cam,175,18,1200,220,46,83,24,20,25);
        ggFilmBox(cam,175,59,1237,220,72,25,21,24,25);
        ggFilmBox(cam,58,30,1200,25,69,105,21,24,25);
        ggFilmBox(cam,292,30,1200,25,69,105,21,24,25);
    }else if(p==F_THRONE||p==F_HALL){
        ggFilmBox(cam,0,0,1470,270,32,136,24,20,25);
        ggFilmBox(cam,0,30,1460,155,112,40,25,24,26);
        if(p==F_THRONE){
            ggFilmBox(cam,0,126,1460,173,108,24,24,20,26);
            ggFilmBox(cam,-76,65,1440,24,61,70,21,24,26);
            ggFilmBox(cam,76,65,1440,24,61,70,21,24,26);
        }
    }
}
static void ggFilmWorldDraw(const FilmBeat *beat,int elapsed,int move,int chapter){
    int p=beat->place;
    if(p==F_ORBIT)return; // globe remains the spherical dedicated renderer
    GgFilmCamera cam=ggFilmCameraAt(beat,move);
    int inside=(p==F_FORGE||p==F_ARCHIVE||p==F_HALL||
                p==F_THRONE||p==F_COUCH||p==F_PORCH);
    if(inside){ggFilmInterior(cam,p,elapsed);return;}
    ggFilmFloor(cam,p,elapsed);
    if(p==F_GATE||p==F_RUINS){
        ggFilmArch(cam,-170,1260,p==F_RUINS);
        ggFilmArch(cam,225,970,p==F_RUINS);
        ggFilmBox(cam,0,0,1610,650,140,80,21,20,26);
    }else if(p==F_VILLAGE||p==F_HEARTH){
        // Persist the same village plan, but let the town actually GROW:
        // one founding shelter, then its first neighbor, then the hall.
        // Later stories never jump back to a vanished building.
        ggFilmHouse(cam,-340,1190,158,138,1);
        if(chapter>=2)ggFilmHouse(cam,325,1040,155,155,1);
        if(chapter>=5)ggFilmHouse(cam,25,1510,185,163,0);
        if(chapter>=10)ggFilmHouse(cam,-75,1800,190,177,1);
        ggFilmTree(cam,-530,910,158,elapsed,0);
        ggFilmTree(cam,515,1180,190,elapsed,2);
        if(chapter>=2)ggFilmBackgroundWorker(cam,-175,1070,elapsed,1);
        if(chapter>=10)ggFilmBackgroundWorker(cam,140,1280,elapsed,0);
    }else if(p==F_FORD||p==F_SHORE||p==F_STORM){
        ggFilmTree(cam,-385,1220,144,elapsed,0);
        ggFilmTree(cam,425,1160,151,elapsed,1);
        if(p==F_FORD){
            if(chapter>=8){
                ggFilmBridge(cam,chapter==9,elapsed);
                if(chapter>=10)ggFilmBackgroundWorker(cam,172,1120,elapsed,1);
            }else{
                for(int k=0;k<5;k++){
                    // The original raised stones remain where travelers
                    // walked before the bridge existed.
                    ggFilmBox(cam,(k%2?37:-39),0,510+k*112,
                              84,9,56,21,20,26);
                }
            }
        }
    }
    if(p==F_STORM){
        // Rain exists within the set's depth, not a fuzzy full-screen veil.
        for(int i=0;i<14;i++){
            int x=((i*131+elapsed/3)%900)-450;
            int z=480+(i*89)%950;
            int y=55+((i*97-elapsed/3)%210+210)%210;
            ggFilmQuad(cam,ggV3(x,y,z),ggV3(x+3,y,z),
                           ggV3(x+11,y+24,z),ggV3(x+8,y+24,z),22);
        }
    }
}
// Actors share the exact world camera used for geometry, so entering a room,
// approaching the foreground, and crossing the frame changes perspective
// rather than just dragging/zooming a flat sprite. Attachment and prop
// choreography still uses FilmBlocking's projected screen-space hand points.
static void ggFilmWorldBlocking(const FilmBeat *beat,int elapsed,int move,
                                FilmBlocking *b){
    if(beat->camera==F_INSERT||beat->place==F_ORBIT)return;
    GgFilmCamera cam=ggFilmCameraAt(beat,move);
    int paired=beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N;
    int leadLeft=paired&&filmActorLeftOf(beat->lead,beat->other);
    int compact=beat->camera==F_HAND&&paired&&beat->prop<F_PROP_N;
    int lx=paired?(leadLeft?-125:125):b->leadX<128?-125:125;
    int ox=paired?(leadLeft?125:-125):0;
    if(compact){lx=leadLeft?-74:74;ox=-lx;}
    int active=beat->lead<F_ACTOR_N;
    int journey=active&&(beat->camera==F_TRACK||
                         (beat->camera==F_WIDE&&beat->prop>=F_PROP_N));
    int arrive=journey?filmEase(elapsed,180):256;
    int away=journey?(256-arrive):0;
    int lz=690+away*165/256;
    int oz=690;
    int stride=journey&&elapsed<180?((elapsed/18)&1)*5:0;
    if(active){
        int drift=journey?(lx<0?-1:1)*away*66/256:0;
        GgS2 pt=ggFilmProject(cam,ggV3(lx+drift,stride,lz));
        b->leadX=pt.x;
        b->leadFeet=pt.y-(beat->lead==F_WISP?33:0);
        b->leadHeight=beat->lead==F_WISP?22:130*260/(lz-cam.z);
    }
    if(paired){
        GgS2 pt=ggFilmProject(cam,ggV3(ox,0,oz));
        b->otherX=pt.x;
        b->otherFeet=pt.y-(beat->other==F_WISP?33:0);
        b->otherHeight=beat->other==F_WISP?22:130*260/(oz-cam.z);
    }
}
#endif
#endif
