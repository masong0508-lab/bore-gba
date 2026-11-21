// 3DS cinematic acting: fixed-point, deterministic keyed performances.
// No texture atlases, floating point, heap, temporary render targets or
// searches in the character renderer. Only two actors per film frame.
#ifndef GG_FILM_ACTING3D_H
#define GG_FILM_ACTING3D_H
enum { GG_FILM_LISTEN,GG_FILM_SPEAK,GG_FILM_POINT,GG_FILM_PRESENT,
       GG_FILM_REFLECT,GG_FILM_REASSURE,GG_FILM_WALK };
typedef struct {
    signed char pose,head,lean,armL,armR,legL,legR,bob;
    signed char gaze,blink,mouth,holdProp,reach,clothSway;
} GgFilmPose;
// One 6x3x3 palette, shared by all guest film cels. These identities
// are stable across chapters, alternate endings and nonlinear films.
static const u8 ggFilmGuestPalette[6][3][3]={
    {{126,65,46},{212,168,133},{68,132,111}},
    {{72,64,58},{200,159,129},{68,99,135}},
    {{103,78,64},{207,164,137},{143,58,66}},
    {{83,57,53},{201,154,120},{105,118,78}},
    {{211,191,146},{226,188,155},{110,91,143}},
    {{90,74,58},{204,167,136},{112,99,78}}
};
static int ggFilmGestureFor(const char*line,int beat){
    // Scripted semantic acting for every existing dialogue reel without
    // requiring 100s of copy-pasted cel frames in ROM.
    if(strstr(line,"records")||strstr(line,"names")||
       strstr(line,"pages")||strstr(line,"books")||
       strstr(line,"supplies")||strstr(line,"provisions")||
       strstr(line,"ledgers")||strstr(line,"archive"))
        return GG_FILM_PRESENT;
    if(strstr(line,"families")||strstr(line,"children")||
       strstr(line,"wounded")||strstr(line,"home")||
       strstr(line,"peace")||strstr(line,"living"))
        return GG_FILM_REASSURE;
    if(strstr(line,"army")||strstr(line,"orders")||
       strstr(line,"crown")||strstr(line,"throne")||
       strstr(line,"banners")||strstr(line,"king"))
        return GG_FILM_POINT;
    if(strstr(line,"remember")||strstr(line,"truth")||
       strstr(line,"burden")||strstr(line,"choice")||
       strstr(line,"fear"))
        return GG_FILM_REFLECT;
    return (beat%4==0)?GG_FILM_POINT:
           (beat%4==2)?GG_FILM_REASSURE:GG_FILM_SPEAK;
}
// Actor-specific speaking state comes from the directed cast slot,
// not merely from the line's speaker label. This is essential when two
// named characters share a conversation: only the actual speaker moves
// their mouth, while the other visibly listens and reacts.
static GgFilmPose ggFilmPoseFor(const char*line,int beat,int local,
                               int hero,int speaking,int walking,
                               int propKind){
    GgFilmPose p={0};
    int t=ggExpClamp(local,0,GG_SAGA_BEAT_TICKS-1);
    int strength=t<12?t*256/12:
                 t<70?256:
                 (GG_SAGA_BEAT_TICKS-t)*256/26;
    strength=ggExpClamp(strength,0,256);
    p.pose=(signed char)(walking?GG_FILM_WALK:
           speaking?ggFilmGestureFor(line,beat):GG_FILM_LISTEN);
    p.gaze=(signed char)(hero?1:-1);
    // Acting has anticipation, held emphasis, reaction and recovery;
    // no per-frame RNG or universal continuous bobbing.
    if(p.pose==GG_FILM_WALK){
        int phase=(t/6)&3;
        p.legL=(signed char)((phase==0?-2:phase==2?2:0));
        p.legR=(signed char)-p.legL;
        p.armL=(signed char)(phase==0?1:phase==2?0:1);
        p.armR=(signed char)(phase==2?1:phase==0?0:1);
        p.bob=(signed char)((phase==1||phase==3)?1:0);
        p.lean=1;
    }else if(speaking){
        if(p.pose==GG_FILM_POINT){
            p.armR=(signed char)(strength*4/256);
            p.head=(signed char)(strength*2/256);
            p.lean=(signed char)(strength/160);
        }else if(p.pose==GG_FILM_PRESENT){
            p.armR=(signed char)(strength*3/256);
            p.armL=(signed char)(strength*2/256);
            // Hold a real, distinguishable book or ration parcel through
            // the entire delivered line; never a generic hovering chip.
            // Values 3 and 4 encode records and crates (prop kinds 2/3).
            // The prop remains tied to the moving right hand.
            if(propKind==2||propKind==3)
                p.holdProp=(signed char)(propKind+1);
        }else if(p.pose==GG_FILM_REFLECT){
            p.head=(signed char)(-strength*2/256);
            p.lean=(signed char)(-strength/160);
            p.gaze=0;
        }else if(p.pose==GG_FILM_REASSURE){
            p.armL=(signed char)(strength*2/256);
            p.armR=(signed char)(strength*2/256);
            p.head=(signed char)(strength/170);
        }else{
            p.armR=(signed char)(strength*2/256);
            p.head=(signed char)(strength/180);
        }
        // Short syllabic mouth shapes, with real silent frames;
        // unlike the old mechanically-flapping two-frame character.
        if(t>7&&t<78){
            int syllable=(t/5+beat*3+(hero?1:3))%9;
            p.mouth=(signed char)(syllable==0||syllable>=7?0:
                                   syllable==2||syllable==5?2:1);
        }
    }else{
        // Speaker's partner responds to the line with a small look,
        // a held reaction and then a natural settle.
        p.head=(signed char)(t>23&&t<66?1:0);
        if((beat&3)==2&&t>35&&t<70)
            p.armL=1;
    }
    // Planted boots: speaking must never bounce the ENTIRE body off
    // the floor. Instead animate only hair/coat movement, head reaction,
    // syllables and arm gestures at staggered beats for the two actors.
    p.blink=(signed char)(((t+beat*17+(hero?13:39))%79)<3);
    if(!walking){
        int phase=(t+beat*13+(hero?5:22))%60;
        p.clothSway=(signed char)(phase<17?-1:phase<37?0:1);
        p.bob=0;
    }
    return p;
}
// Preserve the simple authored-pose API used in existing regression
// scenes; the director uses ggFilmPoseFor for multi-actor blocking.
static GgFilmPose ggFilmPoseAt(const char*voice,const char*line,
                              int beat,int local,int hero,int walking,
                              int propKind){
    int speaking=hero?!strcmp(voice,"YOU"):
        (strcmp(voice,"YOU")&&strcmp(voice,"NARRATOR"));
    return ggFilmPoseFor(line,beat,local,hero,speaking,walking,
                         propKind);
}
// Coordinated upper-body reach, counterbalancing steps and weight transfer.
// All deformations stay on the existing 16x24 cel; no added draw passes.
static void ggFilmPairActPose(GgFilmPose *p,int dir,int inset){
    if(!dir||inset<=0)return;
    int extent=(inset+23)/24;
    if(extent>4)extent=4;
    p->reach=(signed char)(dir*extent);
    // Raise the shoulder in stages; a one-pixel approach must not
    // snap a resting elbow straight to its maximum gesture.
    int lift=inset>=48?2:inset>=16?1:0;
    if(dir<0){if(p->armL<lift)p->armL=(signed char)lift;}
    else if(p->armR<lift)p->armR=(signed char)lift;
    p->lean=(signed char)(p->lean+dir*(inset/64));
    p->head=(signed char)(p->head+(inset>=64?1:0));
    p->legL=(signed char)(dir*(inset/48));
    p->legR=(signed char)-p->legL;
    p->bob=0; // planted feet while reaching, not endless idle bobbing
}
static void ggFilmPairHandAt(int x,int feet,int scale,int targetX,
                              const GgFilmPose *p,int *hx,int *hy){
    int s=ggExpClamp(scale,1,3);
    int dx=targetX-x,half=dx<0?-dx:dx;
    half/=2;
    // The hand STARTS on its sleeve, then extends as the existing cel
    // reaches. Previous code teleported it to the midpoint at t=0.
    int base=half<7*s?half:7*s;
    int full=half<12*s?half:12*s;
    int extent=p->reach<0?-p->reach:p->reach;
    if(extent>4)extent=4;
    int reach=base+(full-base)*extent/4;
    *hx=x+(dx<0?-reach:reach);
    *hy=feet-(13+((dx<0?p->armL:p->armR)>1?1:0))*s;
}
// Rehearse contact using the same sprite-space hands and poses used
// during actual playback. Called ONCE per paired shot, not every frame.
// Exclude near-superimposed bodies and impossible off-angle embraces.
static int ggFilmPairProjectedContact(int ax,int ay,int as,
                                       int bx,int by,int bs){
    int gap=ax-bx;if(gap<0)gap=-gap;
    int aScale=ggExpClamp(as,1,3),bScale=ggExpClamp(bs,1,3);
    if(gap<(aScale+bScale)*4)return 0;
    GgFilmPose a={0},b={0};
    ggFilmPairActPose(&a,bx>ax?1:-1,96);
    ggFilmPairActPose(&b,ax>bx?1:-1,96);
    int ahx,ahy,bhx,bhy;
    ggFilmPairHandAt(ax,ay,aScale,bx,&a,&ahx,&ahy);
    ggFilmPairHandAt(bx,by,bScale,ax,&b,&bhx,&bhy);
    int dx=ahx-bhx,dy=ahy-bhy;
    return dx>=-18&&dx<=18&&dy>=-14&&dy<=14;
}
static void ggFilmDrawHeldObject(u8*out,int handX,int handY,
                                  int s,int propCode,const u8*skin){
    if(propCode==3){
        // Distinct leather-bound public ledger, with visible page edges.
        ggExpRect(out,handX,handY-4*s,6*s,5*s,72,54,53);
        ggExpRect(out,handX+2*s,handY-4*s,4*s,s,226,209,172);
        ggExpRect(out,handX+2*s,handY,4*s,s,226,209,172);
        ggExpRect(out,handX+s,handY-3*s,s,3*s,194,158,91);
    }else if(propCode==4){
        // Bound provisions packet, reused during continuous handoff.
        ggExpRect(out,handX,handY-3*s,6*s,5*s,151,108,68);
        ggExpRect(out,handX+2*s,handY-3*s,2*s,5*s,202,164,109);
        ggExpRect(out,handX,handY-s,6*s,s,109,79,57);
    }else return;
    ggExpRect(out,handX-s,handY-s,3*s,2*s,skin[0],skin[1],skin[2]);
}
// Hand-authored procedural 2.5D cast. Unlike the old giant 16x24 bitmap,
// figures have tapered limbs, rounded faces, individualized silhouettes,
// a shaded face/coat, readable eyes and hair, and expressive contact hands.
// All primitives write directly to the EXISTING 400x240 RGB buffer.
// No animation atlas, heap, alpha framebuffer, matrix or float math.
static int ggFilmArtShade(int v,int light){
    return ggExpClamp(v*light/256,0,255);
}
static void ggFilmArtRounded(u8*out,int x,int y,int w,int h,int cut,
                             int r,int g,int b){
    if(w<=0||h<=0)return;
    cut=ggExpClamp(cut,0,ggExpClamp(w/3,0,h/3));
    if(!cut){ggExpRect(out,x,y,w,h,r,g,b);return;}
    ggExpRect(out,x+cut,y,w-2*cut,cut,r,g,b);
    ggExpRect(out,x,y+cut,w,h-2*cut,r,g,b);
    ggExpRect(out,x+cut,y+h-cut,w-2*cut,cut,r,g,b);
}
// Soft native-resolution contour. Draws at most 16 horizontal bands
// per shape: unlike 2x enlarged box pixels, round cheeks/shoulders remain
// curved even on closeups. One RGB framebuffer; no sqrt/floats/textures.
static void ggFilmArtOval(u8*out,int x,int y,int w,int h,
                           int r,int g,int b){
    static const u8 taper[16]={
        10,8,6,5,3,2,1,0,0,1,2,3,5,6,8,10
    };
    if(w<=0||h<=0)return;
    int step=h>16?(h+15)/16:1;
    for(int row=0;row<h;row+=step){
        int i=row*16/h,cut=(w*taper[i])/32;
        int span=w-cut*2,band=step;
        if(row+band>h)band=h-row;
        if(span<1)continue;
        ggExpRect(out,x+cut,y+row,span,band,r,g,b);
        if(span>5&&i>1&&i<14)
            ggExpRect(out,x+cut+1,y+row,1,band,
                      ggFilmArtShade(r,277),
                      ggFilmArtShade(g,277),
                      ggFilmArtShade(b,277));
    }
}
// A tapered jointed sleeve rather than an independently sliding square.
// Stroke cost is bounded: at most 12 short rectangles per joint.
static void ggFilmArtStroke(u8*out,int x0,int y0,int x1,int y1,int thick,
                            int r,int g,int b){
    int dx=x1-x0,dy=y1-y0;
    int adx=dx<0?-dx:dx,ady=dy<0?-dy:dy;
    int n=adx>ady?adx:ady;
    n=ggExpClamp(n/3,1,12);
    int rad=thick/2;
    for(int i=0;i<=n;i++){
        int px=x0+dx*i/n,py=y0+dy*i/n;
        ggExpRect(out,px-rad,py-rad,thick,thick,r,g,b);
    }
}
static void ggFilmArtFigure(u8*out,int x,int feet,int scale,
                             const u8*hair,const u8*skin,const u8*coat,
                             const GgFilmPose *p,int identity){
    int s=ggExpClamp(scale,1,3);
    int headX=x+p->head*s,bodyX=x+p->lean*s;
    int centerY=feet-p->bob*s;
    int inkR=30,inkG=34,inkB=41;
    int jacketR=ggFilmArtShade(coat[0],205),
        jacketG=ggFilmArtShade(coat[1],205),
        jacketB=ggFilmArtShade(coat[2],205);
    int lightR=ggFilmArtShade(coat[0],292),
        lightG=ggFilmArtShade(coat[1],292),
        lightB=ggFilmArtShade(coat[2],292);
    int shadeR=ggFilmArtShade(coat[0],138),
        shadeG=ggFilmArtShade(coat[1],138),
        shadeB=ggFilmArtShade(coat[2],138);
    int faceR=ggFilmArtShade(skin[0],278),
        faceG=ggFilmArtShade(skin[1],278),
        faceB=ggFilmArtShade(skin[2],278);
    int faceShadeR=ggFilmArtShade(skin[0],195),
        faceShadeG=ggFilmArtShade(skin[1],195),
        faceShadeB=ggFilmArtShade(skin[2],195);
    int hairLightR=ggFilmArtShade(hair[0],292),
        hairLightG=ggFilmArtShade(hair[1],292),
        hairLightB=ggFilmArtShade(hair[2],292);
    int style=identity<0?6:identity;
    // Projected boots and feathered shadow share the SAME world foot.
    // No floating contact disk attached to the torso or camera horizon.
    ggFilmArtOval(out,x-12*s,feet-2*s,24*s,2*s,37,43,42);
    ggFilmArtOval(out,x-8*s,feet-s,16*s,s,26,32,32);
    ggExpRect(out,x-4*s,feet-s,8*s,s,20,25,27);
    // Calf, articulated knees and fitted boots. Both feet stay registered
    // to ground, even when dialogue adds head gestures and arm reach.
    for(int leg=0;leg<2;leg++){
        int side=leg?-1:1;
        int offset=leg?p->legL:p->legR;
        int lx=bodyX+side*3*s+offset*s;
        int y=centerY-10*s;
        ggFilmArtStroke(out,bodyX+side*2*s,y,lx,feet-4*s,3*s,
                        inkR,inkG,inkB);
        ggFilmArtStroke(out,bodyX+side*2*s,y,lx,feet-4*s,2*s,
                        shadeR,shadeG,shadeB);
        ggFilmArtRounded(out,lx-3*s,feet-4*s,6*s,3*s,s,
                         43,43,49);
        ggExpRect(out,lx-2*s,feet-3*s,3*s,s,92,91,97);
    }
    // Tunic: dark silhouette and shaped shoulder line, not a flat rectangle.
    ggFilmArtOval(out,bodyX-7*s,centerY-19*s,14*s,12*s,
                  inkR,inkG,inkB);
    ggFilmArtOval(out,bodyX-6*s,centerY-18*s,12*s,10*s,
                  jacketR,jacketG,jacketB);
    ggExpRect(out,bodyX-5*s,centerY-17*s,3*s,8*s,
              lightR,lightG,lightB);
    ggExpRect(out,bodyX+3*s,centerY-17*s,2*s,8*s,
              shadeR,shadeG,shadeB);
    // Shaped hem, vertical tailoring and a raised collar.
    // Asymmetric wind-shifted coat hem introduces motion without moving
    // either foot. The fabric sways independently of grounded legs.
    ggFilmArtRounded(out,bodyX-6*s+p->clothSway*s,
                     centerY-10*s,12*s,3*s,s,
                     jacketR,jacketG,jacketB);
    ggExpRect(out,bodyX,centerY-16*s,s,8*s,44,43,48);
    ggExpRect(out,bodyX-5*s,centerY-12*s,10*s,s,75,63,63);
    ggExpRect(out,bodyX+3*s,centerY-12*s,2*s,s,189,153,92);
    ggFilmArtRounded(out,bodyX-3*s,centerY-21*s,6*s,5*s,s,
                     faceShadeR,faceShadeG,faceShadeB);
    // Sleeves and palms: draw jointed angled segments whose wrists remain
    // physically connected to the cuffs in both idles and handovers.
    for(int arm=0;arm<2;arm++){
        int dir=arm?1:-1;
        int lift=arm?p->armR:p->armL;
        int extra=(p->reach*dir>0)?(p->reach*dir):0;
        int shoulderX=bodyX+dir*6*s;
        int shoulderY=centerY-17*s;
        int elbowX=shoulderX+dir*(1+extra/2)*s;
        int elbowY=centerY-(14+lift/2)*s;
        int wristX=bodyX+dir*(8+extra)*s;
        int wristY=centerY-(13+lift)*s;
        ggFilmArtStroke(out,shoulderX,shoulderY,elbowX,elbowY,
                        3*s,inkR,inkG,inkB);
        ggFilmArtStroke(out,shoulderX,shoulderY,elbowX,elbowY,
                        2*s,jacketR,jacketG,jacketB);
        ggFilmArtStroke(out,elbowX,elbowY,wristX,wristY,
                        2*s,shadeR,shadeG,shadeB);
        ggFilmArtRounded(out,wristX-s,wristY-s,3*s,3*s,s,
                         faceShadeR,faceShadeG,faceShadeB);
        ggExpRect(out,wristX,wristY-s,s,s,faceR,faceG,faceB);
    }
    // Neck, cheeks and tapered jaw (6 bands create an organic silhouette).
    ggFilmArtOval(out,headX-5*s,centerY-30*s,10*s,12*s,
                  39,39,46);
    ggFilmArtOval(out,headX-4*s,centerY-29*s,8*s,10*s,
                  faceShadeR,faceShadeG,faceShadeB);
    ggFilmArtOval(out,headX-3*s,centerY-28*s,6*s,8*s,
                  faceR,faceG,faceB);
    ggExpRect(out,headX-3*s,centerY-23*s,2*s,3*s,
              faceShadeR,faceShadeG,faceShadeB);
    // Hair has a crown, swept fringe, temporal shading and a highlight;
    // head/gaze offset is independent from torso animation.
    ggFilmArtOval(out,headX-5*s,centerY-31*s,10*s,5*s,
                  hair[0],hair[1],hair[2]);
    ggExpRect(out,headX-5*s,centerY-28*s,2*s,6*s,
              hair[0],hair[1],hair[2]);
    ggExpRect(out,headX+4*s,centerY-28*s,2*s,5*s,
              ggFilmArtShade(hair[0],174),
              ggFilmArtShade(hair[1],174),
              ggFilmArtShade(hair[2],174));
    ggExpRect(out,headX-3*s,centerY-30*s,4*s,s,
              hairLightR,hairLightG,hairLightB);
    // Expressive brows, sclera, irises, nose, lips and a jaw highlight:
    // face features are NOT enlarged 5x7 grid letters or block eyes.
    int gaze=ggExpClamp(p->gaze,-1,1)*s;
    int eyesY=centerY-25*s;
    for(int eye=0;eye<2;eye++){
        int ex=headX+(eye?2:-3)*s+gaze;
        int ew=s>1?s:2,iris=s>1?s-1:1;
        ggExpRect(out,ex,eyesY-s,ew+1,1,
                  ggFilmArtShade(hair[0],115),
                  ggFilmArtShade(hair[1],115),
                  ggFilmArtShade(hair[2],115));
        if(!p->blink){
            ggExpRect(out,ex,eyesY,ew,ggExpClamp(s-1,1,2),
                      230,226,211);
            ggExpRect(out,ex+(eye?0:iris),eyesY,iris,iris,
                      44,60,68);
            ggExpRect(out,ex+(eye?0:iris),eyesY,1,1,161,184,179);
        }else ggExpRect(out,ex,eyesY,ew,1,
                        faceShadeR,faceShadeG,faceShadeB);
    }
    ggExpRect(out,headX,centerY-23*s,s,2*s,
              faceShadeR,faceShadeG,faceShadeB);
    if(p->mouth)ggExpRect(out,headX-s,centerY-21*s,
                         p->mouth==2?3*s:2*s,s,119,59,65);
    else ggExpRect(out,headX-s,centerY-21*s,2*s,1,122,84,76);
    // Character-exclusive costume accents and hair shapes; the same
    // distinct person remains recognizable across stories/ending routes.
    if(style==0){ // Mira: warm shoulder shawl and visible side braid
        ggExpRect(out,bodyX-5*s,centerY-17*s,10*s,2*s,79,151,116);
        ggExpRect(out,headX-6*s+p->clothSway*s,centerY-26*s,2*s,8*s,
                  hair[0],hair[1],hair[2]);
        ggExpRect(out,headX-6*s+p->clothSway*s,centerY-19*s,
                  2*s,s,205,167,102);
    }else if(style==1){ // Thorgrim: reinforced watch armor
        ggFilmArtRounded(out,bodyX-8*s,centerY-18*s,5*s,4*s,s,
                         106,121,142);
        ggFilmArtRounded(out,bodyX+4*s,centerY-18*s,5*s,4*s,s,
                         106,121,142);
        ggExpRect(out,bodyX-2*s,centerY-15*s,4*s,2*s,190,171,127);
        ggExpRect(out,headX-3*s,centerY-22*s,6*s,2*s,98,72,63);
    }else if(style==2){ // Varro: crimson officer lapels, dark cloak
        ggExpRect(out,bodyX-5*s,centerY-17*s,2*s,8*s,171,66,76);
        ggExpRect(out,bodyX+3*s,centerY-17*s,2*s,8*s,171,66,76);
        ggExpRect(out,bodyX-2*s,centerY-16*s,4*s,s,224,191,113);
    }else if(style==3){ // Serin: ink-lined scholar robes and pin
        ggExpRect(out,bodyX+2*s,centerY-17*s,2*s,8*s,202,167,92);
        ggExpRect(out,bodyX-2*s,centerY-15*s,2*s,2*s,220,199,142);
        ggExpRect(out,headX+4*s,centerY-27*s,s,5*s,64,55,72);
    }else if(style==4){ // Elowen: pale mantle and shaped circlet
        ggFilmArtRounded(out,bodyX-7*s,centerY-19*s,14*s,4*s,s,
                         148,125,178);
        ggExpRect(out,headX-3*s,centerY-31*s,7*s,s,221,184,105);
        ggExpRect(out,headX,centerY-32*s,2*s,2*s,237,213,154);
    }else if(style==5){ // citizen: fitted earth-tone leather straps
        ggExpRect(out,bodyX-4*s,centerY-17*s,2*s,8*s,148,118,83);
        ggExpRect(out,bodyX+2*s,centerY-17*s,2*s,8*s,148,118,83);
    }else{ // player identity: original selected haircut and colors
        ggExpRect(out,bodyX-4*s,centerY-17*s,2*s,8*s,
                  lightR,lightG,lightB);
    }
    if(p->holdProp){
        int handX=bodyX+(8+p->reach)*s;
        int handY=centerY-(13+p->armR)*s;
        ggFilmDrawHeldObject(out,handX,handY,s,p->holdProp,skin);
    }
}
// Existing game's player outfit remains customizable. Guest film actors
// receive their own readable silhouette and acting details.
static void ggFilmDrawActor(u8*out,int x,int feet,int scale,
                            const u8*hair,const u8*skin,const u8*coat,
                            const GgFilmPose *p){
    ggFilmArtFigure(out,x,feet,scale,hair,skin,coat,p,6);
}
static inline void ggFilmDrawGuestActor(u8*out,int x,int feet,int scale,
                                        const u8*hair,const u8*skin,
                                        const u8*coat,const GgFilmPose *p,
                                        int identity){
    ggFilmArtFigure(out,x,feet,scale,hair,skin,coat,p,identity);
}
#endif
