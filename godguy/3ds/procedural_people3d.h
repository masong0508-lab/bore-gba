// GodGuy 3DS: anatomically structured, deterministic procedural people.
// Eight facing directions, layered 32x48 source cels with 24 shaded inks.
// One 1.5 KiB cel is regenerated for a visible citizen; no atlas, heap,
// persistent 12,000-citizen caches or platform SDK dependency.
// Identity is derived from a stable actor ID; pose/eyes can animate without
// randomizing hair, clothing, complexion, physique or accessories.
#ifndef GG_PROCEDURAL_PEOPLE3D_H
#define GG_PROCEDURAL_PEOPLE3D_H
#include <stdint.h>
#include <string.h>
enum {GG_PERSON_W=32, GG_PERSON_H=48, GG_PERSON_INKS=24};
enum {
    GG_P_CLEAR,GG_P_OUTLINE,GG_P_BOOT_D,GG_P_BOOT_L,
    GG_P_PANTS_D,GG_P_PANTS_L,
    GG_P_COAT_D,GG_P_COAT_M,GG_P_COAT_L,
    GG_P_SKIN_D,GG_P_SKIN_M,GG_P_SKIN_L,
    GG_P_HAIR_D,GG_P_HAIR_M,GG_P_HAIR_L,
    GG_P_FACE,GG_P_TRIM,GG_P_SHIRT,GG_P_SEAM,
    GG_P_BLUSH,GG_P_IRIS,GG_P_WHITE,GG_P_ACCENT,GG_P_METAL
};
typedef struct {
    unsigned char pixels[GG_PERSON_W*GG_PERSON_H];
    unsigned char palette[GG_PERSON_INKS][3];
} GgPersonCel;
typedef struct {
    uint32_t fingerprint;
    unsigned char hair,hairTint,style,face,build,accessory,complexion;
    unsigned char eye,wear,shoulder;
} GgPersonTraits;
static inline uint32_t ggPersonMix(uint32_t x){
    x^=x>>16;x*=0x7feb352du;x^=x>>15;
    x*=0x846ca68bu;return x^(x>>16);
}
static inline int ggPersonClamp(int x,int lo,int hi){
    return x<lo?lo:x>hi?hi:x;
}
// Standing characters have low-frequency, identity-staggered acting
// even when the original 3DS game simulation is paused. No hidden
// creature movement, arbitrary gait or animation buffers are created.
static inline int ggPersonIdleLook(uint32_t fingerprint,int tick){
    int cycle=128+(int)((fingerprint>>8)&31u);
    int phase=(tick+(int)((fingerprint>>20)&255u))%cycle;
    if(phase<0)phase+=cycle;
    return phase>=cycle-24?((fingerprint&1u)?-1:1):0;
}
static inline int ggPersonIdleGesture(uint32_t fingerprint,int tick){
    int cycle=166+(int)(fingerprint&31u);
    int phase=(tick+(int)((fingerprint>>13)&255u))%cycle;
    if(phase<0)phase+=cycle;
    return phase>=cycle-30&&phase<cycle-12;
}
static inline GgPersonTraits ggPersonTraits(uint32_t id,int family){
    uint32_t h=ggPersonMix(id^((uint32_t)(family&15)*0x9e3779b9u)^
                           0x6d2b79f5u);
    GgPersonTraits p;
    p.fingerprint=h;
    p.hair=(unsigned char)(h%7u);
    p.hairTint=(unsigned char)((h>>28)%7u);
    p.style=(unsigned char)((h>>4)%6u);
    p.face=(unsigned char)((h>>8)%4u);
    p.build=(unsigned char)((h>>11)%4u);
    p.accessory=(unsigned char)((h>>14)%6u);
    p.complexion=(unsigned char)((h>>18)%8u);
    p.eye=(unsigned char)((h>>21)%4u);
    p.wear=(unsigned char)((h>>23)%4u);
    p.shoulder=(unsigned char)((h>>26)%3u);
    return p;
}
static inline void ggPersonInk(GgPersonCel*c,int ink,
                               int r,int g,int b){
    c->palette[ink][0]=(unsigned char)ggPersonClamp(r,0,255);
    c->palette[ink][1]=(unsigned char)ggPersonClamp(g,0,255);
    c->palette[ink][2]=(unsigned char)ggPersonClamp(b,0,255);
}
static void ggPersonColors(GgPersonCel*c,GgPersonTraits p,
                           const unsigned char coat[3][3]){
    static const unsigned char skins[8][3]={
        {246,205,167},{226,172,125},{204,148,107},{175,117,82},
        {146,91,66},{114,72,55},{225,190,160},{183,143,110}
    };
    static const unsigned char hairs[7][3]={
        {55,43,43},{107,77,49},{159,116,64},{201,170,109},
        {99,100,107},{71,59,78},{170,91,60}
    };
    int r=skins[p.complexion][0],g=skins[p.complexion][1],
        b=skins[p.complexion][2];
    ggPersonInk(c,GG_P_SKIN_D,r*74/100,g*71/100,b*69/100);
    ggPersonInk(c,GG_P_SKIN_M,r,g,b);
    ggPersonInk(c,GG_P_SKIN_L,r+(255-r)/3,g+(255-g)/3,b+(255-b)/3);
    ggPersonInk(c,GG_P_BLUSH,ggPersonClamp(r+8,0,255),
                g*79/100,b*81/100);
    r=hairs[p.hairTint][0];g=hairs[p.hairTint][1];
    b=hairs[p.hairTint][2];
    ggPersonInk(c,GG_P_HAIR_D,r*58/100,g*61/100,b*63/100);
    ggPersonInk(c,GG_P_HAIR_M,r,g,b);
    ggPersonInk(c,GG_P_HAIR_L,r+(255-r)/4,g+(255-g)/4,b+(255-b)/4);
    // Keep the simulation's actual family palette, but enforce
    // sufficient saturation and contrast for readable 3D-world fabric.
    int cr=ggPersonClamp((int)coat[0][0]*8,48,200);
    int cg=ggPersonClamp((int)coat[0][1]*8,53,197);
    int cb=ggPersonClamp((int)coat[0][2]*8,51,190);
    ggPersonInk(c,GG_P_COAT_D,cr*58/100,cg*58/100,cb*61/100);
    ggPersonInk(c,GG_P_COAT_M,cr,cg,cb);
    ggPersonInk(c,GG_P_COAT_L,cr+(255-cr)/3,
                cg+(255-cg)/3,cb+(255-cb)/3);
    ggPersonInk(c,GG_P_PANTS_D,41+(p.wear*10),48+(p.wear*9),
                57+(p.wear*7));
    ggPersonInk(c,GG_P_PANTS_L,71+(p.wear*12),78+(p.wear*10),
                88+(p.wear*9));
    ggPersonInk(c,GG_P_BOOT_D,32,34,39);
    ggPersonInk(c,GG_P_BOOT_L,74,68,61);
    ggPersonInk(c,GG_P_OUTLINE,29,32,39);
    ggPersonInk(c,GG_P_SHIRT,227,207,167);
    ggPersonInk(c,GG_P_SEAM,92,75,67);
    ggPersonInk(c,GG_P_TRIM,210,171,111);
    ggPersonInk(c,GG_P_ACCENT,137,67,84);
    ggPersonInk(c,GG_P_METAL,208,201,168);
    ggPersonInk(c,GG_P_FACE,58,45,48);
    ggPersonInk(c,GG_P_IRIS,51+p.eye*13,65+p.eye*7,74+p.eye*6);
    ggPersonInk(c,GG_P_WHITE,236,220,202);
}
static inline void ggPersonDot(GgPersonCel*c,int x,int y,int col){
    if((unsigned)x<GG_PERSON_W&&(unsigned)y<GG_PERSON_H)
        c->pixels[y*GG_PERSON_W+x]=(unsigned char)col;
}
static void ggPersonRect(GgPersonCel*c,int x,int y,int w,int h,int ink){
    for(int yy=0;yy<h;yy++)
        for(int xx=0;xx<w;xx++)
            ggPersonDot(c,x+xx,y+yy,ink);
}
static void ggPersonOval(GgPersonCel*c,int x,int y,int rx,int ry,int ink){
    if(rx<1||ry<1)return;
    int64_t x2=(int64_t)rx*rx,y2=(int64_t)ry*ry;
    int64_t limit=x2*y2;
    for(int dy=-ry;dy<=ry;dy++)for(int dx=-rx;dx<=rx;dx++)
        if((int64_t)dx*dx*y2+(int64_t)dy*dy*x2<=limit)
            ggPersonDot(c,x+dx,y+dy,ink);
}
static void ggPersonStroke(GgPersonCel*c,int x0,int y0,
                           int x1,int y1,int ink,int thick){
    int dx=x1-x0;if(dx<0)dx=-dx;
    int dy=y1-y0;if(dy<0)dy=-dy;
    int sx=x0<x1?1:-1,sy=y0<y1?1:-1,err=dx-dy;
    for(int steps=0;steps<90;steps++){
        ggPersonRect(c,x0-thick/2,y0-thick/2,thick,thick,ink);
        if(x0==x1&&y0==y1)break;
        int twice=err*2;
        if(twice>-dy){err-=dy;x0+=sx;}
        if(twice<dx){err+=dx;y0+=sy;}
    }
}
// Physical front (0), quarter turns (1/7, 3/5), side (2/6),
// back (4). The identity/wardrobe is conserved across all directions.
static void ggPersonGenerate(GgPersonCel*c,uint32_t id,int family,
                             int turn,int tick,int active,
                             const unsigned char coat[3][3]){
    GgPersonTraits p=ggPersonTraits(id,family);
    // The transparent ink must be initialized too: two regenerations
    // on different stacks must produce byte-for-byte identical sprites.
    // This is just one 1.6 KiB reusable cel, not an allocation.
    memset(c,0,sizeof *c);
    ggPersonColors(c,p,coat);
    turn&=7;
    int front=turn==0||turn==1||turn==7;
    int rear=turn==3||turn==4||turn==5;
    int left=turn==1||turn==2||turn==3;
    int right=turn==5||turn==6||turn==7;
    int quarter=turn==1||turn==3||turn==5||turn==7;
    int profile=turn==2||turn==6;
    int lean=turn==1||turn==7?1:turn==3||turn==5?-1:0;
    int width=7+(p.build==2?1:p.build==0?-1:0)+p.shoulder/2;
    int breath=((tick+(int)(p.fingerprint&31u))/20)&1;
    int sway=active?((tick/5)&1?2:-2):0;
    int faceY=9+breath;
    int glance=active?0:ggPersonIdleLook(p.fingerprint,tick);
    int gesture=active?0:ggPersonIdleGesture(p.fingerprint,tick);
    int center=16+lean;
    int skin=GG_P_SKIN_M;
    // Shoes are BELOW pants; both retain one common grounded sole.
    // Movement changes strides only when the population marks active.
    for(int s=-1;s<=1;s+=2){
        int legX=center+s*(profile?2:quarter?3:4)+s*sway;
        ggPersonOval(c,legX,39,3,7,GG_P_OUTLINE);
        ggPersonRect(c,legX-2,35,4,9,GG_P_PANTS_D);
        ggPersonRect(c,legX-1,35,2,8,GG_P_PANTS_L);
        ggPersonOval(c,legX+s,45,4,2,GG_P_BOOT_D);
        ggPersonRect(c,legX-2+s,45,4,1,GG_P_BOOT_L);
        ggPersonRect(c,legX+s-3,47,7,1,GG_P_OUTLINE);
    }
    // Outer silhouette first: taper from shoulders to waist and hem.
    // Three-quarter views narrow the torso slightly. A side profile
    // must not retain both full front-facing shoulders and two arms.
    if(profile)width=ggPersonClamp(width-2,4,10);
    else if(quarter)width=ggPersonClamp(width-1,5,11);
    int shoulders=width+2,hem=width-(p.style==3?1:0);
    ggPersonOval(c,center,22+breath,shoulders,5,GG_P_OUTLINE);
    ggPersonRect(c,center-width,22,2*width+1,12,GG_P_OUTLINE);
    ggPersonOval(c,center,33,hem,3,GG_P_OUTLINE);
    ggPersonOval(c,center,23+breath,width,4,GG_P_COAT_M);
    ggPersonRect(c,center-width+1,24,2*width-1,9,GG_P_COAT_M);
    ggPersonOval(c,center,33,hem-1,2,GG_P_COAT_D);
    ggPersonStroke(c,center-width+2,25,center-width+2,33,
                   GG_P_COAT_L,2);
    ggPersonStroke(c,center+width-2,24,center+width-2,32,
                   GG_P_COAT_D,2);
    if(p.style==0||p.style==2){
        // Fitted double-breasted coat and collar.
        ggPersonRect(c,center-2,22,4,11,GG_P_COAT_D);
        ggPersonStroke(c,center-4,23,center,29,GG_P_SHIRT,1);
        ggPersonStroke(c,center+4,23,center,29,GG_P_COAT_L,1);
        ggPersonDot(c,center+2,30,GG_P_METAL);
    }else if(p.style==1||p.style==4){
        // Short jacket with visible shirt panel and lapels.
        ggPersonRect(c,center-2,23,4,10,GG_P_SHIRT);
        ggPersonStroke(c,center-3,23,center-1,28,GG_P_TRIM,1);
        ggPersonStroke(c,center+3,23,center+1,28,GG_P_COAT_D,1);
        ggPersonStroke(c,center-width+2,31,center+width-2,31,
                       GG_P_SEAM,1);
    }else if(p.style==3){
        // Layered robe with elongated hem and a contrasting sash.
        ggPersonOval(c,center,34,hem,3,GG_P_COAT_L);
        ggPersonRect(c,center-1,23,2,12,GG_P_COAT_D);
        ggPersonRect(c,center-width+1,31,2*width-2,2,GG_P_TRIM);
    }else{
        // Soft tunic with under-vest and stitched seam.
        ggPersonRect(c,center-3,23,6,10,GG_P_COAT_D);
        ggPersonRect(c,center-1,23,2,10,GG_P_COAT_L);
    }
    if(p.style!=3){
        ggPersonStroke(c,center-width+1,33,center+width-1,33,
                       GG_P_PANTS_D,2);
        ggPersonRect(c,center-1,32,3,3,GG_P_TRIM);
        ggPersonDot(c,center,33,GG_P_METAL);
    }
    // Articulated arms: shoulder/elbow/wrist poses are separate
    // geometries; both hands rejoin the sleeves, never float outside.
    for(int s=-1;s<=1;s+=2){
        // From the side, the far arm is occluded by the body rather
        // than appearing as an extra detached limb in front.
        if(profile&&s==(left?1:-1))continue;
        int armX=center+s*(shoulders-1);
        int elbowX=armX+s*2;
        int raised=gesture&&s==((p.fingerprint&1u)?1:-1);
        int handX=raised?elbowX-s*5:
                  elbowX+(active?-s*sway/2:0);
        int armEnd=raised?18+((tick>>2)&1):
                   31+((tick+(int)(p.fingerprint>>6))%47<3?-1:0);
        ggPersonStroke(c,armX,23,elbowX,28,GG_P_OUTLINE,5);
        ggPersonStroke(c,armX,23,elbowX,28,GG_P_COAT_M,3);
        ggPersonStroke(c,elbowX,28,handX,armEnd,
                       GG_P_OUTLINE,5);
        ggPersonStroke(c,elbowX,28,handX,armEnd,
                       GG_P_COAT_D,3);
        ggPersonRect(c,handX-2,armEnd,4,3,GG_P_SKIN_D);
        ggPersonRect(c,handX-1,armEnd,2,2,skin);
    }
    // Neck remains centered under the skull in every perspective.
    ggPersonRect(c,center-3,17+breath,6,5,GG_P_SKIN_D);
    ggPersonRect(c,center-2,18+breath,4,4,skin);
    // Lower face jaw and skull: no misaligned Atari-square heads.
    int jawW=4+(p.face==2?1:0),cranium=6+(p.face==3?1:0);
    int headX=center+(left?-2:right?2:0)+glance;
    ggPersonOval(c,headX,faceY,cranium+1,8,GG_P_OUTLINE);
    ggPersonOval(c,headX,faceY,cranium,7,GG_P_SKIN_D);
    ggPersonOval(c,headX-1,faceY-1,cranium-1,6,GG_P_SKIN_M);
    ggPersonOval(c,headX,faceY+4,jawW,3,GG_P_SKIN_M);
    ggPersonStroke(c,headX-3,faceY-3,headX-2,faceY+4,
                   GG_P_SKIN_L,1);
    if(!rear){
        int noseX=headX+(left?-3:right?3:0);
        ggPersonRect(c,noseX,faceY+1,2,2,GG_P_SKIN_L);
        ggPersonDot(c,noseX+(right?1:0),faceY+3,
                    GG_P_SKIN_D);
        int blink=((tick+(int)(p.fingerprint>>11))%91)<3;
        if(front){
            for(int side=-1;side<=1;side+=2){
                int ex=headX+side*(quarter?3:3);
                if(quarter&&((side<0&&right)||(side>0&&left)))
                    continue;
                ggPersonStroke(c,ex-1,faceY-2,ex+1,faceY-2,
                               GG_P_HAIR_D,1);
                ggPersonDot(c,ex,faceY,blink?GG_P_SKIN_D:
                             GG_P_WHITE);
                if(!blink)ggPersonDot(c,ex+side/2,faceY,
                                      GG_P_IRIS);
            }
        }else{
            int ex=headX+(left?-2:2);
            ggPersonRect(c,ex,faceY,2,blink?1:2,
                         blink?GG_P_SKIN_D:GG_P_IRIS);
            ggPersonDot(c,ex,faceY-2,GG_P_HAIR_D);
        }
        // Mouth shifts with head orientation but never exits the jaw.
        int mx=headX+(left?-2:right?2:0);
        ggPersonStroke(c,mx-1,faceY+5,mx+1,faceY+5,
                       GG_P_FACE,1);
        if(p.face==1)ggPersonDot(c,mx+1,faceY+6,GG_P_BLUSH);
    }
    // Hair sits ON the anatomy, not as disconnected rectangles.
    // Eight directions share identical style, cut and stable color.
    ggPersonOval(c,headX,faceY-5,cranium+1,4,GG_P_HAIR_D);
    ggPersonOval(c,headX-1,faceY-6,cranium,3,GG_P_HAIR_M);
    if(p.hair==0||p.hair==4){
        // Cropped or silver short hair, visible close temples.
        ggPersonRect(c,headX-6,faceY-5,2,4,GG_P_HAIR_D);
        ggPersonRect(c,headX+5,faceY-5,2,3,GG_P_HAIR_D);
    }else if(p.hair==1||p.hair==6){
        // Curls: staggered tufts and sideburn silhouette.
        for(int tuft=-2;tuft<=2;tuft++){
            int xx=headX+tuft*3,yy=faceY-8+((tuft+2)&1);
            ggPersonOval(c,xx,yy,2,2,tuft&1?GG_P_HAIR_M:
                                              GG_P_HAIR_L);
        }
        ggPersonRect(c,headX-7,faceY-4,3,6,GG_P_HAIR_M);
    }else if(p.hair==2){
        // Long flowing hair extends behind jaw, but not across eyes.
        ggPersonRect(c,headX-7,faceY-4,3,11,GG_P_HAIR_D);
        ggPersonRect(c,headX+5,faceY-4,3,12,GG_P_HAIR_M);
        ggPersonRect(c,headX+6,faceY+3,1,5,GG_P_HAIR_L);
    }else if(p.hair==3){
        // High swept crest and angular forehead part.
        ggPersonOval(c,headX,faceY-9,5,3,GG_P_HAIR_M);
        ggPersonStroke(c,headX-4,faceY-9,
                       headX+4,faceY-11,GG_P_HAIR_L,2);
        ggPersonRect(c,headX+5,faceY-5,2,3,GG_P_HAIR_D);
    }else{
        // Tied bun with clear profile, light clasp.
        ggPersonOval(c,headX+(rear?0:3),faceY-10,
                     3,3,GG_P_HAIR_M);
        ggPersonRect(c,headX+1,faceY-11,3,1,GG_P_TRIM);
        ggPersonRect(c,headX-6,faceY-4,2,6,GG_P_HAIR_D);
    }
    ggPersonStroke(c,headX-3,faceY-8,headX+1,faceY-9,
                   GG_P_HAIR_L,1);
    // Clothing wear, clasps and a shoulder seam are intentionally
    // tied to stable identity rather than ticking with the gait.
    if(p.wear==1){
        ggPersonStroke(c,center+width-3,27,
                       center+width-2,31,GG_P_SEAM,1);
    }else if(p.wear==2){
        ggPersonDot(c,center-width+2,27,GG_P_COAT_L);
        ggPersonDot(c,center-width+2,29,GG_P_COAT_L);
    }else if(p.wear==3){
        ggPersonRect(c,center-1,31,2,1,GG_P_METAL);
    }
    if(rear){
        // Back of head has no phantom eyes: nape and hair part.
        ggPersonRect(c,headX-1,faceY-2,2,7,GG_P_HAIR_M);
        ggPersonRect(c,headX,faceY-5,1,5,GG_P_HAIR_L);
    }
    // Specific identity markers, intentionally subtle at distance.
    if(p.accessory==0){
        ggPersonStroke(c,center-width+3,26,center+width-2,32,
                       GG_P_ACCENT,2); // satchel strap
        ggPersonRect(c,center+width-3,31,4,4,GG_P_TRIM);
    }else if(p.accessory==1){
        ggPersonRect(c,center-5,22,10,2,GG_P_ACCENT); // scarf
        ggPersonRect(c,center+2,24,2,7,GG_P_ACCENT);
    }else if(p.accessory==2){
        ggPersonDot(c,center,28,GG_P_METAL); // pendant
        ggPersonDot(c,center,29,GG_P_TRIM);
    }else if(p.accessory==3){
        ggPersonRect(c,center-width+1,29,2,3,GG_P_METAL); // pins
        ggPersonRect(c,center+width-2,29,2,3,GG_P_METAL);
    }else if(p.accessory==4){
        ggPersonStroke(c,center-3,22,center+3,22,GG_P_TRIM,1);
    }
}
static inline unsigned int ggPersonOpaquePixels(const GgPersonCel*c){
    unsigned int n=0;
    for(int i=0;i<GG_PERSON_W*GG_PERSON_H;i++)
        if(c->pixels[i])n++;
    return n;
}
#endif
