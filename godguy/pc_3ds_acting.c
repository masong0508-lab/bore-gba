// SDK-free cinematic actor regression: speaking ownership, hand-anchored
// archive ledger and supply parcel, continuous poses, bounded draw work.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8;
enum {GG_SAGA_BEAT_TICKS=96};
static char ggOriginHumanCel[24][17];
static int draws,bookCover,parcelBody,handLinks;
static int ggExpClamp(int n,int a,int b){return n<a?a:n>b?b:n;}
static void ggExpRect(u8 *out,int x,int y,int w,int h,int r,int g,int b){
    (void)out;(void)x;(void)y;
    if(w<=0||h<=0){fputs("ACTOR FAIL: empty cel\n",stderr);exit(1);}
    draws++;
    if(r==72&&g==54&&b==53)bookCover++;
    if(r==151&&g==108&&b==68)parcelBody++;
    if(r==204&&g==167&&b==132)handLinks++;
}
#include "3ds/film_acting3d.h"
static void check(int ok,const char *why){
    if(!ok){fprintf(stderr,"3DS ACTING FAIL: %s\n",why);exit(1);}
}
int main(void){
    for(int y=0;y<24;y++)memset(ggOriginHumanCel[y],'.',16);
    const u8 hair[3]={90,60,50},skin[3]={204,167,132},coat[3]={58,90,110};
    u8 buffer[1]={0};
    GgFilmPose book=ggFilmPoseFor("The records belong to the people.",
                               2,16,0,1,0,2);
    check(book.pose==GG_FILM_PRESENT&&book.holdProp==3,
          "archive speaker failed to hold a recognisable ledger");
    check(ggFilmPoseFor("The records belong to the people.",2,16,0,0,0,2).holdProp==0,
          "listener is holding a phantom duplicate");
    check(ggFilmPoseFor("The records belong to the people.",2,16,0,1,1,2).holdProp==0,
          "walker teleported a prop into their hands");
    ggFilmDrawActor(buffer,100,140,2,hair,skin,coat,&book);
    check(bookCover==1&&parcelBody==0&&handLinks==1,
          "ledger drawn without a connected matching hand");
    check(draws<=20,"prop added an excessive per-frame rectangle cost");
    draws=bookCover=parcelBody=handLinks=0;
    GgFilmPose parcel=ggFilmPoseFor("The supplies are for the families.",
                                  1,48,1,1,0,3);
    check(parcel.pose==GG_FILM_PRESENT&&parcel.holdProp==4,
          "supply speaker failed to hold a matching packet");
    ggFilmDrawActor(buffer,100,140,2,hair,skin,coat,&parcel);
    check(bookCover==0&&parcelBody==1&&handLinks==1,
          "provisions rendered as a floating generic book");
    check(draws<=20,"provisions prop drawing exceeded bounded detail");
    GgFilmPose silent=ggFilmPoseFor("The records belong to the people.",
                                  2,48,0,0,0,2);
    check(!silent.mouth&&!silent.holdProp,
          "silent partner is lip-syncing or duplicating the prop");
    puts("3DS ACTING OK: speaker-owned book and parcel, hand contact, no floating duplicate, cheap draw");
    return 0;
}
