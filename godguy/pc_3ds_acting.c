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
    check(ggFilmGuestPalette[0][0][0]==126,
          "actor colour identity is not stable");
    GgFilmPose compatibility=ggFilmPoseAt("YOU","The records are open.",
                                         2,48,1,0,2);
    check(compatibility.holdProp==3,
          "older portrait-pose caller lost its acting semantics");
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
    GgFilmPose initial={0};
    ggFilmPairActPose(&initial,1,1);
    check(initial.armR==0&&initial.reach==1,
          "shoulder jumped up before the character actually reached");
    GgFilmPose midway={0};
    ggFilmPairActPose(&midway,1,24);
    check(midway.armR==1&&midway.reach==1,
          "shoulder did not ease through an intermediate gesture");
    GgFilmPose reach=ggFilmPoseFor("We'll help the families.",
                                 2,64,1,0,0,3);
    ggFilmPairActPose(&reach,1,96);
    check(reach.reach==4&&reach.armR>=2&&reach.legL==2&&
          reach.legR==-2&&reach.bob==0,
          "hero lacks planted legs, weight shift or outstretched arm");
    int handX,handY;
    ggFilmPairHandAt(200,140,3,218,&reach,&handX,&handY);
    check(handX==209&&handY<140&&handY>80,
          "offered hand went past the other actor or detached vertically");
    GgFilmPose reply=ggFilmPoseFor("I will help.",2,64,0,0,0,3);
    ggFilmPairActPose(&reply,-1,96);
    int otherX,otherY;
    ggFilmPairHandAt(218,140,2,200,&reply,&otherX,&otherY);
    check(reply.reach==-4&&reply.armL>=2&&otherX==handX,
          "guest did not reciprocate the same physical hand contact");
    check(otherY>80&&otherY<140,
          "guest hand detached from projected torso");
    // The offered hand must begin on the original sleeve, not
    // teleport to the other character at the start of an interaction.
    GgFilmPose idleReach={0};
    int restX,restY,fullX,fullY;
    ggFilmPairHandAt(100,140,3,180,&idleReach,&restX,&restY);
    ggFilmPairActPose(&idleReach,1,96);
    ggFilmPairHandAt(100,140,3,180,&idleReach,&fullX,&fullY);
    check(restX==121&&fullX==136&&restX<fullX&&fullX<140,
          "hand floated at the destination before the arm extended");
    check(ggFilmPairProjectedContact(120,140,3,172,140,3),
          "valid two-person screen-space grasp was rejected");
    check(!ggFilmPairProjectedContact(120,140,3,250,140,3),
          "distant projected people attempted an impossible reach");
    check(!ggFilmPairProjectedContact(120,140,3,172,181,3),
          "cliff-height misalignment triggered a floating handoff");
    check(!ggFilmPairProjectedContact(120,140,3,130,140,3),
          "superimposed characters triggered an unreadable embrace");

    reach.holdProp=0;reply.holdProp=0;
    draws=bookCover=parcelBody=handLinks=0;
    ggFilmDrawActor(buffer,200,140,3,hair,skin,coat,&reach);
    ggFilmDrawActor(buffer,218,140,2,hair,skin,coat,&reply);
    ggFilmDrawHeldObject(buffer,handX,handY,2,3,skin);
    check(bookCover==1&&parcelBody==0&&handLinks==1,
          "joint interaction duplicated or disconnected shared ledger");
    check(draws<40,"full-body joint performance added too many rect draws");
    puts("3DS ACTING OK: speaker-owned book and parcel, hand contact, no floating duplicate, cheap draw");
    return 0;
}
