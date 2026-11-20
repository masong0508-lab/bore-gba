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

static FilmFraming filmFraming(const FilmBeat *beat,int move){
    static const u8 widths[]={128,108,118,114,105};
    int w=widths[beat->camera];
#ifdef __3DS__
    // A second, slower push on the larger display. Keep actors in the
    // composed frame while the set shifts behind them.
    if(beat->camera!=F_WIDE) w-=6+move/64;
#endif
    int h=w*FMV_H/FMV_W, sx=FMV_W-w, sy=FMV_H-h;
    int pan=(beat->camera==F_TRACK)?move:((beat->lead==F_OFFSCREEN)?128:96+move/4);
    FilmFraming frame={sx*pan*FILM_PLATE_SCALE,sy*(beat->camera==F_INSERT?168:94)*FILM_PLATE_SCALE,
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
    int width=height*sw/sh, left=center-width/2, top=feet-height;
    int dx=sw*256/width, dy=sh*256/height;
    for(int y=0,sy=0;y<height;y++,sy+=dy){
        int dstY=top+y;
        if(dstY<FILM_TOP||dstY>=FILM_BOTTOM) continue;
        u8 *out=uiBuf+dstY*256;
        const u8 *source=image+(sy>>8)*sw;
        for(int x=0,sx=0;x<width;x++,sx+=dx){
            int target=left+x, column=sx>>8;
            if((unsigned)target>=256) continue;
            int color=source[flip?sw-1-column:column];
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
static void filmAtmosphere(const FilmBeat *beat,int t){
    const int storm=beat->place==F_STORM, water=beat->place==F_FORD||beat->place==F_SHORE;
#ifdef __3DS__
    const int count=24;
#else
    const int count=9;
#endif
    for(int i=0;i<count;i++){
        int speed=2+i%3, x=(i*71+t*speed/3)&255;
        int y=storm?(i*97+t*speed)%139:13+(i*29+t/3)%81;
        int color=fmvSlots[storm?31:i%4?21:23];
        if(water){
            y=110+(i*7+t/5)%22;
            if(x<76||x>181) continue;
            color=fmvSlots[i%3?13:14];
        }
        if(storm) uiRect(x,FILM_TOP+y,1,3,color);
        else if(water) uiRect(x,FILM_TOP+y,3+i%3,1,color);
        else uiPx(x,FILM_TOP+y,color);
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
    int move=filmEase(elapsed,total), t=elapsed/3;
    filmDrawSet(beat,move);
    filmAtmosphere(beat,t);

    int paired=beat->other<F_ACTOR_N, near=beat->camera==F_NEAR;
    int left=paired?86:126, right=176;
    int height=near?118:82, feet=near?164:157;
    int pose=0, otherPose=0;
    if(beat->camera==F_WIDE) height=64;
    if(beat->camera==F_TRACK){
        left-=36-(move*66)/256;
        pose=elapsed<total-34?1+(elapsed/15&1):0;
        otherPose=paired?pose:0;
    } else if(beat->camera==F_HAND&&elapsed>60){
        pose=3;
        otherPose=3;
    } else if(beat->line&&elapsed>=110&&elapsed<total-45){
        // Small gestures are separated by pauses for the other person to react.
        pose=(elapsed/45&1)?3:0;
    }
    if(beat->camera!=F_INSERT){
        int bob=filmPulse(t/2,4);
        filmDrawActor(beat->other,otherPose,right-(beat->camera==F_TRACK?20-move/8:0),
                      feet+(beat->other==F_WISP?-22+bob:0),height,1);
        filmDrawActor(beat->lead,pose,left,feet+(beat->lead==F_WISP?-23+bob:0),height,0);
    }

    int px=128, py=142, size=beat->camera==F_INSERT?72:35;
    if(beat->camera==F_INSERT){
        // Push into the object as it settles into the shot.
        size+=move/17;
        px+=12-move/10;
        py-=filmPulse(t/2,4);
    } else if(beat->camera==F_HAND){
        int pass=filmEase(elapsed>60?elapsed-60:0,140);
        px=left+20+(paired?(right-left-40)*pass/256:0);
        py=126-filmPulse(pass/8,16)/2;
    } else if(beat->prop==F_GOAT){
        px=28+move*200/256;
        py=(beat->place==F_VILLAGE?124:151)-filmPulse(t,4);
    } else if(beat->prop==F_LAMP){
        px=paired?129:left+27;
        py-=filmPulse(t/3,5);
    }
    if(beat->camera==F_INSERT){
        int settle=filmEase(elapsed,total/2);
        if(beat->prop==F_CRUMB){
            px=82+settle/3;
            py=124+settle/6;
            size=35-settle/11;
        } else if(beat->prop==F_CROWN) py=104+settle/9;
        else if(beat->prop==F_CUP){
            int drop=t%27;
            if(drop<18) uiRect(px-3,FILM_TOP+18+drop*2,2,4,fmvSlots[23]);
        }
    }
    if(beat->prop==F_LAMP||beat->prop==F_SEED){
        int glow=filmPulse(t/2,6);
        for(int i=0;i<6;i++) uiPx(px-9+(i*13+t/2)%19,py-38-glow-(i*7+t)%15,fmvSlots[8+i%3]);
    }
    filmDrawProp(beat->prop,px,py,size);
    // The first seconds give the action or the other actor's reaction room.
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
