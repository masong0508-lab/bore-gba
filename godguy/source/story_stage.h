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
    int left=85,right=171;
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
static void filmDrawBeat(const FmvClip *clip,const FilmBeat *beat,int elapsed){
    if(!elapsed){
        // uiNew is free once the baked prologue has reached its final frame.
#ifdef __3DS__
        ggUnpack(filmWides[beat->place],uiNew,FMV_W*FMV_H*4);
#else
        ggUnpack(filmSets[beat->place],uiNew,FMV_W*FMV_H);
#endif
        if(storyScore>=0) filmScoreTurn(beat);
        if(beat->sound) fxReq(beat->sound);
        filmBars(clip->title);
    }
    int total=beat->seconds*FILM_SECOND;
    int move=filmEase(elapsed,total);
    FilmBlocking block=filmBlocking(beat,elapsed);
    filmDrawSet(beat,move);
    filmAtmosphere(beat,elapsed);

    int paired=beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N;
    int leadPose=0,otherPose=0;
    if(beat->camera==F_TRACK&&elapsed<140){
        leadPose=1+(elapsed/18&1); // one purposeful entrance, then stop
    } else if(beat->camera==F_HAND){
        if(elapsed>=60&&elapsed<230) leadPose=3;
        if(paired&&elapsed>=170&&elapsed<290) otherPose=3;
    } else if(beat->line&&elapsed>=130&&elapsed<220){
        leadPose=3; // one deliberate gesture while speaking
    }
    if(beat->camera!=F_INSERT){
        filmDrawActor(beat->other,otherPose,block.otherX,block.otherFeet,
                      block.otherHeight,paired&&block.otherX>block.leadX);
        filmDrawActor(beat->lead,leadPose,block.leadX,block.leadFeet,
                      block.leadHeight,paired&&block.leadX>block.otherX);
    }

    if(beat->prop<F_PROP_N){
        int px=128,py=146,size=22;
        if(beat->camera==F_INSERT){
            // Insert shots use a fixed-scale prop on an actual surface:
            // a seed meets soil, a cup rests on a wooden sill, a hammer
            // strikes a bench. Never zoom a 20-pixel prop into 50 pixels.
            size=28;
            py=148;
            int surface=148;
            int surfaceColor=beat->prop==F_SEED||beat->prop==F_LEAF?
                fmvSlots[17]:beat->place==F_FORGE?fmvSlots[21]:fmvSlots[25];
            uiRect(73,surface,111,3,surfaceColor);
            uiRect(79,surface+3,101,1,fmvSlots[20]);
            if(beat->prop==F_SEED){
                py-=6-filmEase(elapsed,75)*6/256; // settle into soil once
            } else if(beat->prop==F_HAMMER){
                py-=((elapsed/45)&1)?2:0;
            } else if(beat->prop==F_CRUMB){
                size=23;
                py-=4-filmEase(elapsed,75)*4/256;
            } else if(beat->prop==F_CROWN){
                py=146-filmEase(elapsed,100)*3/256;
            }
            if(beat->prop==F_CUP&&beat->place==F_PORCH){
                int drop=FILM_TOP+18+(elapsed/3)%110;
                if(drop<py-12) uiRect(px,drop,1,2,fmvSlots[23]);
            }
        } else if(beat->prop==F_GOAT){
            // The goat belongs on the floor, with a short walk across it.
            px=55+move*64/256;
            py=157;
            size=24;
        } else if(beat->camera==F_HAND&&beat->lead<F_ACTOR_N){
            int pass=filmEase(elapsed>70?elapsed-70:0,180);
            px=block.leadX+(paired?(block.otherX-block.leadX)*pass/256:16);
            // Match the hand height of a hovering Wisp and a standing adult.
            py=block.leadFeet-(beat->lead==F_WISP?-1:32);
            size=21;
        } else if(beat->lead<F_ACTOR_N){
            // The object remains with its owner, beside their hand.
            int side=paired&&block.leadX>block.otherX?-1:1;
            px=block.leadX+side*(beat->lead==F_WISP?15:16);
            py=block.leadFeet-(beat->lead==F_WISP?-1:32);
            size=beat->camera==F_WIDE?17:22;
        } else {
            // Establishing shots use a prop in the scene, not at screen centre.
            if(beat->prop==F_LAMP){ px=204;py=138; }
            else if(beat->prop==F_CROWN){ px=131;py=114; }
            else if(beat->prop==F_MASK){ px=154;py=142; }
            else if(beat->prop==F_HAMMER){ px=145;py=143; }
            else if(beat->prop==F_CUP){ px=130;py=146; }
            else if(beat->prop==F_SEED){ px=116;py=148; }
            else { px=154;py=145; }
        }
        if(beat->prop==F_LAMP){
            // Two anchored warm pixels read as a lantern rather than
            // particles flying above the cast.
            uiPx(px-2,py-size+2,fmvSlots[10]);
            uiPx(px+2,py-size+2,fmvSlots[9]);
        }
        filmDrawProp(beat->prop,px,py,size);
    }
    // Silence/action first, dialogue afterwards. The caption ends before
    // the cut so the next speaker gets a clean visual handoff.
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
