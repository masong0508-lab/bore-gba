// The chapter reels dress shared illustrated sets with cels, camera moves and
// timed subtitles. Playback draws into the same back page as the baked films.
#include "story_reels.h"
#ifdef __3DS__
#include "story_film_hq_data.h"
#endif

enum { FILM_TOP=24, FILM_BOTTOM=168, FILM_H=144, FILM_SECOND=60 };
#ifdef __3DS__
enum { FILM_STAGE_TICKS=3, FILM_PLATE_SCALE=2 };
#else
enum { FILM_STAGE_TICKS=5, FILM_PLATE_SCALE=1 };
#endif

_Static_assert(sizeof filmReels/sizeof *filmReels==sizeof fmvClips/sizeof *fmvClips,"Every film has an edit");
_Static_assert(sizeof filmSets/sizeof *filmSets==F_SET_N,"One plate per set");
_Static_assert(sizeof filmProps/sizeof *filmProps==F_PROP_N,"One cel per prop");
// Shared DS movie cels stay at eight original cast members. Nessa and
// Serin's dedicated additional twelve-pose acting cels are 3DS-only.
_Static_assert(sizeof filmCasts/sizeof *filmCasts==(F_ELDER+1)*FILM_CEL_N,"All shared cast cels are present");
#ifdef __3DS__
_Static_assert(sizeof filmExtraReels/sizeof *filmExtraReels==sizeof filmReels/sizeof *filmReels,"Every film has a 3DS edit");
_Static_assert(sizeof filmFeatureReels/sizeof *filmFeatureReels==sizeof filmReels/sizeof *filmReels,
               "Every 3DS chapter has a feature-film sequence");
_Static_assert(FILM_BONUS_N==6,"The 3DS film archive must contain six stories");
_Static_assert(sizeof filmBonusTitles/sizeof *filmBonusTitles==FILM_BONUS_N,
               "Each bonus film has a visible archive title");
_Static_assert(sizeof filmHqCasts/sizeof *filmHqCasts==F_ACTOR_N*FILM_HQ_N,
               "Every 3DS actor has twelve detailed acting keys");
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
#ifdef __3DS__
    // Longer films need musical turning points connected to consequences,
    // not an endlessly looping establishing cue or random effect hits.
    else if(beat==&featureFord[14]) storyScore=GM_GLASSRIVER;
    else if(beat==&featureInk[14]) storyScore=GM_BESTIARY;
    else if(beat==&featureSiege[11]) storyScore=GM_LASTBEACON;
    else if(beat==&featureGodhood[12]) storyScore=GM_HEARTH;
    else if(beat==&featureFinale[21]) storyScore=GM_FIRSTSUN;
#endif
    else return;
    musReq(storyScore);
}

#ifdef __3DS__
// -1: regular campaign; 0..5: selected unlockable chapter in the Extras
// film archive. This affects playback only, never world save data.
static int filmBonusIndex=-1;
#endif
// One continuous clock across all staged shots in a chapter. An actor's
// gestures use local beat time; smoke, rain, wind and flames must not
// visibly jump backward after every edit. No new allocation.
static int filmSceneryClock;
static int filmReelTicks(const FilmReel *reel){
    int ticks=0;
    for(int i=0;i<reel->count;i++) ticks+=reel->beats[i].seconds*FILM_SECOND;
    return ticks;
}
static int fmvLength(const FmvClip *clip){
#ifdef __3DS__
    if(filmBonusIndex>=0&&filmBonusIndex<FILM_BONUS_N)
        return filmReelTicks(&filmBonusReels[filmBonusIndex]);
#endif
    int id=(int)(clip-fmvClips);
    int length=clip->frames*FMV_TICKS+filmReelTicks(&filmReels[id]);
#ifdef __3DS__
    length+=filmReelTicks(&filmExtraReels[id]);
    length+=filmReelTicks(&filmFeatureReels[id]);
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
#ifdef __3DS__
        // Depth-graded lateral tracking: far scenery moves less than the
        // near ground. Keep the set's original pixel geometry and focal
        // length, without enlarging/blurring the illustrated backdrop.
        // The shift is subtle (maximum two source pixels) and is only
        // applied during an authored tracking shot.
        int near=y-FILM_TOP;
        int parallax=beat->camera==F_TRACK?
            (move-128)*near/(FILM_H*64):0;
        for(int x=0;x<256;x++){
            int source=x+parallax;
            if(source<0)source=0;
            if(source>255)source=255;
            out[x]=fmvSlots[row[columns[source]]];
        }
#else
        for(int x=0;x<256;x++) out[x]=fmvSlots[row[columns[x]]];
#endif
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
#ifdef __3DS__
    // 40x48 native authored detail: do NOT magnify the 28x34 DS art.
    // Scratch memory remains on the call stack and is shared by the two
    // sequential actors, not allocated or cached per frame.
    u8 pixels[FILM_HQ_W*FILM_HQ_H];
    if(pose<0||pose>=FILM_HQ_N) pose=0;
    ggUnpack(filmHqCasts[who*FILM_HQ_N+pose],pixels,sizeof pixels);
    filmDrawCel(pixels,FILM_HQ_W,FILM_HQ_H,x,feet,h,flip);
#else
    u8 pixels[FILM_CEL_W*FILM_CEL_H];
    if(pose<0||pose>=FILM_CEL_N) pose=0;
    ggUnpack(filmCasts[who*FILM_CEL_N+pose],pixels,sizeof pixels);
    filmDrawCel(pixels,FILM_CEL_W,FILM_CEL_H,x,feet,h,flip);
#endif
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
#ifndef __3DS__
static int filmDSSpeechEnd(const FilmBeat *beat){
    static const FilmBeat *last;
    static int end;
    if(beat!=last){
        last=beat;
        int n=0;
        if(beat->line)
            while(beat->line[n]&&n<41)n++;
        end=140+n*5;
        int limit=beat->seconds*FILM_SECOND-55;
        if(end>limit)end=limit;
    }
    return end;
}
#endif
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
#ifdef __3DS__
    static const u8 rank[F_ACTOR_N]={6,1,3,0,5,4,2,1,2,7};
#else
    static const u8 rank[F_ACTOR_N]={6,1,3,0,5,4,2,1};
#endif
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
#ifdef __3DS__
    // Walk the set during establishing action, rather than merely changing
    // expressions in place. This is world-ground travel, not cel zooming.
    // Arrival settles BEFORE the character interacts with a hand prop.
    if(beat->camera==F_WIDE&&beat->lead<F_ACTOR_N&&
       beat->prop>=F_PROP_N){
        int approach=filmEase(elapsed,180);
        leadX+=(paired&&!leadLeft?1:-1)*(12-12*approach/256);
    }
#endif
    // Restrained one-pixel acting, with a held pose instead of a bounce.
    int breath=(elapsed/72)&1;
#ifdef __3DS__
    // Footfall is tied to actual travel, not to dialogue or a static pose.
    int walking=(beat->camera==F_TRACK&&elapsed<140)||
                (beat->camera==F_WIDE&&beat->prop>=F_PROP_N&&elapsed<180);
    int stride=walking?((elapsed/18)&1):0;
#else
    int stride=0;
#endif
    FilmBlocking b={leadX,otherX,
        feet-breath-stride-(beat->lead==F_WISP?33:0),
        feet-breath-(beat->other==F_WISP?33:0),
        filmStageHeight(beat->lead,h),filmStageHeight(beat->other,h)};
    return b;
}

// Native DS/DSi environmental choreography. The base film remains
// 256x192 with six compressed 28x34 actor keys. Each scene uses a handful
// of world-anchored drawing commands rather than a new sprite sheet,
// framebuffer, palette, or heap allocation.
#ifndef __3DS__
static void filmDSLivingSets(const FilmBeat *beat,int t,int chapter){
    int p=beat->place;
    if(p==F_VILLAGE){
        if(chapter>=2){
            // Smoke rises from a roof, never from the speaker's position.
            for(int i=0;i<3;i++){
                int rise=(t/22+i*13)%29;
                uiRect(52+i+(rise/9),106-rise,2+(i&1),2,
                       fmvSlots[i&1?21:22]);
            }
        }
        if(chapter>=5){
            // A travelling market cart gives the village a genuine
            // background activity track, with its wheels attached.
            int x=17+(t/7)%164;
            uiRect(x,145,27,9,fmvSlots[24]);
            uiRect(x+2,143,23,3,fmvSlots[25]);
            uiRect(x+4,154,5,5,fmvSlots[20]);
            uiRect(x+20,154,5,5,fmvSlots[20]);
            uiRect(x+9,140,9,5,fmvSlots[21]);
            uiRect(x+11,139,5,2,fmvSlots[26]);
        }
        if(chapter==2&&beat==roofBeats+2){
            // The hammer disturbs real timber dust at the worksite.
            for(int i=0;i<3;i++){
                int y=123-(t/9+i*7)%13;
                uiRect(94+i*6,y,2,2,fmvSlots[25]);
            }
        }
    }else if(p==F_PORCH){
        // Curtains are hinged to the same rain window on every take.
        int sway=((t/36)%5)-2;
        uiRect(205+sway,57,4,25,fmvSlots[16]);
        uiRect(241-sway,57,3,25,fmvSlots[16]);
        if(chapter==1){
            // The doorway is gradually revealed only after the arrival.
            int open=t<420?t/52:8;
            uiRect(30,63,4,43,fmvSlots[24]);
            uiRect(33,65,2+open,39,fmvSlots[2]);
        }
    }else if(p==F_FORGE){
        // Near the forge mouth: three rising sparks, not screen-wide fog.
        for(int i=0;i<4;i++){
            int rise=(t/10+i*9)%24;
            int x=207+i*5+(rise/12);
            uiRect(x,139-rise,1+(i&1),2,fmvSlots[i&1?8:9]);
        }
    }else if(p==F_ARCHIVE){
        // Open folio between the two actors; the right leaf lifts as
        // pages are turned, and the writing remains on the paper.
        int turn=((t/36)%7)-3;
        uiRect(112,145,17,10,fmvSlots[23]);
        uiRect(130,145+turn/2,17,10,fmvSlots[26]);
        uiRect(129,145,2,11,fmvSlots[24]);
        for(int i=0;i<3;i++){
            uiRect(115,148+i*2,10-i,1,fmvSlots[25]);
            uiRect(134,148+turn/2+i*2,10-i,1,fmvSlots[25]);
        }
    }else if(p==F_HEARTH){
        for(int i=0;i<3;i++){
            int lift=(t/14+i*5)%8;
            uiRect(124+i*5,151-lift,2,7-lift/3,
                   fmvSlots[i&1?9:8]);
        }
    }else if(p==F_FORD||p==F_SHORE){
        for(int i=0;i<6;i++){
            int x=48+i*27+((t/12+i*3)%11);
            int y=144+(i*7)%15;
            uiRect(x,y,8+(i&3),1,fmvSlots[i&1?13:14]);
        }
    }else if(p==F_GATE||p==F_RUINS){
        // Guard fire is fixed to two sconces. Only the flame moves.
        if(p==F_GATE)
            for(int i=0;i<2;i++){
                int x=56+i*139;
                uiRect(x,109-((t/15+i)%3),2,6,fmvSlots[9]);
                uiPx(x+1,108-((t/15+i)%3),fmvSlots[10]);
            }
    }else if(p==F_HALL||p==F_THRONE){
        // Two banners respond to wind without sliding off their rods.
        int flutter=(t/38)%5;
        uiRect(32,79,3+flutter,24,fmvSlots[5]);
        uiRect(219-flutter,79,3+flutter,24,fmvSlots[5]);
    }
}
static void filmDSForeground(const FilmBeat *beat){
    // Small depth cues at the correct contact plane. These objects stay
    // behind hand gestures and below dialogue; they never follow faces.
    if(beat->place==F_PORCH){
        uiRect(105,159,51,3,fmvSlots[24]);
        uiRect(108,162,3,3,fmvSlots[25]);
        uiRect(151,162,3,3,fmvSlots[25]);
    }else if(beat->place==F_ARCHIVE){
        uiRect(108,158,46,3,fmvSlots[24]);
    }
}
#endif

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
#ifdef __3DS__
                     beat->lead==F_NESSA?"NESSA":beat->lead==F_SERIN?"SERIN":
#endif
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
#ifdef __3DS__
    if(filmBonusIndex>=0&&filmBonusIndex<FILM_BONUS_N)
        return filmFindBeat(&filmBonusReels[filmBonusIndex],elapsed);
#endif
    int id=(int)(clip-fmvClips);
    const FilmBeat *beat=filmFindBeat(&filmReels[id],elapsed);
#ifdef __3DS__
    if(!beat) beat=filmFindBeat(&filmExtraReels[id],elapsed);
    if(!beat) beat=filmFindBeat(&filmFeatureReels[id],elapsed);
#endif
    return beat;
}
// A drawn cel has a real reachable hand, not a generic "object beside
// the actor" point. The same cel geometry is shared by DS and 3DS.
typedef struct { int x,y; } FilmPoint;
static FilmPoint filmHandPoint(int who,int pose,int cx,int feet,int height,int flip){
#ifdef __3DS__
    // High-detail actor() draws a real 40x48 hand for each acting pose.
    static const u8 hx[FILM_HQ_N]={26,25,27,35,30,30,26,30,28,35,30,34};
    static const u8 hy[FILM_HQ_N]={34,34,34,25,27,27,34,23,15,37,32,29};
    if(pose<0||pose>=FILM_HQ_N) pose=0;
    int sw=FILM_HQ_W, sh=FILM_HQ_H;
    int sx=hx[pose],sy=hy[pose];
#else
    int sw=FILM_CEL_W,sh=FILM_CEL_H;
    int sx=pose==3?24:19,sy=pose==3?18:24;
#endif
    int width=height*sw/sh;
    int x=cx-width/2+(flip?sw-1-sx:sx)*width/sw;
    int y=feet-height+sy*height/sh;
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
    // Interior furniture belongs to the set, not to the currently speaking
    // actor: the cup stays on the same sill when the edit reverses.
    if(beat->place==F_PORCH){FilmPoint p={128,150};return p;}
    if(beat->place==F_FORGE){FilmPoint p={128,150};return p;}
    if(beat->place==F_ARCHIVE){FilmPoint p={128,151};return p;}
    if(beat->place==F_HEARTH){FilmPoint p={128,151};return p;}
    if(beat->place==F_COUCH){FilmPoint p={128,152};return p;}
    if(beat->place==F_HALL){FilmPoint p={128,150};return p;}
    if(beat->place==F_THRONE){FilmPoint p={128,150};return p;}
    FilmPoint p={handX+((handX>128)?-14:14),156};
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
static int filmHandColor(int who){
#ifdef __3DS__
    static const u8 skin[F_ACTOR_N]={15,27,18,27,26,21,27,26,26,27};
#else
    static const u8 skin[F_ACTOR_N]={15,27,18,27,26,21,27,26};
#endif
    return fmvSlots[skin[who]];
}
static void filmArmReach(int who,FilmPoint from,FilmPoint target){
    // Attach the hand to the object while lifting or handing it over.
    // A single 2-pixel line costs at most 31 iterations, no new art buffers.
    int dx=target.x-from.x,dy=target.y-from.y;
    int adx=dx<0?-dx:dx,ady=dy<0?-dy:dy;
    int steps=adx>ady?adx:ady;
    if(!steps)return;
    if(steps>31)steps=31;
    int color=filmHandColor(who);
    int x=from.x*256, y=from.y*256;
    int sx=dx*256/steps,sy=dy*256/steps;
    for(int i=1;i<=steps;i++){
        x+=sx; y+=sy;
        uiRect(x/256,y/256,2,2,color);
    }
}
static void filmGripOverlay(int who,FilmPoint hand,int prop){
    int color=filmHandColor(who);
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
        // The actor reaches down BEFORE the object starts lifting.
        holding=1;
        thumb=elapsed>=40;
    }else if(action==F_ACT_PUT){
        filmSurface(beat->place,ground.x,ground.y);
        int down=filmEase(elapsed>55?elapsed-55:0,170);
        grip.x=filmLerp(leadHand.x,restGrip.x,down);
        grip.y=filmLerp(leadHand.y,restGrip.y,down);
        // Release only when the item actually touches the sill/ground.
        holding=down<248;
        thumb=holding;
    }else if((action==F_ACT_PASS||action==F_ACT_RECEIVE)&&paired&&
               beat->other!=F_WISP){
        FilmPoint otherHand=filmHandPoint(beat->other,otherPose,b.otherX,
                           b.otherFeet,b.otherHeight,flipOther);
        int receive=action==F_ACT_RECEIVE;
        FilmPoint giver=receive?otherHand:leadHand;
        FilmPoint taker=receive?leadHand:otherHand;
        int fromActor=receive?beat->other:beat->lead;
        int toActor=receive?beat->lead:beat->other;
        int handoff=filmEase(elapsed>60?elapsed-60:0,160);
        grip.x=filmLerp(giver.x,taker.x,handoff);
        grip.y=filmLerp(giver.y,taker.y,handoff);
        if(handoff>190){holding=0;thumb=0;}
        FilmPoint center=filmPropCenterForGrip(prop,size,grip);
        filmArmReach(fromActor,giver,grip);
        if(handoff>90)filmArmReach(toActor,taker,grip);
        filmDrawProp(prop,center.x,center.y,size);
        if(thumb)filmGripOverlay(fromActor,grip,prop);
        if(!holding)filmGripOverlay(toActor,grip,prop);
        return;
    }else if(action==F_ACT_WORK){
        // Controlled tool strike: the hammer remains in the hand.
        if(elapsed>70&&elapsed<250) grip.y+=((elapsed/42)&1)*2;
    }
    FilmPoint center=filmPropCenterForGrip(prop,size,grip);
    if(thumb&&(action==F_ACT_TAKE||action==F_ACT_PUT))
        filmArmReach(beat->lead,leadHand,grip);
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
    const FilmReel *feature=&filmFeatureReels[i];
    for(int j=0;j<feature->count;j++)
        if(beat==feature->beats+j) return j?feature->beats+j-1:extra->beats+extra->count-1;
    if(filmBonusIndex>=0&&filmBonusIndex<FILM_BONUS_N){
        const FilmReel *bonus=&filmBonusReels[filmBonusIndex];
        for(int j=0;j<bonus->count;j++)
            if(beat==bonus->beats+j)return j?bonus->beats+j-1:NULL;
    }
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
                filmSurface(F_PORCH,128,150);
                filmDrawProp(F_CUP,128,150,19);
            }
        }else if(beat->place==F_VILLAGE){
            // After the rain, puddles keep the prior weather visible.
            for(int i=0;i<5;i++)uiRect(37+i*40,152+i%2*3,13,1,fmvSlots[14]);
        }
    }else if(chapter==2&&beat->place==F_VILLAGE){
        // The unfinished roof stays in the SAME position across cuts.
        // A continuous seven-second construction sequence. Planks
        // appear as they are nailed in, remain placed when a new line
        // starts, and never vanish after the chapter's fifth shot.
        // The DS uses its existing colors and framebuffer only.
        int phase=elapsed/(7*FILM_SECOND);
        int complete=phase>=5,planks=complete?6:phase+1;
        if(planks<0)planks=0;
        uiRect(49,117,2,31,fmvSlots[24]);
        uiRect(105,117,2,31,fmvSlots[24]);
        uiRect(48,117,60,3,fmvSlots[25]);
        for(int i=0;i<planks;i++)
            uiRect(49+i*10,110+i%2,10,4,fmvSlots[24]);
        if(complete){
            uiRect(47,115,64,3,fmvSlots[25]);
            // The hammer stays at the worksite after construction, but
            // not simultaneously in a speaking actor's gripped hand.
            if(beat->prop!=F_HAMMER)
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

#ifndef __3DS__
// DS/DSi: purposeful scenery motion that uses the *existing* indexed film
// page and existing native-detail character cels. No extra bitmap, RAM,
// dynamic allocations, or expensive per-pixel transforms.
//
// Avoid "cutting everything": environment time is the complete chapter
// clock; the same window, forge, street or archive remains recognizable.
static void filmDSSetAction(const FilmBeat *beat,int t,int chapter){
    switch(beat->place){
    case F_PORCH:
        if(chapter==1){
            // The two cloth curtains sway within the established window;
            // their motion continues when Mira and Wisp exchange lines.
            int sway=(t/24)%5-2;
            uiRect(208+sway,63,3,22,fmvSlots[16]);
            uiRect(243-sway,63,3,22,fmvSlots[16]);
            uiRect(207+sway,85,6,2,fmvSlots[17]);
            uiRect(240-sway,85,6,2,fmvSlots[17]);
        }
        break;
    case F_VILLAGE:
        if(chapter>=3){
            // A grounded village worker walks past the same persistent
            // lane, not an animated primitive, ghost or floating prop.
            // Keep named Thorgrim unique when he is in the foreground.
            if(beat->lead!=F_THORGRIM&&beat->other!=F_THORGRIM){
                int phase=(t/15)%90;
                int travel=phase<45?phase:90-phase;
                int x=121+travel/2;
                int pose=(t/15)&1?1:2;
                filmDrawActor(F_THORGRIM,pose,x,157,24,phase>=45);
            }
            // A pair of fixed-height windows gently change light as the
            // village grows rather than twinkling randomly in empty air.
            int bright=(t/78)&1;
            uiRect(67,119,4,5,fmvSlots[bright?10:9]);
            uiRect(177,116,4,5,fmvSlots[bright?10:9]);
        }
        break;
    case F_FORGE: {
        // The smith's bellows physically compress, then recover beside
        // a fixed glowing fire. Re-use 2D palette colors and rectangles.
        int compression=(t/20)%7;
        uiRect(196,139,24,5,fmvSlots[24]);
        uiRect(200+compression,132,17-compression,8,fmvSlots[25]);
        uiRect(202+compression,135,11-compression,3,fmvSlots[20]);
        break;
    }
    case F_ARCHIVE: {
        // On the desk, a bound page moves through a single turn.
        // Repeated shots revisit this exact desk and remain legible.
        int page=(t/42)%6;
        uiRect(110,150,34,3,fmvSlots[24]);
        uiRect(112,144,17,6,fmvSlots[23]);
        uiRect(129,144-page,12,6+page,fmvSlots[22]);
        uiRect(127,142,2,9,fmvSlots[25]);
        break;
    }
    case F_FORD:
    case F_SHORE:
        // Local, shallow ripples; stable banks and no scrolling overlay.
        for(int i=0;i<3;i++){
            int travel=(t/14+i*3)%8;
            uiRect(65+i*56+travel,150+i%2*3,8,1,fmvSlots[i&1?12:14]);
        }
        break;
    case F_HEARTH:
        // The fire flickers inside its fixed hearth aperture, not all
        // over the picture or independently of the ambient flame.
        uiRect(187,135+(t/22)%3,6,7-(t/22)%3,fmvSlots[9]);
        uiRect(190,136,2,4,fmvSlots[10]);
        break;
    default:
        break;
    }
}
#endif

#ifdef __3DS__
// Physically anchored shadows, reflections and practical light at 20 fps.
// Never zoom a plate, create floating effects, or allocate a second surface.
static void filmStageLight(const FilmBeat *beat,int t){
    int p=beat->place, pulse=(t/21)&1;
    if(p==F_FORGE||p==F_HEARTH){
        int x=p==F_FORGE?192:204,y=p==F_FORGE?140:137;
        uiRect(x-6,y+3,16,1,fmvSlots[pulse?8:9]);
        uiRect(x-3,y-4+(t/18&1),5,2,fmvSlots[9]);
        uiPx(x,y-6,fmvSlots[10]);
    }else if(p==F_SHORE||p==F_FORD||p==F_VILLAGE){
        for(int i=0;i<5;i++){
            int x=24+i*47,y=156+i%3*2;
            int shift=((t/15+i)&1);
            uiRect(x+shift,y,9,1,fmvSlots[i%2?13:14]);
        }
    }else if(p==F_PORCH){
        // Window highlights are fixed to the wall. Rain falls *outside*.
        for(int i=0;i<6;i++){
            int x=216+i*4, y=60+((i*11+t/4)%30);
            uiRect(x,y,1,3,fmvSlots[22]);
        }
    }else if(p==F_GATE||p==F_RUINS){
        for(int i=0;i<3;i++){
            int x=17+i*67,y=146+i%2*5;
            uiRect(x,y,4+(pulse&1),1,fmvSlots[21]);
        }
    }
}
static void filmCastContact(int who,int x,int feet,int height){
    if(who>=F_ACTOR_N)return;
    if(who==F_WISP){
        // The spark is genuinely hovering; its light glances off the floor.
        uiRect(x-5,155,10,1,fmvSlots[13]);
        return;
    }
    uiRect(x-height/3,feet+1,height*2/3,2,fmvSlots[20]);
    uiRect(x-height/5,feet+3,height*2/5,1,fmvSlots[21]);
}
static int filmActingPose(const FilmBeat *beat,int elapsed,int lead,int action){
    int paired=beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N;
    // Speaking length follows the actual line, not the entire shot. A
    // ten-word sentence should not trigger eight seconds of lip flapping.
    // Cache the subtitle duration per immutable FilmBeat: no per-frame
    // allocation or text measurement in the actor drawing loop.
    static const FilmBeat *lastLine;
    static int speechEnd;
    if(beat!=lastLine){
        lastLine=beat;
        int n=0;
        if(beat->line)while(beat->line[n]&&n<41)n++;
        speechEnd=108+35+n*5;
        int latest=beat->seconds*FILM_SECOND-75;
        if(speechEnd>latest)speechEnd=latest;
    }
    int spoken=beat->line&&elapsed>=108&&elapsed<speechEnd;
    if(!lead){
        // The second performer *listens*. A held look and brief nod give
        // dialogue a reaction shot rather than synchronized mouth flapping.
        if((action==F_ACT_PASS||action==F_ACT_RECEIVE)&&paired&&
           elapsed>=75&&elapsed<=290)return elapsed<165?9:3;
        if(elapsed>=280&&elapsed<320&&beat->line)return 10;
        return (elapsed+(beat->other*13))%167<10?6:0;
    }
    if(((beat->camera==F_TRACK&&elapsed<140)||
        (beat->camera==F_WIDE&&beat->prop>=F_PROP_N&&elapsed<180))&&
       action!=F_ACT_WORK)
        return (elapsed/18)&1?1:2;
    if(action==F_ACT_TAKE)
        return elapsed<45?9:elapsed<145?3:elapsed<280?8:0;
    if(action==F_ACT_PUT)
        return elapsed<55?8:elapsed<265?3:elapsed<310?9:0;
    if(action==F_ACT_PASS||action==F_ACT_RECEIVE)
        return elapsed<65?9:elapsed<280?3:elapsed<328?7:0;
    if(action==F_ACT_WORK)
        return elapsed<60?9:elapsed<255?((elapsed/42)&1?11:3):elapsed<298?8:0;
    if(action==F_ACT_APPROACH)
        return elapsed<95?2:elapsed<205?3:elapsed<260?7:0;
    if(spoken){
        // Two authored mouth cels, plus a timed small hand gesture. No
        // constant twitch or continued talking during reaction silence.
        if(elapsed>=204&&elapsed<225)return 7;
        return (elapsed/12)&1?4:5;
    }
    // Directed silent consequences for the additional 3DS ensemble. Serin
    // writes rather than gesturing with a floating quill; Nessa braces
    // before she demonstrates how to cross the current; Hollow's final
    // admission lands as a visible, held bow toward his witness.
    if(beat->lead==F_SERIN&&beat->prop==F_QUILL&&
       elapsed>=speechEnd&&elapsed<beat->seconds*FILM_SECOND-42)
        return (elapsed/30)&1?11:3;
    if(beat->lead==F_NESSA&&beat->place==F_FORD&&
       elapsed>=speechEnd&&elapsed<beat->seconds*FILM_SECOND-45)
        return 9;
    if(beat->lead==F_HOLLOW&&beat->other==F_SERIN&&
       elapsed>=speechEnd&&elapsed<beat->seconds*FILM_SECOND-20)
        return 10;
    if(beat->line&&elapsed>=speechEnd&&elapsed<speechEnd+30)
        return 6; // listen for the response before resuming a neutral hold
    return (elapsed+beat->lead*17)%193<9?6:0;
}
#endif

#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
// Default 3DS film: true world-space geometry and projected actor routes.
// A diagnostic -DGG_FILM_2D_FALLBACK build retains the previously working
// 3DS art/camera path if a device-specific frame budget regression appears.
// DS/DSi always use the shared low-cost illustrated film director.
#include "story_world3d.h"
#endif

// Sparse effects are synced to on-screen hand contact, not to unrelated
// cuts. Use times divisible by both 3 (3DS) and 5 (DS) frame ticks.
static int filmActionCue(const FilmBeat *beat,int elapsed){
    if(elapsed==0) return beat->sound?beat->sound:-1;
    int action=filmActionOf(beat);
    if(action==F_ACT_TAKE&&elapsed==195)
        return beat->prop==F_CUP?FX_INSPECT:-1;
    if(action==F_ACT_PUT&&elapsed==225)
        return FX_SETTLE;
    if(action==F_ACT_WORK&&elapsed==180)
        return FX_STEP;
    if((action==F_ACT_PASS||action==F_ACT_RECEIVE)&&elapsed==225){
        // A hammer should sound like a small clank, never like eating.
        // Bread/crumbs get a soft feed rustle; the cup makes a clink;
        // quill/spoon touches are lighter than metalwork.
        if(beat->prop==F_HAMMER)return FX_STEP;
        if(beat->prop==F_BREAD||beat->prop==F_CRUMB)return FX_FEED;
        if(beat->prop==F_QUILL||beat->prop==F_SPOON)return FX_PAINT;
        if(beat->prop==F_LAMP||beat->prop==F_CROWN)return FX_GIFT;
        return FX_INSPECT;
    }
    return -1;
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
#ifdef __3DS__
        filmBars(filmBonusIndex>=0&&filmBonusIndex<FILM_BONUS_N?
                 filmBonusTitles[filmBonusIndex]:clip->title);
#else
        filmBars(clip->title);
#endif
        const FilmBeat *previous=filmPreviousBeat(clip,beat);
        establishing=previous&&previous->place!=beat->place;
    }
    int total=beat->seconds*FILM_SECOND;
    int move=filmEase(elapsed,total);
    FilmBlocking block=filmBlocking(beat,elapsed);
#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
    ggFilmWorldBlocking(beat,elapsed,move,&block);
#endif
    int captionOn=beat->line&&elapsed>=100&&elapsed<total-20;
    int position=beat->lead<F_ACTOR_N?
                 32+block.leadX*64/256:64;
    gmFilmMixSet(1,captionOn,position);
    int cue=filmActionCue(beat,elapsed);
    if(cue>=0)fxReq(cue);
    filmDrawSet(beat,move);
#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
    // The geometry is projected through the same dollying camera as actors.
    // Draw before cast so roofs, water and room floors form a stable world.
    int chapter=(int)(clip-fmvClips),roofFinished=1;
    if(chapter==2){
        // Only the early construction takes show exposed rafters.
        // Later angles, extras and subsequent films retain the new roof.
        for(int shot=0;shot<5;shot++)
            if(beat==roofBeats+shot)roofFinished=0;
    }
    ggFilmWorldDraw(beat,elapsed,move,
                    filmBonusIndex>=0?14:chapter,roofFinished,
                    filmBonusIndex==0||filmBonusIndex==1);
#endif
#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
    // World geometry now owns the rain window, roof construction and
    // ground. Keep only the intentionally grounded first seed/sprout from
    // the 2D continuity pass; do not paint floating structural overlays
    // on top of the coherent 3D set.
    if(filmBonusIndex<0&&(beat->place==F_SHORE||beat->place==F_ORBIT))
        filmWorldContinuity(clip,beat,elapsed);
#elif defined(__3DS__)
    if(filmBonusIndex<0)filmWorldContinuity(clip,beat,elapsed);
#else
    filmWorldContinuity(clip,beat,filmSceneryClock);
    filmDSSetAction(beat,filmSceneryClock,(int)(clip-fmvClips));
#endif
#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
    // Environment, weather, tree sway and background people continue
    // across edits. The spoken line, prop contact and actor poses remain
    // local to the current shot, preserving exact timing.
    filmAtmosphere(beat,ggFilmSceneTime(elapsed));
    filmStageLight(beat,ggFilmSceneTime(elapsed));
#elif defined(__3DS__)
    // Low-cost diagnostic path cannot call the world-stage clock.
    filmAtmosphere(beat,elapsed);
    filmStageLight(beat,elapsed);
#else
    // DS and DSi get independently moving, stage-anchored details too.
    filmDSLivingSets(beat,filmSceneryClock,(int)(clip-fmvClips));
    filmAtmosphere(beat,filmSceneryClock);
#endif

    int paired=beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N;
    int action=filmActionOf(beat);
    int leadPose=0,otherPose=0;
    if(beat->camera==F_TRACK&&elapsed<140&&action!=F_ACT_WORK)
        leadPose=1+(elapsed/18&1);
    else if((beat->prop<F_PROP_N&&(action==F_ACT_TAKE||action==F_ACT_PUT||
             action==F_ACT_WORK||action==F_ACT_PASS||action==F_ACT_RECEIVE))||
             beat->camera==F_HAND){
        if(elapsed>30&&elapsed<310)leadPose=3;
        if((action==F_ACT_PASS||action==F_ACT_RECEIVE)&&paired&&
           elapsed>55&&elapsed<310)otherPose=3;
    }else if(beat->line&&elapsed>=108&&elapsed<
#ifdef __3DS__
              220
#else
              filmDSSpeechEnd(beat)
#endif
             ){
#ifdef __3DS__
        leadPose=3;
#else
        // Two native 28x34 speech keys alternate; no fake reaching while
        // a character speaks. A listener may blink/react after the line.
        leadPose=(elapsed/12)&1?4:0;
#endif
    }
#ifndef __3DS__
    if(paired&&beat->line&&elapsed>=filmDSSpeechEnd(beat)+12&&
       elapsed<filmDSSpeechEnd(beat)+65)
        otherPose=5;
#endif
#ifdef __3DS__
    leadPose=filmActingPose(beat,elapsed,1,action);
    otherPose=filmActingPose(beat,elapsed,0,action);
#endif

    int showCast=!(establishing&&elapsed<39);
    if(beat->camera!=F_INSERT&&showCast){
#ifdef __3DS__
        filmCastContact(beat->other,block.otherX,block.otherFeet,block.otherHeight);
        filmCastContact(beat->lead,block.leadX,block.leadFeet,block.leadHeight);
#endif
        filmDrawActor(beat->other,otherPose,block.otherX,block.otherFeet,
                      block.otherHeight,paired&&block.otherX>block.leadX);
        filmDrawActor(beat->lead,leadPose,block.leadX,block.leadFeet,
                      block.leadHeight,paired&&block.leadX>block.otherX);
    }
    if(showCast||beat->camera==F_INSERT)
        filmDrawPropAction(beat,elapsed,leadPose,otherPose,block);
#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
    // True near-camera geometry is composited last, so the actors and
    // gripped props are partially hidden when they move behind an obstacle.
    ggFilmWorldForeground(beat,move,filmBonusIndex>=0?14:(int)(clip-fmvClips));
#endif
#ifndef __3DS__
    filmDSForeground(beat);
#endif
    filmCaption(beat,captionOn);
}
static int fmvRenderAt(const FmvClip *clip,int elapsed){
    int baked=clip->frames*FMV_TICKS;
#ifdef __3DS__
    if(filmBonusIndex>=0&&filmBonusIndex<FILM_BONUS_N)baked=0;
#endif
    if(elapsed<baked){
        if(elapsed%FMV_TICKS) return 0;
        fmvDecode(clip,elapsed/FMV_TICKS);
        return 1;
    }
    elapsed-=baked;
    filmSceneryClock=elapsed;
#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
    ggFilmWorldClock=elapsed;
#endif
    const FilmBeat *beat=filmBeatAt(clip,&elapsed);
    if(!beat||elapsed%FILM_STAGE_TICKS) return 0;
    filmDrawBeat(clip,beat,elapsed);
    return 1;
}
