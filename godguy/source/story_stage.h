// The chapter reels dress shared illustrated sets with cels, camera moves and
// timed subtitles. Playback draws into the same back page as the baked films.
#include "story_reels.h"

enum { FILM_TOP=24, FILM_BOTTOM=168, FILM_H=144, FILM_SECOND=60 };
#ifdef __3DS__
enum { FILM_STAGE_TICKS=3, FILM_PLATE_SCALE=2 };
#else
enum { FILM_STAGE_TICKS=5, FILM_PLATE_SCALE=1 };
#endif

_Static_assert(sizeof filmReels/sizeof *filmReels==sizeof fmvClips/sizeof *fmvClips,"Every film has an edit");
_Static_assert(sizeof filmSets/sizeof *filmSets==F_SET_N,"One plate per set");
_Static_assert(sizeof filmProps/sizeof *filmProps==F_PROP_N,"One cel per prop");
_Static_assert(sizeof filmCasts/sizeof *filmCasts==F_ACTOR_N*FILM_CEL_N,"All cast cels are present");
#ifdef __3DS__
_Static_assert(sizeof filmExtraReels/sizeof *filmExtraReels==sizeof filmReels/sizeof *filmReels,"Every film has a 3DS edit");
#endif

static const u8 filmOpeningScore[]={
    GM_GENESIS, GM_LANTERNRIVER, GM_CLAY, GM_GLASSRIVER, GM_LASTBEACON,
    GM_IRONSALT, GM_SKYWARD, GM_BESTIARY, GM_HEARTH, GM_SIEGE,
    GM_MILLIONLIVES, GM_FIELDSGOLD, GM_DREAMINGDEEP, GM_CROWNTHORNS, GM_WORLDTURNS,
};
_Static_assert(sizeof filmOpeningScore==sizeof filmReels/sizeof *filmReels,"Every film has a score");

static void filmScoreTurn(const FilmBeat *beat){
    if(beat==&siegeBeats[16]) storyScore=GM_LASTBEACON;
    else if(beat==&godhoodBeats[14]) storyScore=GM_HEARTH;
    else if(beat==&finaleBeats[17]) storyScore=GM_FIRSTSUN;
    else return;
    musReq(storyScore);
}

static int filmReelTicks(const FilmReel *reel){
    int ticks=0;
    for(int i=0;i<reel->count;i++) ticks+=reel->beats[i].seconds*FILM_SECOND;
    return ticks;
}
static int fmvLength(const FmvClip *clip){
    int id=(int)(clip-fmvClips);
    int length=clip->frames*FMV_TICKS+filmReelTicks(&filmReels[id]);
#ifdef __3DS__
    length+=filmReelTicks(&filmExtraReels[id]);
#endif
    return length;
}

typedef struct {
    int x, y, dx, dy;
} FilmFraming;

_Static_assert(FMV_W*FMV_H*FILM_PLATE_SCALE*FILM_PLATE_SCALE<=sizeof uiNew,"Backdrops share the transition buffer");

// Locked cameras: a cut chooses its composition, rather than zooming for
// the entire line. The only camera travel is a short, measured track.
static FilmFraming filmFraming(const FilmBeat *beat,int move){
    // Every shot is rendered at the same focal length. Tracking has only a
    // four-source-pixel margin and never performs a digital zoom.
    int w=beat->camera==F_TRACK?124:128;
    int h=w*FMV_H/FMV_W, sx=FMV_W-w, sy=FMV_H-h;
    int pan=128;
    if(beat->camera==F_TRACK) pan=96+move*64/256; // at most two output pixels
    FilmFraming frame={sx*pan*FILM_PLATE_SCALE,
                       sy*(beat->camera==F_INSERT?168:96)*FILM_PLATE_SCALE,
                       w*FILM_PLATE_SCALE,h*FILM_PLATE_SCALE*256/FILM_H};
    return frame;
}
static void filmDrawSet(const FilmBeat *beat,int move){
    FilmFraming camera=filmFraming(beat,move);
    u8 columns[256];
    int sx=camera.x, sy=camera.y;
    for(int x=0;x<256;x++,sx+=camera.dx) columns[x]=(u8)(sx>>8);
    for(int y=FILM_TOP;y<FILM_BOTTOM;y++,sy+=camera.dy){
        const u8 *row=uiNew+(sy>>8)*FMV_W*FILM_PLATE_SCALE;
        u8 *out=uiBuf+y*256;
        for(int x=0;x<256;x++) out[x]=fmvSlots[row[columns[x]]];
    }
}
static void filmDrawCel(const u8 *image,int sw,int sh,int center,int feet,int height,int flip){
    // Exact, centre-sampled 8.8 texel steps avoid cumulative rounding
    // drift and inconsistent left/right edge widths while scaling.
    // Actors and props are displayed close to their native cel resolution.
    int width=height*sw/sh, left=center-width/2, top=feet-height;
    if(width<=0||height<=0) return;
    // One 16.16 step calculation per axis; no divides in the inner
    // pixel loop (important for DS ARM9 frame and music timing).
    int dx=(sw<<16)/width, dy=(sh<<16)/height, sy=dy/2;
    for(int y=0;y<height;y++,sy+=dy){
        int dstY=top+y;
        if(dstY<FILM_TOP||dstY>=FILM_BOTTOM) continue;
        int sourceY=sy>>16;
        if(sourceY>=sh) sourceY=sh-1;
        u8 *out=uiBuf+dstY*256;
        const u8 *source=image+sourceY*sw;
        int sx=dx/2;
        for(int x=0;x<width;x++,sx+=dx){
            int target=left+x;
            if((unsigned)target>=256) continue;
            int sourceX=sx>>16;
            if(sourceX>=sw) sourceX=sw-1;
            int color=source[flip?sw-1-sourceX:sourceX];
            if(color!=255) out[target]=fmvSlots[color];
        }
    }
}
static void filmDrawActor(int who,int pose,int x,int feet,int h,int flip){
    if(who>=F_ACTOR_N) return;
    u8 pixels[FILM_CEL_W*FILM_CEL_H];
    ggUnpack(filmCasts[who*FILM_CEL_N+pose],pixels,sizeof pixels);
    filmDrawCel(pixels,FILM_CEL_W,FILM_CEL_H,x,feet,h,flip);
}
static void filmDrawProp(int prop,int x,int feet,int height){
    if(prop>=F_PROP_N) return;
    u8 pixels[FILM_PROP_SIZE*FILM_PROP_SIZE];
    ggUnpack(filmProps[prop],pixels,sizeof pixels);
    filmDrawCel(pixels,FILM_PROP_SIZE,FILM_PROP_SIZE,x,feet,height,0);
}

static int filmEase(int elapsed,int total){
    int u=elapsed>=total?256:(elapsed*256)/total;
    return (u*u*(768-2*u))>>16;
}
static int filmPulse(int phase,int range){
    int at=phase%(2*range);
    return at<range?at:2*range-at;
}

// An actor occupies one side of the set for the *whole* conversation.
// Switching speakers must not teleport Wisp, Mira or the smith across a cut.
typedef struct {
    int leadX, otherX, leadFeet, otherFeet, leadHeight, otherHeight;
} FilmBlocking;
static int filmScreenOrder(int who){
    static const u8 rank[F_ACTOR_N]={6,1,3,0,5,4,2,1};
    return who<F_ACTOR_N?rank[who]:0;
}
static int filmActorLeftOf(int actor,int partner){
    int a=filmScreenOrder(actor),b=filmScreenOrder(partner);
    return a==b?actor<partner:a<b;
}
static int filmStageHeight(int who,int humanHeight){
    // Preserve Wisp's physical scale in every shot.
    if(who==F_WISP) return humanHeight>=48?24:21;
    return humanHeight;
}
static FilmBlocking filmBlocking(const FilmBeat *beat,int elapsed){
    int paired=beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N;
    // Screen direction is fixed even as the shot type changes.
    // For an actual hand-off both performers move close enough that
    // their hands can touch the same object. All other frames keep the
    // established screen-side positions.
    int compact=beat->camera==F_HAND&&paired&&beat->prop<F_PROP_N;
    int left=compact?106:85,right=compact?150:171;
    int leadLeft=paired&&filmActorLeftOf(beat->lead,beat->other);
    int leadX=paired?(leadLeft?left:right):
                beat->lead==F_WISP?171:filmScreenOrder(beat->lead)<=2?85:171;
    int otherX=paired?(leadLeft?right:left):0;
    // A 34-pixel source cel never expands into a 92-pixel 'close-up'.
    // Composition changes via gestures, eyelines and cuts, not blown-up art.
    int h=beat->camera==F_WIDE?43:beat->camera==F_NEAR?49:46;
    int feet=158;
    if(beat->camera==F_TRACK&&beat->lead<F_ACTOR_N){
        int arrival=filmEase(elapsed,140)*19/256;
        leadX+=(paired&&!leadLeft?1:-1)*(19-arrival);
    }
    // Restrained one-pixel acting, with a held pose instead of a bounce.
    int breath=(elapsed/72)&1;
    FilmBlocking b={leadX,otherX,
        feet-breath-(beat->lead==F_WISP?33:0),
        feet-breath-(beat->other==F_WISP?33:0),
        filmStageHeight(beat->lead,h),filmStageHeight(beat->other,h)};
    return b;
}

// All atmosphere has an in-world source. No drifting particles in empty
// rooms or random streaks crossing a character's face.
static void filmAtmosphere(const FilmBeat *beat,int t){
    static const u8 lightX[F_SET_N]={30,160,221,32,145,42,193,210,221,128,129,205,206,63};
    static const u8 lightY[F_SET_N]={41,151,110,141,150,126,130,142,120,145,83,141,111,145};
    int x=lightX[beat->place],y=lightY[beat->place];
    // A fixed lantern/star flicker supplies motion without floating props.
    // Soft, anchored light changes every 0.4 seconds; do not manufacture
    // constant flashing solely to satisfy a frame-difference test.
    uiPx(x,y,fmvSlots[(t/24)&1?10:9]);
    if(beat->place==F_STORM){
#ifdef __3DS__
        const int count=12;
#else
        const int count=7;
#endif
        for(int i=0;i<count;i++){
            int rx=(i*59+13)&255,ry=FILM_TOP+((i*37+t/2)%FILM_H);
            uiRect(rx,ry,1,3,fmvSlots[22]);
        }
    } else if(beat->place==F_SHORE||beat->place==F_FORD){
        for(int i=0;i<4;i++){
            int rx=63+i*41,ry=147+i%3*3;
            uiRect(rx+((t/12+i)&1),ry,6,1,fmvSlots[14]);
        }
    } else if(beat->place==F_FORGE||beat->place==F_HEARTH){
        for(int i=0;i<3;i++) uiPx(189+i*8,135+((t/8+i)&3),fmvSlots[i&1?9:8]);
    }
}
static void filmCaption(const FilmBeat *beat,int shown){
    uiRect(0,168,256,24,U_SHADOW);
    if(!shown||!beat->line) return;
    const char *name=beat->lead==F_ELDER?"MIRA":beat->lead==F_DAVE?"DAVE":
                     beat->lead<W_N?whoNm[beat->lead]:NULL;
    if(name){
        uiTextC(128,171,name,U_GOLD,1);
        uiTextC(128,181,beat->line,U_WHITE,1);
    } else uiTextC(128,177,beat->line,U_WHITE,1);
}
static void filmBars(const char *title){
    uiRect(0,0,256,24,U_SHADOW);
    uiRect(0,168,256,24,U_SHADOW);
    uiTextC(128,4,title,U_GOLD,1);
    uiTextR(249,15,"A / START  SKIP",U_DIM);
}

static const FilmBeat *filmFindBeat(const FilmReel *reel,int *elapsed){
    for(int i=0;i<reel->count;i++){
        const FilmBeat *beat=&reel->beats[i];
        int length=beat->seconds*FILM_SECOND;
        if(*elapsed<length) return beat;
        *elapsed-=length;
    }
    return NULL;
}
static const FilmBeat *filmBeatAt(const FmvClip *clip,int *elapsed){
    int id=(int)(clip-fmvClips);
    const FilmBeat *beat=filmFindBeat(&filmReels[id],elapsed);
#ifdef __3DS__
    if(!beat) beat=filmFindBeat(&filmExtraReels[id],elapsed);
#endif
    return beat;
}
// A drawn cel has a real reachable hand, not a generic "object beside
// the actor" point. The same cel geometry is shared by DS and 3DS.
typedef struct { int x,y; } FilmPoint;
static FilmPoint filmHandPoint(int who,int pose,int cx,int feet,int height,int flip){
    int width=height*FILM_CEL_W/FILM_CEL_H;
    // Source art has an extended right arm in cel 3. Its visible hand is
    // around (24,18) of the 28x34 cel; idle hand around (19,24).
    int sx=pose==3?24:19, sy=pose==3?18:24;
    int x=cx-width/2+(flip?FILM_CEL_W-1-sx:sx)*width/FILM_CEL_W;
    int y=feet-height+sy*height/FILM_CEL_H;
    if(who==F_WISP){
        // The spark has no human fingers. A seed/leaf must stay grounded.
        x=cx+(flip?-8:8);
        y=feet+7;
    }
    FilmPoint point={x,y};
    return point;
}
static FilmPoint filmPropGripPoint(int prop){
    // Exact attachment points on the 20x20 prop drawings: cup handle,
    // hammer shaft, lamp loop, quill shaft, bread crust, and so on.
    static const u8 gx[F_PROP_N]={17,10,6,5,7,10,11,9,14,11,11,11};
    static const u8 gy[F_PROP_N]={11,10,16,16,15,3,13,12,11,12,12,11};
    FilmPoint p={gx[prop],gy[prop]};
    return p;
}
static FilmPoint filmSurfacePoint(const FilmBeat *beat,int handX){
    int ground=beat->place==F_SHORE||beat->place==F_STORM||
               beat->place==F_FORD||beat->place==F_RUINS||
               beat->place==F_VILLAGE||beat->place==F_GATE;
    FilmPoint p={handX+((handX>128)?-14:14),ground?156:150};
    return p;
}
static int filmActionOf(const FilmBeat *beat){
    if(beat->action!=F_ACT_AUTO) return beat->action;
    if(beat->prop>=F_PROP_N) return F_ACT_AUTO;
    if(beat->camera==F_HAND&&beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N)
        return F_ACT_PASS;
    return F_ACT_AUTO;
}
static void filmSurface(int place,int x,int y){
    int outdoor=place==F_SHORE||place==F_STORM||place==F_FORD||
                place==F_VILLAGE||place==F_GATE||place==F_RUINS;
    if(outdoor){
        // A persistent patch of soil/stone under a grounded object.
        uiRect(x-16,y,32,2,fmvSlots[20]);
        uiRect(x-12,y+2,24,1,fmvSlots[21]);
    }else{
        // Narrow table/sill with two real supporting legs.
        uiRect(x-24,y,48,3,fmvSlots[25]);
        uiRect(x-18,y+3,3,14,fmvSlots[24]);
        uiRect(x+15,y+3,3,14,fmvSlots[24]);
    }
}
static FilmPoint filmPropCenterForGrip(int prop,int size,FilmPoint hand){
    FilmPoint pin=filmPropGripPoint(prop);
    FilmPoint result={
        hand.x-((pin.x-10)*size)/FILM_PROP_SIZE,
        hand.y+(FILM_PROP_SIZE-pin.y)*size/FILM_PROP_SIZE
    };
    return result;
}
static int filmLerp(int start,int finish,int step){
    return start+(finish-start)*step/256;
}
static void filmGripOverlay(int who,FilmPoint hand,int prop){
    // An actual thumb/finger over the grip closes the silhouette.
    static const u8 skin[F_ACTOR_N]={15,27,18,27,26,21,27,26};
    int color=fmvSlots[skin[who]];
    if(prop==F_HAMMER||prop==F_QUILL||prop==F_SPOON)
        uiRect(hand.x-1,hand.y,3,2,color);
    else
        uiRect(hand.x-2,hand.y-1,4,2,color);
}
static void filmDrawPropAction(const FilmBeat *beat,int elapsed,int leadPose,int otherPose,FilmBlocking b){
    int prop=beat->prop;
    if(prop>=F_PROP_N)return;
    if(prop==F_GOAT){
        // The goat is a character on the floor, not a levitating hand prop.
        int walk=filmEase(elapsed,beat->seconds*FILM_SECOND);
        filmDrawProp(prop,58+walk*49/256,157,24);
        return;
    }
    int size=beat->camera==F_INSERT?27:beat->camera==F_WIDE?17:19;
    int paired=beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N;
    int flipLead=paired&&b.leadX>b.otherX, flipOther=paired&&b.otherX>b.leadX;
    if(beat->camera==F_INSERT||beat->lead>=F_ACTOR_N){
        // Insert and establishing objects *start* on a named support.
        FilmPoint table={128,149};
        filmSurface(beat->place,table.x,table.y);
        // Resting props use their literal lower edge on the table top.
        // Grip/pivot coordinates apply only while a character HOLDS one.
        int feet=table.y;
        if(beat->camera==F_INSERT&&prop==F_HAMMER&&elapsed>=72&&elapsed<142)
            feet-=2;
        filmDrawProp(prop,table.x,feet,size);
        return;
    }
    int action=filmActionOf(beat);
    FilmPoint leadHand=filmHandPoint(beat->lead,leadPose,b.leadX,b.leadFeet,
                                   b.leadHeight,flipLead);
    FilmPoint grip=leadHand;
    FilmPoint ground=filmSurfacePoint(beat,leadHand.x);
    FilmPoint pivot=filmPropGripPoint(prop);
    FilmPoint restGrip={
        ground.x+(pivot.x-10)*size/FILM_PROP_SIZE,
        ground.y-size+pivot.y*size/FILM_PROP_SIZE
    };
    // Wisp has no fingers: seed, leaf and cup stay physically on the floor.
    if(beat->lead==F_WISP||action==F_ACT_APPROACH){
        filmSurface(beat->place,ground.x,ground.y);
        filmDrawProp(prop,ground.x,ground.y,size);
        return;
    }
    int holding=1, thumb=1;
    if(action==F_ACT_TAKE){
        filmSurface(beat->place,ground.x,ground.y);
        int lift=filmEase(elapsed>50?elapsed-50:0,145);
        grip.x=filmLerp(restGrip.x,leadHand.x,lift);
        grip.y=filmLerp(restGrip.y,leadHand.y,lift);
        holding=lift>150;
        thumb=holding;
    }else if(action==F_ACT_PUT){
        filmSurface(beat->place,ground.x,ground.y);
        int down=filmEase(elapsed>55?elapsed-55:0,170);
        grip.x=filmLerp(leadHand.x,restGrip.x,down);
        grip.y=filmLerp(leadHand.y,restGrip.y,down);
        holding=down<165;
        thumb=holding;
    }else if(action==F_ACT_PASS&&paired&&beat->other!=F_WISP){
        FilmPoint otherHand=filmHandPoint(beat->other,otherPose,b.otherX,
                           b.otherFeet,b.otherHeight,flipOther);
        int handoff=filmEase(elapsed>60?elapsed-60:0,160);
        grip.x=filmLerp(leadHand.x,otherHand.x,handoff);
        grip.y=filmLerp(leadHand.y,otherHand.y,handoff);
        if(handoff>190){holding=0;thumb=0;}
        FilmPoint center=filmPropCenterForGrip(prop,size,grip);
        filmDrawProp(prop,center.x,center.y,size);
        if(thumb)filmGripOverlay(beat->lead,grip,prop);
        if(!holding)filmGripOverlay(beat->other,grip,prop);
        return;
    }else if(action==F_ACT_WORK){
        // Controlled tool strike: the hammer remains in the hand.
        if(elapsed>70&&elapsed<250) grip.y+=((elapsed/42)&1)*2;
    }
    FilmPoint center=filmPropCenterForGrip(prop,size,grip);
    filmDrawProp(prop,center.x,center.y,size);
    if(thumb)filmGripOverlay(beat->lead,grip,prop);
}

// A location change is a new scene; keep an establishing look on the set
// before the actors enter. Near/insert cuts within one set are continuous.
static const FilmBeat *filmPreviousBeat(const FmvClip *clip,const FilmBeat *beat){
    int i=(int)(clip-fmvClips);
    const FilmReel *main=&filmReels[i];
    for(int j=0;j<main->count;j++)
        if(beat==main->beats+j) return j?main->beats+j-1:NULL;
#ifdef __3DS__
    const FilmReel *extra=&filmExtraReels[i];
    for(int j=0;j<extra->count;j++)
        if(beat==extra->beats+j) return j?extra->beats+j-1:main->beats+main->count-1;
#endif
    return NULL;
}
static void filmWorldContinuity(const FmvClip *clip,const FilmBeat *beat,int elapsed){
    int chapter=(int)(clip-fmvClips);
    if(chapter==1){
        if(beat->place==F_PORCH){
            // The same storm remains visible from the sheltered porch.
            uiRect(211,56,30,38,fmvSlots[2]);
            uiRect(212,57,28,36,fmvSlots[13]);
            uiRect(211,56,30,2,fmvSlots[24]);
            uiRect(211,92,30,3,fmvSlots[24]);
            for(int i=0;i<4;i++) uiRect(216+i*6,61+(elapsed/7+i*13)%24,1,4,fmvSlots[22]);
            // The cup remains on the sill after Mira puts it there.
            if(beat==firstRainBeats+6){
                filmSurface(F_PORCH,133,150);
                filmDrawProp(F_CUP,133,150,19);
            }
        }else if(beat->place==F_VILLAGE){
            // After the rain, puddles keep the prior weather visible.
            for(int i=0;i<5;i++)uiRect(37+i*40,152+i%2*3,13,1,fmvSlots[14]);
        }
    }else if(chapter==2&&beat->place==F_VILLAGE){
        // The unfinished roof stays in the SAME position across cuts.
        int finished=beat==roofBeats+5||beat==roofBeats+6;
        uiRect(49,117,2,31,fmvSlots[24]);
        uiRect(105,117,2,31,fmvSlots[24]);
        uiRect(48,117,60,3,fmvSlots[25]);
        if(finished){
            for(int i=0;i<6;i++)uiRect(49+i*10,110+i%2,10,4,fmvSlots[24]);
            uiRect(47,115,64,3,fmvSlots[25]);
            // Thorgrim leaves his hammer at the completed work site.
            filmDrawProp(F_HAMMER,117,156,17);
        }else{
            uiRect(48,111,20,3,fmvSlots[24]);
            uiRect(85,111,20,3,fmvSlots[24]);
        }
    }else if(chapter==0&&beat->place==F_SHORE){
        // The sprout remains where the seed went into the earth.
        if(beat==firstLightBeats+5||beat==firstLightBeats+6){
            uiRect(129,149,2,7,fmvSlots[17]);
            uiRect(126,150,4,2,fmvSlots[14]);
            uiRect(131,150,4,2,fmvSlots[14]);
        }
    }
}

static void filmDrawBeat(const FmvClip *clip,const FilmBeat *beat,int elapsed){
    static u8 establishing;
    if(!elapsed){
#ifdef __3DS__
        ggUnpack(filmWides[beat->place],uiNew,FMV_W*FMV_H*4);
#else
        ggUnpack(filmSets[beat->place],uiNew,FMV_W*FMV_H);
#endif
        if(storyScore>=0) filmScoreTurn(beat);
        if(beat->sound) fxReq(beat->sound);
        filmBars(clip->title);
        const FilmBeat *previous=filmPreviousBeat(clip,beat);
        establishing=previous&&previous->place!=beat->place;
    }
    int total=beat->seconds*FILM_SECOND;
    int move=filmEase(elapsed,total);
    FilmBlocking block=filmBlocking(beat,elapsed);
    filmDrawSet(beat,move);
    filmWorldContinuity(clip,beat,elapsed);
    filmAtmosphere(beat,elapsed);

    int paired=beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N;
    int action=filmActionOf(beat);
    int leadPose=0,otherPose=0;
    if(beat->camera==F_TRACK&&elapsed<140) leadPose=1+(elapsed/18&1);
    else if((beat->prop<F_PROP_N&&(action==F_ACT_TAKE||action==F_ACT_PUT||
             action==F_ACT_WORK||action==F_ACT_PASS))||beat->camera==F_HAND){
        if(elapsed>30&&elapsed<310)leadPose=3;
        if(action==F_ACT_PASS&&paired&&elapsed>55&&elapsed<310)otherPose=3;
    }else if(beat->line&&elapsed>=130&&elapsed<220)leadPose=3;

    int showCast=!(establishing&&elapsed<39);
    if(beat->camera!=F_INSERT&&showCast){
        filmDrawActor(beat->other,otherPose,block.otherX,block.otherFeet,
                      block.otherHeight,paired&&block.otherX>block.leadX);
        filmDrawActor(beat->lead,leadPose,block.leadX,block.leadFeet,
                      block.leadHeight,paired&&block.leadX>block.otherX);
    }
    if(showCast||beat->camera==F_INSERT)
        filmDrawPropAction(beat,elapsed,leadPose,otherPose,block);
    filmCaption(beat,elapsed>=100&&elapsed<total-20);
}
static int fmvRenderAt(const FmvClip *clip,int elapsed){
    int baked=clip->frames*FMV_TICKS;
    if(elapsed<baked){
        if(elapsed%FMV_TICKS) return 0;
        fmvDecode(clip,elapsed/FMV_TICKS);
        return 1;
    }
    elapsed-=baked;
    const FilmBeat *beat=filmBeatAt(clip,&elapsed);
    if(!beat||elapsed%FILM_STAGE_TICKS) return 0;
    filmDrawBeat(clip,beat,elapsed);
    return 1;
}
