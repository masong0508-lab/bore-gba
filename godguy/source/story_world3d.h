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

// The long movie has one continuous scenery clock per chapter. The stage
// sets this from the complete clip position; direct geometry tests fall back
// to the local shot clock. Dialogue poses still use their local shot time.
static int ggFilmWorldClock=-1;
static int ggFilmSceneTime(int local){
    return ggFilmWorldClock>=0?ggFilmWorldClock:local;
}

static GgV3 ggV3(int x,int y,int z){
    GgV3 v={(short)x,(short)y,(short)z}; return v;
}
// Deterministic physical camera positions, not zooms of a painted plate.
// Similar dialogue cuts still observe the *same* room and landmarks, but
// from a distinct real angle. No random-per-frame camera jitter.
static int ggFilmShotIdentity(const FilmBeat *beat){
    unsigned id=(unsigned)beat->place*29u+(unsigned)beat->camera*17u+
                (unsigned)beat->lead*7u+(unsigned)beat->other*11u+
                (unsigned)beat->prop*3u;
    if(beat->line)
        for(int j=0;j<5&&beat->line[j];j++)
            id=id*33u+(unsigned char)beat->line[j];
    return (int)(id%63u);
}
static GgFilmCamera ggFilmCameraAt(const FilmBeat *beat,int move){
    int id=ggFilmShotIdentity(beat);
    GgFilmCamera c={0,0};
    // Distinct stable framing, even on five successive conversation beats
    // in the same house. Restrained x/Z values keep scene geography stable.
    c.x=((id%5)-2)*19;
    c.z=((id/5)%3-1)*31;
    if(beat->camera==F_TRACK){
        c.x+=(move-128)/3;  // purposeful world-space tracking
        c.z+=(move-128)/5;
    }else if(beat->camera==F_WIDE){
        c.x+=(move-128)/12;
        c.z+=(move-128)/3;
    }else if(beat->camera==F_HAND){
        c.z+=24;
    }else if(beat->camera==F_INSERT){
        c.x+=((id/11)%3-1)*9;
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
static void ggFilmHouse(GgFilmCamera cam,int x,int z,int w,int height,int lit,
                        int roofFinished){
    int f=z-43,top=height;
    ggFilmBox(cam,x,0,z,w,height,86,21,20,24);
    if(roofFinished){
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
    }else{
        // Visible rafter skeleton under construction: the player can
        // actually SEE the missing roof during Thorgrim's early dialogue.
        ggFilmBox(cam,x,top+39,z,9,9,111,25,24,26);
        for(int k=0;k<4;k++){
            int rz=f-8+k*31;
            ggFilmQuad(cam,ggV3(x-w/2-8,top,rz),
                           ggV3(x,top+45,rz),
                           ggV3(x,top+46,rz+5),
                           ggV3(x-w/2-8,top+3,rz+5),24);
            ggFilmQuad(cam,ggV3(x,top+45,rz),
                           ggV3(x+w/2+8,top,rz),
                           ggV3(x+w/2+8,top+3,rz+5),
                           ggV3(x,top+46,rz+5),25);
        }
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
static void ggFilmFloor(GgFilmCamera cam,int p,int t,int winter){
    int wet=(p==F_FORD||p==F_SHORE||p==F_STORM);
    int room=(p==F_FORGE||p==F_ARCHIVE||p==F_HALL||
              p==F_THRONE||p==F_COUCH||p==F_PORCH||p==F_HEARTH);
    u8 base=wet?13:room?21:winter?22:p==F_VILLAGE?17:20;
    ggFilmQuad(cam,ggV3(-850,0,330),ggV3(850,0,330),
                   ggV3(850,0,1800),ggV3(-850,0,1800),base);
    if(room){
        // Actual wooden floorboards on a room plane. Narrow, warm seams
        // replace the old universal dark checker grid from the preview.
        for(int x=-540;x<=540;x+=180){
            ggFilmQuad(cam,ggV3(x,1,400),ggV3(x+3,1,400),
                           ggV3(x+3,1,1700),ggV3(x,1,1700),24);
        }
        for(int z=585;z<1630;z+=380)
            ggFilmQuad(cam,ggV3(-640,1,z),ggV3(640,1,z),
                           ggV3(640,1,z+2),ggV3(-640,1,z+2),20);
        return;
    }
    if(wet){
        // Natural banks, not a graph-paper sea. Waves live at y=2 on
        // the water plane, so distance really changes their size.
        ggFilmQuad(cam,ggV3(-850,1,400),ggV3(-600,1,400),
                       ggV3(-455,1,1800),ggV3(-850,1,1800),winter?22:21);
        ggFilmQuad(cam,ggV3(615,1,400),ggV3(850,1,400),
                       ggV3(850,1,1800),ggV3(460,1,1800),winter?23:20);
        for(int i=0;i<16;i++){
            int z=460+(i*113)%1180;
            int x=((i*257+t/5)%850)-425;
            int w=21+(i*17)%53;
            ggFilmQuad(cam,ggV3(x,2,z),ggV3(x+w,2,z),
                           ggV3(x+w+5,2,z+4),ggV3(x+7,2,z+4),
                           i&1?14:12);
        }
        return;
    }
    // The village and the ruined gate have uneven natural ground, with
    // a continuous walkable lane. Scattered triangular soil/grass/stone
    // facets are genuinely on the ground plane; no uniform grid.
    if(p==F_VILLAGE)
        ggFilmQuad(cam,ggV3(-108,1,400),ggV3(108,1,400),
                       ggV3(70,1,1750),ggV3(-76,1,1750),winter?20:21);
    for(int i=0;i<18;i++){
        int x=((i*293+73)%1450)-725;
        int z=450+(i*173)%1300;
        int w=23+(i*13)%71;
        if(p==F_VILLAGE&&x>-106&&x<106)continue;
        u8 tone=winter?(i&1?23:22):p==F_VILLAGE?(i%3?16:18):(i&1?24:25);
        ggFilmQuad(cam,ggV3(x,1,z),ggV3(x+w,1,z+3),
                       ggV3(x+w-7,1,z+19),ggV3(x+6,1,z+13),tone);
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
// Background performers use the SAME 40x48 detailed 2D cels as the
// principal actors; no cheap box heads, floating cylinder limbs or
// bubble-headed 3D stand-ins. Only their projected size changes with Z.
static void ggFilmBackgroundWorker(GgFilmCamera cam,int x,int z,int t,int id){
    int phase=(t/9+id*37)%176;
    int travel=phase<88?phase-44:132-phase;
    int walking=phase>8&&phase<80;
    int who=id==1?F_THORGRIM:F_NESSA;
    int pose=walking?((t/18)&1?1:2):((t/42)&1?11:0);
    GgS2 foot=ggFilmProject(cam,ggV3(x+travel,0,z));
    int h=130*260/(z-cam.z);
    if(h<16)h=16;
    if(h>46)h=46;
    // World-anchored feet and a real 2D walk/work pose, never a block mesh.
    filmDrawActor(who,pose,foot.x,foot.y,h,phase>=88);
}
static void ggFilmInterior(GgFilmCamera cam,int p,int t,int chapter,int winter){
    // Walls, cornice and corridor all converge on the same perspective.
    ggFilmQuad(cam,ggV3(-700,0,1600),ggV3(700,0,1600),
                   ggV3(700,390,1600),ggV3(-700,390,1600),22);
    ggFilmQuad(cam,ggV3(-700,0,350),ggV3(-700,0,1600),
                   ggV3(-700,390,1600),ggV3(-700,390,350),20);
    ggFilmQuad(cam,ggV3(700,0,1600),ggV3(700,0,350),
                   ggV3(700,390,350),ggV3(700,390,1600),21);
    ggFilmFloor(cam,p,t,winter);
    if(p==F_PORCH){
        // A real shelter, not the generic palace corridor that the first
        // rain scene previously inherited. The same window, sill, roof,
        // beams and furniture occupy fixed world coordinates in every cut.
        ggFilmBox(cam,0,242,1490,800,22,75,24,20,26);
        ggFilmBox(cam,-325,0,990,34,260,39,21,20,25);
        ggFilmBox(cam,325,0,990,34,260,39,21,20,25);
        ggFilmBox(cam,0,238,994,688,18,42,25,24,26);
        for(int x=-280;x<=280;x+=140)
            ggFilmBox(cam,x,254,1100,9,15,660,25,24,26);
        // Glazed window with a world-positioned rain layer behind the
        // mullions; the light never follows the talking actor's face.
        ggFilmQuad(cam,ggV3(200,115,1554),ggV3(450,115,1554),
                       ggV3(450,253,1554),ggV3(200,253,1554),12);
        // The first-rain chapter has rain beyond the window. Afterward
        // the very same porch remains dry; weather does not loop forever
        // just because the story visits the location again.
        if(chapter==1||winter)
            for(int i=0;i<9;i++){
                int x=210+i*24;
                int y=130+((i*37+t/5)%104);
                if(winter)
                    ggFilmQuad(cam,ggV3(x,y,1551),ggV3(x+6,y,1551),
                                   ggV3(x+6,y+7,1551),ggV3(x,y+7,1551),23);
                else
                    ggFilmQuad(cam,ggV3(x,y,1551),ggV3(x+3,y,1551),
                                   ggV3(x+3,y+16,1551),ggV3(x,y+16,1551),13);
            }
        ggFilmBox(cam,325,114,1536,268,10,12,24,25,26);
        ggFilmBox(cam,325,248,1536,268,10,12,24,25,26);
        ggFilmBox(cam,200,114,1536,10,148,12,24,25,26);
        ggFilmBox(cam,450,114,1536,10,148,12,24,25,26);
        ggFilmBox(cam,325,114,1536,8,148,12,25,24,26);
        ggFilmBox(cam,325,182,1536,265,8,12,25,24,26);
        // Side bench and an honest foreground surface for Mira's cup at
        // screen (128,150). The table is z=583, top y=36 in world space.
        ggFilmBox(cam,-220,0,1120,195,31,71,20,24,25);
        ggFilmBox(cam,-220,28,1138,195,57,19,21,24,26);
        ggFilmBox(cam,0,0,583,145,36,74,24,20,26);
        ggFilmBox(cam,-52,0,583,12,39,42,20,24,25);
        ggFilmBox(cam,52,0,583,12,39,42,20,24,25);
        return;
    }
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
    }else if(p==F_COUCH||p==F_HEARTH){
        ggFilmBox(cam,175,18,1200,220,46,83,24,20,25);
        ggFilmBox(cam,175,59,1237,220,72,25,21,24,25);
        ggFilmBox(cam,58,30,1200,25,69,105,21,24,25);
        ggFilmBox(cam,292,30,1200,25,69,105,21,24,25);
        if(p==F_HEARTH){
            // Hearth stonework, recessed fire and mantel are fixed to the
            // rear wall. Firelight flickers *inside* its opening.
            ggFilmBox(cam,-245,0,1500,194,140,48,21,20,26);
            ggFilmBox(cam,-245,26,1469,115,79,10,20,24,25);
            ggFilmBox(cam,-245,26,1462,81,64,8,(t/20)&1?9:10,24,26);
            ggFilmBox(cam,-245,142,1491,230,13,67,25,24,26);
        }
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
static void ggFilmWorldDraw(const FilmBeat *beat,int elapsed,int move,
                            int chapter,int roofFinished,int winter){
    int p=beat->place;
    if(p==F_ORBIT)return; // globe remains the spherical dedicated renderer
    int scenic=ggFilmSceneTime(elapsed);
    GgFilmCamera cam=ggFilmCameraAt(beat,move);
    // Hearth is an actual interior. Formerly it incorrectly received the
    // outdoor village set (houses and trees), breaking cuts from the room.
    int inside=(p==F_FORGE||p==F_ARCHIVE||p==F_HALL||
                p==F_THRONE||p==F_COUCH||p==F_PORCH||p==F_HEARTH);
    if(inside){ggFilmInterior(cam,p,scenic,chapter,winter);return;}
    ggFilmFloor(cam,p,scenic,winter);
    if(p==F_GATE||p==F_RUINS){
        ggFilmArch(cam,-170,1260,p==F_RUINS);
        ggFilmArch(cam,225,970,p==F_RUINS);
        ggFilmBox(cam,0,0,1610,650,140,80,21,20,26);
    }else if(p==F_VILLAGE){
        // Persist the same village plan, but let the town actually GROW:
        // one founding shelter, then its first neighbor, then the hall.
        // Later stories never jump back to a vanished building.
        ggFilmHouse(cam,-340,1190,158,138,1,1);
        if(chapter>=2)ggFilmHouse(cam,325,1040,155,155,1,roofFinished);
        if(chapter>=5)ggFilmHouse(cam,25,1510,185,163,0,1);
        if(chapter>=10)ggFilmHouse(cam,-75,1800,190,177,1,1);
        ggFilmTree(cam,-530,910,158,scenic,0);
        ggFilmTree(cam,515,1180,190,scenic,2);
        if(chapter>=2)ggFilmBackgroundWorker(cam,-175,1070,scenic,1);
        if(chapter>=10)ggFilmBackgroundWorker(cam,140,1280,scenic,0);
    }else if(p==F_FORD||p==F_SHORE||p==F_STORM){
        ggFilmTree(cam,-385,1220,144,scenic,0);
        ggFilmTree(cam,425,1160,151,scenic,1);
        if(p==F_FORD){
            if(chapter>=8){
                ggFilmBridge(cam,chapter==9,scenic);
                if(chapter>=10)ggFilmBackgroundWorker(cam,172,1120,scenic,1);
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
    if(winter){
        // Both unlockable winter reels have snow at scene depth rather than
        // a flat white overlay. Flakes descend through the camera volume.
        for(int i=0;i<14;i++){
            int x=((i*179+scenic/6)%1100)-550;
            int z=470+(i*107)%1200;
            int y=40+((i*61-scenic/8)%260+260)%260;
            ggFilmQuad(cam,ggV3(x,y,z),ggV3(x+5,y,z),
                           ggV3(x+5,y+5,z),ggV3(x,y+5,z),i&1?22:23);
        }
    }
    if(p==F_STORM){
        // Rain exists within the set's depth, not a fuzzy full-screen veil.
        for(int i=0;i<14;i++){
            int x=((i*131+scenic/3)%900)-450;
            int z=480+(i*89)%950;
            int y=55+((i*97-scenic/3)%210+210)%210;
            ggFilmQuad(cam,ggV3(x,y,z),ggV3(x+3,y,z),
                           ggV3(x+11,y+24,z),ggV3(x+8,y+24,z),22);
        }
    }
}
// Selective painter-order occluders sit *in front* of the actors.
// This gives genuine traversal behind door posts, bridge rails and trees,
// without allocating a full-depth buffer on the 3DS. The supporting geometry
// is authored at nearer Z than character feet (normally around 690).
static void ggFilmWorldForeground(const FilmBeat *beat,int move,int chapter){
    int p=beat->place;
    if(p==F_ORBIT||beat->camera==F_INSERT)return;
    GgFilmCamera cam=ggFilmCameraAt(beat,move);
    if(p==F_FORD&&chapter>=8){
        // Near timber handrail, diagonal braces and a physical end post.
        ggFilmBox(cam,-160,75,515,13,10,136,25,24,26);
        ggFilmBox(cam,-160,0,515,19,91,23,20,24,25);
        ggFilmQuad(cam,ggV3(-169,15,531),ggV3(-151,70,531),
                       ggV3(-151,77,537),ggV3(-169,21,537),24);
    }else if(p==F_PORCH||p==F_HALL||p==F_ARCHIVE||p==F_FORGE){
        // Local architecture stays at fixed world coordinates during cuts.
        ggFilmBox(cam,-335,0,510,30,276,37,20,24,26);
        ggFilmBox(cam,335,0,510,30,276,37,21,24,26);
        ggFilmBox(cam,0,267,510,704,24,37,24,25,26);
        if(p==F_ARCHIVE)ggFilmBox(cam,-320,12,540,67,96,45,24,20,25);
    }else if(p==F_GATE){
        ggFilmArch(cam,-300,552,0);
    }else if(p==F_VILLAGE&&chapter>=5){
        // A foreground canopy can really obscure a person passing it.
        ggFilmTree(cam,-365,558,124,move*2,4);
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
