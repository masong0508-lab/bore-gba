// GodGuy 3DS cinematic partner regression: deterministic cues,
// contact timing, same-tile approach, and persistent one-object ownership.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "3ds/film_partner3d.h"
static void check(int ok,const char *what){
    if(!ok){fprintf(stderr,"3DS PARTNER FAIL: %s\n",what);exit(1);}
}
int main(void){
    check(ggFilmPairCue("The names remain where ordinary people can read.",
        "Their children can trace the records.")==GG_FILM_PAIR_BOOK,
        "public records did not produce a ledger handoff");
    check(ggFilmPairCue("The children have crossed the ford.",
        "The wagons will feed more families tonight.")==GG_FILM_PAIR_PARCEL,
        "relief wagons did not produce a provision handoff");
    check(ggFilmPairCue("I have orders to seize supplies.",
        "The army will remember.")==GG_FILM_PAIR_NONE,
        "coercive seizure wrongly became a friendly gift");
    check(ggFilmPairCue("The secret pages belong to you alone now.",
        "I will hide the books.")==GG_FILM_PAIR_NONE,
        "private forbidden records were handed out as a public gift");
    check(ggFilmPairCue("We must seize provisions.",
        "The families are frightened.")==GG_FILM_PAIR_NONE,
        "coercive script accidentally played a friendly clasp");
    check(ggFilmPairCue("The wounded need our help.",
        "We will find a way home.")==GG_FILM_PAIR_COMFORT,
        "reassuring scene had no sympathetic physical response");
    check(ggFilmPairCue("The road is long.", "The wind is cold.")==
          GG_FILM_PAIR_NONE,"random narrative dialogue spawned contact");
    int peak=0,wasRising=1,prior=0;
    const int sx=32*256+128,sy=29*256+128;
    const int gx=sx+256,gy=sy;
    for(int t=0;t<=192;t++){
        int beat=t>=96?1:0,local=t>=96?t-96:t;
        int inset=ggFilmPairInset(beat,local,96);
        int hx,hy,wx,wy;
        ggFilmPairPosition(sx,sy,gx,gy,inset,1,&hx,&hy);
        ggFilmPairPosition(sx,sy,gx,gy,inset,0,&wx,&wy);
        check(inset>=0&&inset<=96,"inset exceeded physical stage limits");
        check((hx>>8)==32&&(wx>>8)==33,"partner left reserved world tiles");
        check(hy==sy&&wy==gy,"partner drifted out of floor lane");
        check(wx>hx,"participants overlapped or crossed their own floors");
        check(hx-sx==inset&&gx-wx==inset,"actors not moving toward each other");
        check(ggFilmPairInset(beat,local,96)==inset,
              "animation not deterministic under pause/replay");
        if(inset==96)peak++;
        if(wasRising&&inset<prior)wasRising=0;
        if(!wasRising)check(inset<=prior,
                            "approach resumed forward after retreat started");
        prior=inset;
    }
    check(peak>=24&&ggFilmPairInset(0,48,96)==0&&
          ggFilmPairInset(1,0,96)==96&&
          ggFilmPairInset(1,96,96)==0,
          "staging does not hold smoothly across paired beats");
    check(ggFilmPairInset(0,-999,96)==0&&
          ggFilmPairInset(1,999,96)==0&&
          ggFilmPairInset(0,30,0)==0,
          "invalid timing escaped fixed insets");
    // Symmetric north/south approaches stay in each actor's own cell.
    int hx,hy,wx,wy;
    ggFilmPairPosition(sx,sy,sx,sy-256,96,1,&hx,&hy);
    ggFilmPairPosition(sx,sy,sx,sy-256,96,0,&wx,&wy);
    check((hy>>8)==29&&(wy>>8)==28&&hx==sx&&wx==sx,
          "north/south actors crossed floor-tile borders");
    check(ggFilmPairDirection(20,40)==1&&
          ggFilmPairDirection(40,20)==-1&&
          ggFilmPairDirection(20,20)==0,
          "hand reach did not orient toward partner");
    check(ggFilmPairTouch(200,80,212,91)&&
          !ggFilmPairTouch(200,80,240,91)&&
          !ggFilmPairTouch(200,80,210,103),
          "object transferred without actual hand proximity");
    int owner=ggFilmPairObjectOwner(GG_FILM_PAIR_BOOK,0,0);
    check(owner==0,"initial ledger has no source");
    owner=ggFilmPairObjectOwner(GG_FILM_PAIR_BOOK,1,0);
    check(owner==1,"visible fingertip contact did not pass the ledger");
    owner=ggFilmPairObjectOwner(GG_FILM_PAIR_BOOK,0,owner);
    check(owner==1,"receiver lost the ledger after stepping away");
    check(ggFilmPairObjectOwner(GG_FILM_PAIR_COMFORT,1,1)==-1&&
          ggFilmPairObjectOwner(GG_FILM_PAIR_NONE,1,1)==-1,
          "comfort gesture materialized an imaginary second prop");
    // A+NEXT skips frames; a paused or resumed reel may render only
    // the first and last ticks of either line. The book still changes
    // owners exactly once at the authored cut, independent of history.
    const int scrub[]={0,5,48,94,95,96,97,140,191,97,0,191};
    for(unsigned i=0;i<sizeof(scrub)/sizeof(scrub[0]);i++){
        int frame=scrub[i],beat=frame>=96?1:0,local=frame>=96?frame-96:frame;
        int expected=frame>=96;
        check(ggFilmPairOwnerAt(GG_FILM_PAIR_BOOK,beat,local,96)==expected,
              "jump, reverse seek or playback pause lost object ownership");
        check(ggFilmPairOwnerAt(GG_FILM_PAIR_PARCEL,beat,local,96)==expected,
              "provisions owner depends on rendered contact frames");
    }
    check(ggFilmPairOwnerAt(GG_FILM_PAIR_COMFORT,1,40,96)==-1&&
          ggFilmPairOwnerAt(GG_FILM_PAIR_NONE,1,40,96)==-1,
          "non-transfer narrative made an imaginary gift");
    check(ggFilmPairOwnerAt(GG_FILM_PAIR_BOOK,1,40,0)==0,
          "zero-duration dialogue must not synthesize a handover");
    puts("3DS PARTNER OK: story cues, grounded synchronized approach, contact, persistent single-object handover");
    return 0;
}
