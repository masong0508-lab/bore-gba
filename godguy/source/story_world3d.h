// GodGuy 3DS chapter-film world stage.
// Fixed-camera perspective projection of authored world-space geometry.
// No GPU dependency, heap allocation, texture cache, z-buffer or new surface:
// triangles rasterize straight into the existing 256x192 indexed film page.
// Fixed draw order: far landscape -> structures -> terrain details -> cast.
// DS/DSi compile the original painted-plate renderer unchanged.
#ifndef GG_STORY_WORLD3D_H
#define GG_STORY_WORLD3D_H
#ifdef __3DS__

#include "story_world3d_palette.h"

typedef struct { short x,y,z; } GgV3;
typedef struct { int x,y; } GgS2;
typedef struct { int x,z,yaw; u8 *page; } GgFilmCamera;
typedef struct { GgV3 lead,other; int leadDepth,otherDepth; } GgFilmFootprints;
enum { GG_FILM_DUSK_SUN_X=-1050, GG_FILM_DUSK_SUN_Z=3720 };

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
// Coherent coverage grammar: keep a location's establishing angle, repeat
// each speaker's familiar near angle, and reserve traveling moves for TRACK.
// The old system hashed subtitle text, creating a jarring camera jump on
// virtually every spoken line in the same room. That is not filmmaking.
// World geometry, light sources and screen direction stay anchored.
static GgFilmCamera ggFilmCameraAt(const FilmBeat *beat,int move){
    int paired=beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N;
    int speakerLeft=paired?filmActorLeftOf(beat->lead,beat->other):
                    beat->lead==F_WISP;
    GgFilmCamera c={0,0,0,uiBuf};
    if(beat->camera==F_WIDE){
        // This is the audience's spatial anchor; return here regularly.
        c.z=-22;
        // A deliberate arrival shot may dolly into the fixed geography.
        if(beat->lead==F_OFFSCREEN||!paired){
            c.x=(move-128)/14;
            c.z+=(move-128)/4;
        }
    }else if(beat->camera==F_NEAR){
        // Two actors share the same line of action. Each character
        // receives ONE repeatable three-quarter angle per conversation.
        c.x=speakerLeft?-29:29;
        c.z=49;
        c.yaw=speakerLeft?-12:12;
    }else if(beat->camera==F_TRACK){
        c.x=(speakerLeft?-22:22)+(move-128)/3;
        c.z=-58+move*124/256;
        c.yaw=(speakerLeft?-8:8)+(move-128)/11;
    }else if(beat->camera==F_HAND){
        // Shared insert/prop coverage returns to the same work surface.
        c.x=speakerLeft?-12:12;
        c.z=26;
    }else if(beat->camera==F_INSERT){
        c.z=22;
    }
    return c;
}
static GgS2 ggFilmProject(GgFilmCamera cam,GgV3 p){
    int x=p.x-cam.x,z=p.z-cam.z;
    int across=x-z*cam.yaw/256;
    int depth=z+x*cam.yaw/256;
    if(depth<120)depth=120;
    GgS2 q={128+across*260/depth,68+(220-(int)p.y)*260/depth};
    return q;
}
// Fixed-point edge steps pay the setup division once per edge. Each visible
// row is still one contiguous write into the existing destination page.
static void ggFilmTriSpan(u8 *page,GgS2 top,int longStep,GgS2 a,GgS2 b,u8 pixel){
    int start=a.y>FILM_TOP?a.y:FILM_TOP;
    int end=b.y<FILM_BOTTOM?b.y:FILM_BOTTOM;
    if(start>=end)return;
    int shortStep=(b.x-a.x)*4096/(b.y-a.y);
    int x=top.x*4096+(start-top.y)*longStep;
    int shortX=a.x*4096+(start-a.y)*shortStep;
    for(int y=start;y<end;y++){
        int l=x>>12,r=shortX>>12;
        if(l>r){int swap=l;l=r;r=swap;}
        if(l<0)l=0;
        if(r>255)r=255;
        if(l<=r)memset(page+y*256+l,pixel,(size_t)(r-l+1));
        x+=longStep;
        shortX+=shortStep;
    }
}
static void ggFilmTri(u8 *page,GgS2 a,GgS2 b,GgS2 c,u8 ink){
    if((a.x<0&&b.x<0&&c.x<0)||(a.x>255&&b.x>255&&c.x>255))return;
    if(a.y>b.y){GgS2 swap=a;a=b;b=swap;}
    if(b.y>c.y){GgS2 swap=b;b=c;c=swap;}
    if(a.y>b.y){GgS2 swap=a;a=b;b=swap;}
    if(a.y==c.y||c.y<=FILM_TOP||a.y>=FILM_BOTTOM)return;
    int step=(c.x-a.x)*4096/(c.y-a.y);
    u8 pixel=ggFilmInk(ink);
    ggFilmTriSpan(page,a,step,a,b,pixel);
    ggFilmTriSpan(page,a,step,b,c,pixel);
}
static void ggFilmQuad(GgFilmCamera cam,GgV3 a,GgV3 b,GgV3 c,GgV3 d,u8 ink){
    GgS2 aa=ggFilmProject(cam,a),bb=ggFilmProject(cam,b);
    GgS2 cc=ggFilmProject(cam,c),dd=ggFilmProject(cam,d);
    ggFilmTri(cam.page,aa,bb,cc,ink);
    ggFilmTri(cam.page,aa,cc,dd,ink);
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

#include "story_world3d_material.h"

#include "story_world3d_exterior.h"

static int ggFilmRoom(int p){
    return p==F_FORGE||p==F_ARCHIVE||p==F_HALL||p==F_THRONE||
           p==F_COUCH||p==F_PORCH||p==F_HEARTH;
}
static void ggFilmBackdrop(GgFilmCamera cam,int place,int t,int winter){
    int room=ggFilmRoom(place),storm=place==F_STORM;
    int evening=place==F_SHORE||place==F_GATE||place==F_RUINS;
    static const u8 shades[][7]={
        {31,GG_SKY_HIGH,GG_SKY_HIGH,GG_SKY_MIST,GG_SKY_MIST,GG_CLOUD,GG_CLOUD},
        {GG_SLATE,30,4,28,27,26,25},
        {2,GG_SLATE,GG_SKY_RAIN,GG_SKY_RAIN,31,GG_STONE,21},
        {31,GG_SKY_RAIN,GG_SKY_HIGH,GG_SKY_MIST,GG_SKY_MIST,GG_CLOUD,GG_CLOUD},
    };
    const u8 *sky=shades[winter?3:storm?2:evening?1:0];
    for(int y=FILM_TOP;y<FILM_BOTTOM;y++){
        int band=(y-FILM_TOP)/8;
        memset(cam.page+y*256,ggFilmInk(room?20:sky[band<6?band:6]),256);
    }
    if(room)return;
    ggFilmClouds(cam,storm,evening,t);
    if(!storm&&!winter){
        if(evening)ggFilmSun(cam,GG_FILM_DUSK_SUN_X,454,GG_FILM_DUSK_SUN_Z,130,10);
        else if(place==F_ORBIT)ggFilmSun(cam,990,635,3660,105,11);
    }
    static const short first[]={415,610,355,755,502,366,635,498,655,455,486};
    static const short second[]={335,294,496,358,301,486,333,507,273,465,313};
    static const short third[]={215,185,340,249,190,226,196,330,234,270,291};
    ggFilmRidge(cam,first,3450,0,evening?30:GG_SKY_HIGH,
                evening?4:31,winter?23:evening?29:GG_SKY_MIST);
    ggFilmRidge(cam,second,2890,220,evening?3:GG_SKY_RAIN,
                evening?2:GG_SLATE,storm?21:31);
    ggFilmRidge(cam,third,2320,-210,evening?GG_SLATE:GG_PINE,
                evening?3:16,31);
}
#include "story_world3d_ground.h"

static void ggFilmFloor(GgFilmCamera cam,int p,int t,int chapter,int winter){
    int wet=p==F_FORD||p==F_SHORE||p==F_STORM;
    u8 base=p==F_FORD?13:wet?GG_WATER_DARK:winter?22:
            p==F_VILLAGE||p==F_ORBIT?17:GG_SLATE;
    int extent=1900,far=3220;
    ggFilmPaving(cam,-extent,extent,330,far,0,base);
    if(wet){
        ggFilmWater(cam,p,t);
        if(p==F_FORD){
            // The bank crests meet the two ends of the elevated deck.
            ggFilmQuad(cam,ggV3(-extent,1,330),ggV3(extent,1,330),
                          ggV3(extent,1,604),ggV3(-extent,1,628),winter?22:18);
            ggFilmQuad(cam,ggV3(-extent,1,1156),ggV3(extent,1,1102),
                          ggV3(extent,1,far),ggV3(-extent,1,far),winter?23:17);
            for(int i=0;i<9;i++){
                int x=155+i*64;
                int nearZ=559+(i*31)%36,farZ=1169+(i*33)%49;
                for(int side=-1;side<=1;side+=2){
                    ggFilmGroundPatch(cam,side*x,2,nearZ,63,30,winter?23:GG_MOSS);
                    ggFilmTuft(cam,side*x,2,nearZ,23+i%5*4,side*6,GG_PINE);
                    ggFilmGroundPatch(cam,side*x,2,farZ,96,67,winter?22:GG_LEAF);
                    ggFilmTuft(cam,side*x,2,farZ,34+i%3*8,side*8,18);
                }
            }
            if(chapter<GG_FILM_BRIDGE_CHAPTER)ggFilmSteppingStones(cam);
        }else{
            ggFilmBeach(cam,p,t);
        }
        return;
    }
    ggFilmDryGround(cam,p,winter);
}
// Timber bays share the same grade as cast and prop blocking. Each
// rail segment ends in a post which bears down into its deck and pier.
static int ggFilmBridgeBayZ(int bay){
    return GG_FILM_BRIDGE_FIRST+bay*GG_FILM_BRIDGE_BAY+GG_FILM_BRIDGE_BAY/2;
}
static int ggFilmBridgeSway(int bay,int damaged){
    return damaged&&bay>4?(bay-4)*3:0;
}
static void ggFilmBridgeSide(GgFilmCamera cam,int bay,int side,int damaged){
    int z=ggFilmBridgeBayZ(bay),lastBay=bay==GG_FILM_BRIDGE_BAYS-1;
    int end=lastBay?GG_FILM_BRIDGE_END-8:z+GG_FILM_BRIDGE_BAY;
    int first=side*96+ggFilmBridgeSway(bay,damaged);
    int last=side*96+ggFilmBridgeSway(bay+1<GG_FILM_BRIDGE_BAYS?bay+1:bay,damaged);
    ggFilmQuad(cam,ggV3(first-4,69,z),ggV3(first+4,69,z),
                  ggV3(last+4,69,end),ggV3(last-4,69,end),26);
    ggFilmQuad(cam,ggV3(first-4,61,z),ggV3(last-4,61,end),
                  ggV3(last-4,69,end),ggV3(first-4,69,z),25);
    if(!damaged||bay!=5||side<0){
        ggFilmQuad(cam,ggV3(first-5,81,z),ggV3(first+5,81,z),
                      ggV3(last+5,81,end),ggV3(last-5,81,end),26);
        ggFilmQuad(cam,ggV3(first-5,73,z),ggV3(last-5,73,end),
                      ggV3(last-5,81,end),ggV3(first-5,81,z),25);
    }
    ggFilmBox(cam,first,30,z,15,56,16,25,24,26);
    ggFilmBox(cam,first,83,z,20,5,21,24,25,26);
    if(lastBay)ggFilmBox(cam,last,30,end,17,56,16,25,24,26);
}
static void ggFilmBridge(GgFilmCamera cam,int damaged,int t){
    ggFilmStoneCourse(cam,ggFilmBridgeGrade[2],ggFilmBridgeGrade[3],103,-9,GG_STONE);
    for(int i=GG_FILM_BRIDGE_BAYS-1;i>=0;i--){
        int z=ggFilmBridgeBayZ(i),sway=ggFilmBridgeSway(i,damaged);
        for(int side=-1;side<=1;side+=2){
            int foot=side*83+sway;
            ggFilmGroundPatch(cam,foot+side*32,2,z,118,27,GG_WATER_DARK);
            ggFilmBox(cam,foot,0,z,29,25,33,GG_STONE,GG_SLATE,21);
            ggFilmQuad(cam,ggV3(foot-5,13,z-13),ggV3(foot+5,13,z-13),
                          ggV3(side*36+sway+6,27,z-13),ggV3(side*36+sway-5,27,z-13),24);
        }
        ggFilmBox(cam,sway,23,z,192,GG_FILM_BRIDGE_TOP-23,GG_FILM_BRIDGE_BAY,24,20,25);
        for(int slat=-3;slat<=3;slat++){
            int tz=z+slat*11;
            u8 wood=(i+slat+4)%3?25:26;
            ggFilmPaving(cam,sway-93,sway+93,tz-5,tz+5,GG_FILM_BRIDGE_TOP,wood);
            if(slat&1)ggFilmGroundPatch(cam,sway-18,35,tz,127,4,24);
        }
        for(int side=-1;side<=1;side+=2)ggFilmBridgeSide(cam,i,side,damaged);
    }
    ggFilmStoneCourse(cam,ggFilmBridgeGrade[0],ggFilmBridgeGrade[1],103,9,GG_LIMESTONE);
    // The rescue lantern sits on a rail post, above the retaining bracket.
    int z=ggFilmBridgeBayZ(3);
    ggFilmBox(cam,-96,88,z,12,10,12,24,20,25);
    ggFilmBox(cam,-96,98,z,16,17,16,(t/20)&1?10:9,24,26);
}
// Background performers use the SAME 40x48 detailed 2D cels as the
// principal actors; no cheap box heads, floating cylinder limbs or
// bubble-headed 3D stand-ins. Only their projected size changes with Z.
static void ggFilmBackgroundWorker(GgFilmCamera cam,int place,int chapter,
                                   int x,int z,int t,int id){
    int phase=(t/9+id*37)%176;
    int travel=phase<88?phase-44:132-phase;
    int walking=phase>8&&phase<80;
    int who=id==1?F_THORGRIM:F_NESSA;
    int pose=walking?((t/18)&1?1:2):((t/42)&1?11:0);
    x+=travel;
    int y=ggFilmSurfaceHeight(place,chapter,x,z);
    GgS2 foot=ggFilmProject(cam,ggV3(x,y,z));
    GgS2 head=ggFilmProject(cam,ggV3(x,y+130,z));
    int h=foot.y-head.y;
    if(h<16)h=16;
    if(h>46)h=46;
    // World-anchored feet and a real 2D walk/work pose, never a block mesh.
    filmDrawActor(who,pose,foot.x,foot.y,h,phase>=88);
}
// Lightweight scenery animation: stable world-space props and purposeful
// slow changes. No framebuffers, random fly-bys or repetitive particle fog.
static void ggFilmMarketCart(GgFilmCamera cam,int t){
    // A background wagon travels through the village in depth, which
    // produces real foreshortening rather than a single scrolling bitmap.
    int phase=t%2600;
    int z=1450-phase*650/2600;
    int x=248+(phase*19/2600);
    ggFilmGroundPatch(cam,x+10,1,z,127,133,GG_PINE);
    for(int end=-1;end<=1;end+=2)
        ggFilmBox(cam,x,18,z+end*34,119,7,8,24,20,25);
    for(int end=-1;end<=1;end+=2)
        ggFilmCartWheel(cam,x+57,21,z+end*34,21,phase/69);
    ggFilmBox(cam,x,22,z,91,30,95,24,20,25);
    ggFilmBox(cam,x,52,z,99,9,99,25,24,26);
    for(int end=-1;end<=1;end+=2)
        ggFilmCartWheel(cam,x-57,21,z+end*34,21,phase/69);
    ggFilmGrain(cam,x-41,x+40,44,z-51,5,26);
    ggFilmGrain(cam,x-39,x+37,30,z-51,4,25);
    // Cargo is attached to the carriage. It never floats ahead of it.
    ggFilmBox(cam,x-13,61,z+5,39,17,42,21,24,26);
    ggFilmBox(cam,x+23,61,z-16,34,13,42,20,24,25);
}
#include "story_world3d_interior.h"

static GgS2 ggFilmPropSupport(GgFilmCamera camera,int place,int chapter){
    GgV3 point=ggV3(0,ggFilmSurfaceHeight(place,chapter,0,675),675);
    if(place==F_PORCH)point=ggV3(0,GG_FILM_PORCH_TABLE_TOP,GG_FILM_PORCH_TABLE_Z);
    else if(place==F_ARCHIVE)point=ggV3(57,GG_FILM_ARCHIVE_DESK_TOP,815);
    else if(place==F_FORGE)point=ggV3(-155,GG_FILM_ANVIL_TOP,GG_FILM_ANVIL_Z);
    else if(place==F_COUCH)point=ggV3(24,GG_FILM_COUCH_TABLE_TOP,GG_FILM_COUCH_TABLE_Z);
    else if(place==F_THRONE)point=ggV3(0,GG_FILM_THRONE_STAND_TOP,GG_FILM_THRONE_STAND_Z);
    return ggFilmProject(camera,point);
}
static void ggFilmWorldDraw(GgFilmCamera cam,const FilmBeat *beat,int scenic,
                            int chapter,int roofFinished,int winter){
    int p=beat->place;
    ggFilmBackdrop(cam,p,scenic,winter);
    if(ggFilmRoom(p)){
        ggFilmInterior(cam,p,scenic,chapter,winter);
        return;
    }
    ggFilmFloor(cam,p,scenic,chapter,winter);
    if(p==F_GATE||p==F_RUINS){
        int ruined=p==F_RUINS;
        ggFilmPaving(cam,-139,139,428,1570,2,21);
        for(int i=0;i<9;i++){
            int z=465+i*91;
            ggFilmPaving(cam,-145,145,z,z+4,3,GG_SLATE);
            ggFilmPaving(cam,(i%2?34:-23),(i%2?38:-19),z,z+91,3,GG_SLATE);
        }
        for(int side=-1;side<=1;side+=2){
            int x=side*356;
            ggFilmBox(cam,x,0,1530,258,ruined?97:180,110,GG_STONE,GG_SLATE,GG_LIMESTONE);
            ggFilmMasonry(cam,x-127,x+127,0,ruined?97:180,1474,
                          34,66,20,GG_STONE,21);
            if(!ruined){
                ggFilmBox(cam,side*471,0,1390,105,269,120,GG_STONE,GG_SLATE,GG_LIMESTONE);
                ggFilmPanel(cam,side*471-10,side*471+10,138,212,1328,20);
                for(int i=-1;i<=1;i++)
                    ggFilmBox(cam,side*471+i*39,259,1330,25,34,18,GG_STONE,20,GG_LIMESTONE);
            }
            for(int i=0;i<6;i++){
                int wx=side*(197+(i*101)%321),z=1090-i*87;
                ggFilmGroundPatch(cam,wx+14,2,z,104,54,20);
                ggFilmRock(cam,wx,0,z,76+i%4*17,26+i%3*21,64+i*10,
                           GG_STONE,21,GG_SLATE);
                if(ruined)ggFilmTuft(cam,wx-17,2,z-22,38+i%3*13,side*7,GG_PINE);
            }
        }
        if(!ruined){
            int h=chapter==9?143:170;
            ggFilmBox(cam,-173,0,1295,94,h,74,GG_STONE,GG_SLATE,GG_LIMESTONE);
            ggFilmMasonry(cam,-219,-127,0,h,1256,35,43,20,GG_STONE,21);
            ggFilmRock(cam,-174,h-5,1294,94,28,72,GG_LIMESTONE,GG_STONE,GG_SLATE);
        }
        ggFilmArch(cam,ruined?-170:0,1330,ruined);
        if(ruined)ggFilmArch(cam,260,1060,1);
        if(p==F_GATE&&chapter==9&&scenic<42*FMV_TICKS){
            for(int i=0;i<3;i++){
                int x=-390+(scenic*8+i*227)%780;
                int y=115+(i*55+scenic)%170;
                int z=1050+i*107;
                ggFilmQuad(cam,ggV3(x,y,z),ggV3(x+65,y+19,z),
                               ggV3(x+65,y+24,z),ggV3(x,y+5,z),8);
            }
            if(scenic>=22*FMV_TICKS&&scenic<28*FMV_TICKS){
                GgV3 impact=ggV3(-168,130,1233);
                for(int i=0;i<4;i++){
                    int dx=(i&1)?-50:50,dy=(i&2)?-40:45;
                    ggFilmTri(cam.page,ggFilmProject(cam,impact),
                        ggFilmProject(cam,ggV3(impact.x+dx,impact.y+dy,impact.z)),
                        ggFilmProject(cam,ggV3(impact.x+dx/3,impact.y+dy/2-10,impact.z)),
                        i&1?9:10);
                }
            }
        }
    }else if(p==F_VILLAGE){
        // Persist the same village plan, but let the town actually GROW:
        // one founding shelter, then its first neighbor, then the hall.
        // Later stories never jump back to a vanished building.
        ggFilmTree(cam,-622,1610,284,scenic,4);
        ggFilmTree(cam,566,1720,260,scenic,3);
        if(chapter>=10)ggFilmHouse(cam,-94,1760,240,205,1,1);
        if(chapter>=5)ggFilmHouse(cam,136,1450,193,184,0,1);
        ggFilmHouse(cam,-348,1137,221,182,1,1);
        ggFilmFence(cam,-523,1055,3);
        if(chapter>=2){
            ggFilmHouse(cam,341,1070,216,196,1,roofFinished);
            ggFilmFence(cam,303,955,3);
            ggFilmBox(cam,266,0,893,84,29,34,25,24,26);
        }
        if(chapter>=5)ggFilmMarket(cam,-204,1022);
        ggFilmTree(cam,-535,939,224,scenic,0);
        ggFilmTree(cam,547,1130,228,scenic,2);
        // One physical character cannot work in the distance and speak
        // in the foreground simultaneously. Keep the 2D performers
        // distinct across every cut, even during large ensemble shots.
        if(cam.page==uiBuf&&chapter>=2&&beat->lead!=F_THORGRIM&&beat->other!=F_THORGRIM)
            ggFilmBackgroundWorker(cam,p,chapter,-175,1070,scenic,1);
        if(cam.page==uiBuf&&chapter>=10&&beat->lead!=F_NESSA&&beat->other!=F_NESSA)
            ggFilmBackgroundWorker(cam,p,chapter,140,1280,scenic,0);
        // Roads become active as the village grows. The moving cart and
        // its cargo share the same world transform over all dialogue cuts.
        if(chapter>=5)ggFilmMarketCart(cam,scenic);
        // Permanent street lanterns, not arbitrary flickering screen dots.
        ggFilmBeam(cam,-115,0,913,8,128,8,24);
        ggFilmBox(cam,-115,127,913,23,27,24,20,24,25);
        ggFilmBox(cam,-115,139,913,15,15,16,(scenic/37)&1?9:10,24,26);
    }else if(p==F_FORD||p==F_SHORE||p==F_STORM){
        if(p==F_FORD){
            ggFilmPine(cam,-642,1370,273,scenic,1,0);
            ggFilmPine(cam,-437,1350,208,scenic,3,0);
            ggFilmTree(cam,571,1410,244,scenic,1);
            ggFilmRock(cam,382,1,1182,127,74,96,GG_STONE,21,GG_SLATE);
            ggFilmRock(cam,-361,0,576,110,42,65,GG_LIMESTONE,GG_STONE,GG_SLATE);
            ggFilmRock(cam,410,0,555,134,63,103,GG_STONE,21,GG_SLATE);
            if(chapter>=GG_FILM_BRIDGE_CHAPTER){
                ggFilmBridge(cam,chapter==9,scenic);
                if(cam.page==uiBuf&&chapter>=10&&beat->lead!=F_THORGRIM&&beat->other!=F_THORGRIM)
                    ggFilmBackgroundWorker(cam,p,chapter,172,1205,scenic,1);
            }
        }else ggFilmSeaCliffs(cam,p,scenic);
    }else if(p==F_ORBIT){
        // A low stone observation wall and a real path give the open
        // lookout a foreground and a destination for the walking camera.
        ggFilmTree(cam,-642,1500,286,scenic,3);
        ggFilmPine(cam,516,1280,254,scenic,2,0);
        ggFilmPine(cam,633,1330,268,scenic,3,0);
        ggFilmTree(cam,-471,1080,256,scenic,0);
        for(int side=-1;side<=1;side+=2){
            int x=side*315;
            ggFilmGroundPatch(cam,x+18,2,755,330,126,GG_PINE);
            ggFilmBox(cam,x,0,790,358,44,57,GG_STONE,GG_SLATE,GG_LIMESTONE);
            ggFilmMasonry(cam,x-178,x+178,0,43,760,23,54,GG_SLATE,GG_LIMESTONE,GG_STONE);
            ggFilmBox(cam,x,43,790,364,8,65,GG_STONE,GG_SLATE,GG_LIMESTONE);
            ggFilmRock(cam,side*243,0,647,119,39,63,GG_MOSS,GG_STONE,GG_SLATE);
            ggFilmTuft(cam,side*204,1,621,39,side*4,GG_MOSS);
            ggFilmTuft(cam,side*265,1,580,49,side*8,GG_LEAF);
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
static void ggFilmWorldForeground(GgFilmCamera cam,const FilmBeat *beat,int move,int chapter){
    int p=beat->place;
    if(beat->camera==F_INSERT)return;
    if(p==F_FORD&&chapter>=GG_FILM_BRIDGE_CHAPTER){
        for(int side=-1;side<=1;side+=2){
            ggFilmBridgeSide(cam,1,side,chapter==9);
            ggFilmBridgeSide(cam,0,side,chapter==9);
        }
    }else if(p==F_PORCH||p==F_HALL||p==F_ARCHIVE||p==F_FORGE){
        int wood=p==F_PORCH,frame=wood?24:GG_SLATE;
        for(int side=-1;side<=1;side+=2){
            int x=side*(p==F_HALL?322:335);
            ggFilmBox(cam,x,0,510,35,277,51,frame,wood?25:20,wood?26:GG_STONE);
            ggFilmPanel(cam,x-13,x-6,13,268,483,wood?25:GG_STONE);
            ggFilmBox(cam,x,0,510,44,17,59,frame,20,wood?25:GG_STONE);
        }
        ggFilmBox(cam,0,268,510,723,18,48,frame,wood?25:20,wood?26:GG_STONE);
        ggFilmPanel(cam,-360,360,269,273,484,wood?25:GG_STONE);
        if(p==F_ARCHIVE)ggFilmBox(cam,-320,12,540,67,96,45,25,24,26);
    }else if(p==F_GATE){
        ggFilmArch(cam,-300,552,0);
    }else if(p==F_VILLAGE&&chapter>=5){
        // A foreground canopy can really obscure a person passing it.
        ggFilmTree(cam,-365,558,124,ggFilmSceneTime(move*2),4);
    }
}
// Actors share the exact world camera used for geometry, so entering a room,
// approaching the foreground, and crossing the frame changes perspective
// rather than just dragging/zooming a flat sprite. Attachment and prop
// choreography still uses FilmBlocking's projected screen-space hand points.
static GgFilmFootprints ggFilmWorldBlocking(GgFilmCamera cam,const FilmBeat *beat,int elapsed,
                                           int chapter,FilmBlocking *b){
    GgFilmFootprints marks={0};
    if(beat->camera==F_INSERT)return marks;
    int paired=beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N;
    int leadLeft=paired&&filmActorLeftOf(beat->lead,beat->other);
    int compact=beat->camera==F_HAND&&paired&&beat->prop<F_PROP_N;
    int lx=paired?(leadLeft?-125:125):b->leadX<128?-125:125;
    int ox=paired?(leadLeft?125:-125):0;
    if(compact||beat->place==F_FORD){
        int halfWidth=beat->place==F_FORD?65:74;
        lx=paired?(leadLeft?-halfWidth:halfWidth):0;
        ox=-lx;
    }
    int active=beat->lead<F_ACTOR_N;
    int journey=active&&(beat->camera==F_TRACK||
                         (beat->camera==F_WIDE&&beat->prop>=F_PROP_N));
    int arrive=journey?filmEase(elapsed,180):256;
    int away=journey?(256-arrive):0;
    int lz=690+away*165/256;
    int oz=690;
    if(active){
        int travel=beat->place==F_FORD?16:66;
        int drift=journey?(lx<0?-1:1)*away*travel/256:0;
        int ground=ggFilmSurfaceHeight(beat->place,chapter,lx+drift,lz);
        GgV3 foot=ggV3(lx+drift,ground+(beat->lead==F_WISP?84:0),lz);
        marks.leadDepth=lz;
        if(beat->lead!=F_WISP)marks.lead=ggV3(foot.x,ground,foot.z);
        GgS2 pt=ggFilmProject(cam,foot);
        GgS2 head=ggFilmProject(cam,ggV3(foot.x,foot.y+(beat->lead==F_WISP?58:130),foot.z));
        b->leadX=pt.x;
        b->leadFeet=pt.y;
        b->leadHeight=pt.y-head.y;
    }
    if(paired){
        int ground=ggFilmSurfaceHeight(beat->place,chapter,ox,oz);
        GgV3 foot=ggV3(ox,ground+(beat->other==F_WISP?84:0),oz);
        marks.otherDepth=oz;
        if(beat->other!=F_WISP)marks.other=ggV3(foot.x,ground,foot.z);
        GgS2 pt=ggFilmProject(cam,foot);
        GgS2 head=ggFilmProject(cam,ggV3(foot.x,foot.y+(beat->other==F_WISP?58:130),foot.z));
        b->otherX=pt.x;
        b->otherFeet=pt.y;
        b->otherHeight=pt.y-head.y;
    }
    if(beat->place==F_COUCH){
        // Shoes are on the rug in front of the cushion. Hip artwork is at
        // source row 35 and the sole at 46 in the 48-row seated cels.
        const int seatZ=GG_FILM_COUCH_Z-51;
        if(active&&beat->lead!=F_WISP){
            int worldX=GG_FILM_COUCH_X+(paired?(leadLeft?-70:70):0);
            marks.lead=ggV3(worldX,1,seatZ);
            marks.leadDepth=seatZ;
            GgS2 pt=ggFilmProject(cam,marks.lead);
            b->leadX=pt.x;
            b->leadFeet=pt.y;
            b->leadHeight=130*260/(seatZ-cam.z);
        }
        if(paired&&beat->other!=F_WISP){
            int worldX=GG_FILM_COUCH_X+(leadLeft?70:-70);
            marks.other=ggV3(worldX,1,seatZ);
            marks.otherDepth=seatZ;
            GgS2 pt=ggFilmProject(cam,marks.other);
            b->otherX=pt.x;
            b->otherFeet=pt.y;
            b->otherHeight=130*260/(seatZ-cam.z);
        }
    }
    return marks;
}
static void ggFilmCastShadow(GgFilmCamera cam,GgV3 mark,int place,int chapter){
    if(!mark.z)return;
    int x=mark.x,y=mark.y+1,z=mark.z;
    int indoor=ggFilmRoom(place);
    int castX=50,castZ=38;
    // Keep the floor shadow's direction tied to the light visible in the set.
    if(place==F_PORCH){
        castX=(x-GG_FILM_PORCH_WINDOW_X)/9;
        castZ=(z-GG_FILM_PORCH_WINDOW_Z)/9;
    }else if(place==F_SHORE||place==F_GATE||place==F_RUINS){
        castX=(x-GG_FILM_DUSK_SUN_X)/15;
        castZ=(z-GG_FILM_DUSK_SUN_Z)/15;
    }
    u8 shadow=place==F_SHORE?26:place==F_STORM?GG_SLATE:
              indoor||place==F_FORD?24:place==F_GATE||place==F_RUINS?20:GG_PINE;
    GgV3 nearL=ggV3(x-22,y,z-18),nearR=ggV3(x+20,y,z-15);
    GgV3 farL=ggV3(x+castX-18,y,z+castZ+2);
    GgV3 farR=ggV3(x+castX+18,y,z+castZ-2);
    int contact=24;
    if(place==F_FORD&&mark.y>1){
        // Neither the contact core nor the tail can spill across the
        // edge of a deck board into the water.
        int limit=chapter>=GG_FILM_BRIDGE_CHAPTER?GG_FILM_BRIDGE_HALF:GG_FILM_FORD_HALF;
        GgV3 *corners[]={&nearL,&nearR,&farL,&farR};
        for(int i=0;i<4;i++){
            GgV3 *p=corners[i];
            if(p->x<-limit)p->x=(short)-limit;
            if(p->x>limit)p->x=(short)limit;
            p->y=(short)(ggFilmSurfaceHeight(place,chapter,p->x,p->z)+1);
        }
        if(contact>x+limit)contact=x+limit;
        if(contact>limit-x)contact=limit-x;
    }
    ggFilmQuad(cam,nearL,nearR,farR,farL,shadow);
    ggFilmGroundPatch(cam,x,y,z,2*contact,37,indoor?GG_OXBLOOD:shadow);
}
#endif
#endif
