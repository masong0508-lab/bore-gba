// GodGuy 3DS: directional pixel-art inhabitants, from one stable identity.
// Eight facing directions, articulated 32x48 source cels, 24 shaded inks.
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
    // Family hues stay recognizable; a stable dye/wear variation gives
    // neighboring residents individual fabric without frame-to-frame noise.
    int dye=(int)((p.fingerprint>>16)&15u)-7;
    int cr=ggPersonClamp((int)coat[0][0]*6+28+dye,48,208);
    int cg=ggPersonClamp((int)coat[0][1]*6+25+dye,48,203);
    int cb=ggPersonClamp((int)coat[0][2]*6+24+dye,45,196);
    ggPersonInk(c,GG_P_COAT_D,cr*67/100,cg*66/100,cb*70/100);
    ggPersonInk(c,GG_P_COAT_M,cr,cg,cb);
    ggPersonInk(c,GG_P_COAT_L,cr+(255-cr)/4,
                cg+(255-cg)/4,cb+(255-cb)/4);
    ggPersonInk(c,GG_P_PANTS_D,41+(p.wear*10),48+(p.wear*9),
                57+(p.wear*7));
    ggPersonInk(c,GG_P_PANTS_L,71+(p.wear*12),78+(p.wear*10),
                88+(p.wear*9));
    ggPersonInk(c,GG_P_BOOT_D,32,34,39);
    ggPersonInk(c,GG_P_BOOT_L,74,68,61);
    ggPersonInk(c,GG_P_OUTLINE,32,34,39);
    ggPersonInk(c,GG_P_SHIRT,227,207,167);
    ggPersonInk(c,GG_P_SEAM,92,75,67);
    ggPersonInk(c,GG_P_TRIM,195+(p.wear&1)*13,157,102);
    static const unsigned char accents[4][3]={
        {146,67,61},{65,103,114},{121,94,135},{141,117,57}
    };
    ggPersonInk(c,GG_P_ACCENT,accents[p.eye][0],
                accents[p.eye][1],accents[p.eye][2]);
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
// A tapered fabric panel uses connected scanlines, not overlapping
// round blobs. Shoulders, waist and hem therefore have deliberate edges.
static void ggPersonTaper(GgPersonCel*c,int cx,int y0,int y1,
                          int top,int bottom,int ink){
    if(y1<y0)return;
    int span=y1-y0;
    for(int y=y0;y<=y1;y++){
        int half=span?top+(bottom-top)*(y-y0)/span:top;
        ggPersonRect(c,cx-half,y,half*2+1,1,ink);
    }
}
static void ggPersonArm(GgPersonCel*c,int x,int y,int elbowX,
                        int elbowY,int handX,int handY,int nearSide,
                        int rolled){
    ggPersonStroke(c,x,y,elbowX,elbowY,GG_P_OUTLINE,4);
    ggPersonStroke(c,elbowX,elbowY,handX,handY-1,GG_P_OUTLINE,4);
    ggPersonStroke(c,x,y,elbowX,elbowY,
                   nearSide?GG_P_COAT_M:GG_P_COAT_D,2);
    ggPersonStroke(c,elbowX,elbowY,handX,handY-1,
                   rolled?GG_P_SKIN_D:GG_P_COAT_D,2);
    if(!rolled)ggPersonRect(c,handX-1,handY-2,2,1,GG_P_TRIM);
    ggPersonRect(c,handX-1,handY-1,3,3,GG_P_SKIN_D);
    ggPersonRect(c,handX-1,handY-1,2,2,GG_P_SKIN_M);
    ggPersonDot(c,x-1,y,GG_P_COAT_L);
}
// Physical front (0), quarter turns (1/7, 3/5), side (2/6), back (4).
// Front fasteners never appear through the back. Hair wraps the entire
// rear skull, and a profile has a projecting nose and one visible eye.
static void ggPersonGenerate(GgPersonCel*c,uint32_t id,int family,
                             int turn,int tick,int active,
                             const unsigned char coat[3][3]){
    GgPersonTraits p=ggPersonTraits(id,family);
    memset(c,0,sizeof *c);
    ggPersonColors(c,p,coat);
    turn&=7;
    int rear=turn>=3&&turn<=5;
    int profile=turn==2||turn==6;
    int quarter=turn&1;
    int facing=turn>=1&&turn<=3?-1:turn>=5?1:0;
    int center=16;
    int phase=((unsigned)tick/4u+(p.fingerprint&7u))&7;
    static const signed char stride[8]={0,2,3,2,0,-2,-3,-2};
    static const unsigned char lift[8]={0,1,2,1,0,0,0,0};
    int breath=active?((phase&3)==2?-1:0):
                       (int)(((unsigned)tick+(p.fingerprint&31u))/24u&1u);
    int glance=active?0:ggPersonIdleLook(p.fingerprint,tick);
    int gesture=active?0:ggPersonIdleGesture(p.fingerprint,tick);
    int width=6+(p.build==2?2:p.build==3?1:0)+p.shoulder/2;
    if(profile)width=4+(p.build==2);
    else if(quarter)width--;
    int shoulderY=19+breath,waistY=29+breath;
    int hemY=(p.style==2?34:p.style==3?38:32)+breath;
    int hemW=width-(p.style==0||p.style==1?1:0);
    int headX=center+facing+glance,faceY=10+breath;
    int armX[2],elbowX[2],handX[2],handY[2];
    int elbowY[2],nearArm=profile?(facing<0?0:1):-1;
    // Hips, knees and ankles articulate independently. The advancing
    // boot lifts while the supporting sole remains at row 47; side
    // strides move along the facing axis, never as jumping-jack legs.
    for(int layer=0;layer<2;layer++){
        int side=layer?1:-1;
        int legPhase=(phase+(layer?4:0))&7;
        int swing=active?stride[legPhase]:0;
        int lifted=active?lift[legPhase]:0;
        int hipX=center+side*(profile||quarter?2:3);
        int ankleX=hipX+facing*swing/(quarter?2:1);
        if(!profile)ankleX+=side;
        int kneeX=(hipX+ankleX)/2+facing*(lifted?1:0);
        int soleY=47-lifted;
        ggPersonStroke(c,hipX,31+breath,kneeX,39-lifted/2,
                       GG_P_OUTLINE,5);
        ggPersonStroke(c,kneeX,39-lifted/2,ankleX,soleY-3,
                       GG_P_OUTLINE,4);
        ggPersonStroke(c,hipX-1,32+breath,kneeX-1,39-lifted/2,
                       layer?GG_P_PANTS_D:GG_P_PANTS_L,3);
        ggPersonStroke(c,kneeX-1,39-lifted/2,ankleX-1,soleY-3,
                       layer?GG_P_PANTS_D:GG_P_PANTS_L,2);
        ggPersonDot(c,kneeX,39-lifted/2,GG_P_PANTS_L);
        int toe=profile?facing:side;
        ggPersonRect(c,ankleX-2,soleY-3,4,3,GG_P_BOOT_D);
        ggPersonRect(c,ankleX-2+(toe<0?-1:0),soleY-1,5,1,
                     GG_P_BOOT_L);
        ggPersonRect(c,ankleX-2+(toe<0?-1:0),soleY,5,1,
                     GG_P_OUTLINE);
    }
    for(int i=0;i<2;i++){
        int side=i?1:-1;
        int swing=active?stride[(phase+(i?0:4))&7]:0;
        armX[i]=center+side*(width+1);
        if(profile)armX[i]=center+(i==nearArm?-1:1);
        elbowX[i]=armX[i]+(profile?facing*swing/2:side);
        handX[i]=elbowX[i]+(profile?facing*swing/2:
                            quarter?facing*swing/2:0);
        elbowY[i]=25+breath;
        handY[i]=31+breath-(active&&swing>1?1:0);
        if(gesture&&i==((p.fingerprint&1u)?1:0)){
            elbowX[i]+=side;
            handX[i]=elbowX[i]-side*3;
            handY[i]=20+((unsigned)tick/5u&1u);
        }
        // Far sleeve is genuinely behind the torso in a side view.
        if(profile&&i!=nearArm)
            ggPersonArm(c,armX[i],shoulderY+1,elbowX[i],elbowY[i],
                        handX[i],handY[i],0,p.style==4);
    }
    // Shoulder slope, fitted waist and six clearly different hems.
    ggPersonTaper(c,center,shoulderY-1,shoulderY+2,width-2,width,
                   GG_P_OUTLINE);
    ggPersonTaper(c,center,shoulderY+2,waistY,width,width-1,
                   GG_P_OUTLINE);
    ggPersonTaper(c,center,waistY,hemY,width-1,hemW,GG_P_OUTLINE);
    ggPersonTaper(c,center,shoulderY,shoulderY+3,width-2,width-1,
                   GG_P_COAT_M);
    ggPersonTaper(c,center,shoulderY+3,waistY,width-1,width-2,
                   GG_P_COAT_M);
    ggPersonTaper(c,center,waistY,hemY-1,width-2,hemW-1,
                   GG_P_COAT_M);
    ggPersonStroke(c,center-width+2,shoulderY+3,
                   center-hemW+2,hemY-2,GG_P_COAT_L,1);
    ggPersonStroke(c,center+width-2,shoulderY+3,
                   center+hemW-2,hemY-2,GG_P_COAT_D,2);
    // Hem folds follow the garment; long robes hide thighs naturally.
    if(p.style==2||p.style==3){
        ggPersonStroke(c,center-2,waistY+1,center-3,hemY-1,
                       GG_P_COAT_D,1);
        ggPersonStroke(c,center+2,waistY+2,center+3,hemY-1,
                       GG_P_COAT_L,1);
        ggPersonRect(c,center-hemW+1,hemY-1,2*hemW-1,1,GG_P_TRIM);
    }
    int opening=center+(profile?facing*(width-2):quarter?facing:0);
    if(rear){
        // A shoulder yoke and back seam replace the front shirt/lapels.
        ggPersonStroke(c,center-width+2,shoulderY+4,
                       center+width-2,shoulderY+4,GG_P_COAT_L,1);
        ggPersonStroke(c,center,shoulderY+5,center,hemY-2,
                       GG_P_COAT_D,1);
        if(p.style==5){
            ggPersonTaper(c,center,shoulderY+1,26+breath,
                           width-1,width,GG_P_ACCENT);
            ggPersonRect(c,center-width+1,26+breath,2*width-1,1,
                         GG_P_TRIM);
        }
    }else{
        if(p.style==0){
            // Tailored travel coat: collar, lapels and two small clasps.
            ggPersonRect(c,opening-1,shoulderY,3,7,GG_P_SHIRT);
            ggPersonStroke(c,opening-3,shoulderY+1,opening,shoulderY+6,
                           GG_P_COAT_L,1);
            ggPersonStroke(c,opening+3,shoulderY+1,opening,shoulderY+6,
                           GG_P_COAT_D,1);
            ggPersonStroke(c,opening,shoulderY+6,opening,hemY-1,
                           GG_P_COAT_D,1);
            ggPersonDot(c,opening+1,26+breath,GG_P_METAL);
            ggPersonDot(c,opening+1,29+breath,GG_P_METAL);
        }else if(p.style==1){
            // Open waistcoat: warm shirt panel and visible pocket.
            ggPersonRect(c,opening-2,shoulderY,profile?2:4,
                         hemY-shoulderY-1,GG_P_SHIRT);
            ggPersonStroke(c,opening-3,shoulderY+2,opening-2,hemY-2,
                           GG_P_COAT_D,1);
            ggPersonStroke(c,opening+2,shoulderY+2,opening+2,hemY-2,
                           GG_P_COAT_L,1);
            if(!profile)ggPersonRect(c,center+width-4,26+breath,3,1,
                                    GG_P_TRIM);
        }else if(p.style==2){
            // Long tunic: laced neckline and side-split lower panels.
            ggPersonRect(c,opening-1,shoulderY,3,5,GG_P_COAT_D);
            ggPersonDot(c,opening,shoulderY+1,GG_P_TRIM);
            ggPersonDot(c,opening,shoulderY+3,GG_P_TRIM);
            ggPersonStroke(c,opening,waistY+2,opening,hemY,
                           GG_P_PANTS_D,1);
        }else if(p.style==3){
            // Robe: diagonal wrap and a continuous woven sash.
            ggPersonStroke(c,opening-3,shoulderY+1,
                           opening+2,waistY-1,GG_P_SHIRT,2);
            ggPersonRect(c,center-width+2,waistY,2*width-3,2,
                         GG_P_ACCENT);
            ggPersonRect(c,opening+2,waistY+2,2,5,GG_P_ACCENT);
        }else if(p.style==4){
            // Canvas apron over a short tunic, with a useful silhouette.
            ggPersonRect(c,opening-2,shoulderY+4,profile?3:5,6,
                         GG_P_TRIM);
            ggPersonTaper(c,opening,waistY-1,hemY-1,
                           profile?2:3,profile?2:4,GG_P_TRIM);
            ggPersonStroke(c,opening-2,shoulderY+1,
                           opening-2,shoulderY+5,GG_P_SHIRT,1);
            ggPersonRect(c,opening-1,waistY+1,3,1,GG_P_SEAM);
        }else{
            // Shoulder mantle changes the outline, not just its color.
            ggPersonTaper(c,center,shoulderY,shoulderY+5,
                           width-2,width+1,GG_P_ACCENT);
            ggPersonRect(c,center-width,shoulderY+5,2*width+1,1,
                         GG_P_TRIM);
            ggPersonDot(c,opening,shoulderY+2,GG_P_METAL);
            ggPersonStroke(c,opening,shoulderY+7,opening,hemY-1,
                           GG_P_COAT_D,1);
        }
    }
    if(p.style!=3&&p.style!=4){
        ggPersonRect(c,center-width+2,waistY,2*width-3,2,GG_P_BOOT_D);
        if(!rear&&!profile){
            ggPersonRect(c,opening,waistY,2,2,GG_P_TRIM);
            ggPersonDot(c,opening,waistY,GG_P_METAL);
        }
    }
    // Accessory straps follow the correct side of the body. Rear bags
    // and shawls use their reverse face; no floating pins on empty air.
    if(p.accessory==0){
        int strap=rear?-1:1;
        ggPersonStroke(c,center-strap*(width-2),shoulderY+1,
                       center+strap*(width-2),waistY+2,GG_P_BOOT_D,2);
        ggPersonStroke(c,center-strap*(width-2),shoulderY+1,
                       center+strap*(width-2),waistY+2,GG_P_TRIM,1);
        int bagX=center+(profile?-facing:strap)*(width-1);
        ggPersonRect(c,bagX-2,waistY+1,5,5,GG_P_BOOT_D);
        ggPersonRect(c,bagX-1,waistY+1,3,4,GG_P_ACCENT);
        ggPersonRect(c,bagX-1,waistY+1,3,1,GG_P_TRIM);
    }else if(p.accessory==1){
        ggPersonRect(c,center-3,shoulderY,7,2,GG_P_ACCENT);
        if(!rear)ggPersonRect(c,opening+1,shoulderY+2,2,6,
                             GG_P_ACCENT);
    }else if(p.accessory==2&&!rear&&!profile){
        ggPersonStroke(c,opening-2,shoulderY+1,opening,shoulderY+5,
                       GG_P_TRIM,1);
        ggPersonDot(c,opening,shoulderY+5,GG_P_METAL);
    }else if(p.accessory==3&&!rear){
        ggPersonRect(c,center-width+2,shoulderY+5,2,2,GG_P_METAL);
    }
    if(p.wear==1&&!rear){
        ggPersonRect(c,center+width-3,waistY-3,2,2,GG_P_SEAM);
        ggPersonDot(c,center+width-2,waistY-3,GG_P_COAT_L);
    }
    for(int i=0;i<2;i++){
        if(profile&&i!=nearArm)continue;
        ggPersonArm(c,armX[i],shoulderY+1,elbowX[i],elbowY[i],
                    handX[i],handY[i],i==0,p.style==4);
    }
    // Neck and ears connect the narrower, adult-proportioned head to
    // the shoulder line. Crown remains inside the cel in every pose.
    ggPersonRect(c,headX-1,faceY+5,3,shoulderY-faceY-3,GG_P_SKIN_D);
    ggPersonRect(c,headX-1,faceY+5,2,shoulderY-faceY-4,GG_P_SKIN_M);
    int jaw=(p.face==2?4:3),skull=5;
    if(profile)skull=4;
    ggPersonOval(c,headX,faceY,skull+1,6,GG_P_OUTLINE);
    ggPersonOval(c,headX,faceY,skull,5,rear?GG_P_HAIR_D:GG_P_SKIN_D);
    if(rear){
        ggPersonOval(c,headX-1,faceY-1,skull-1,4,GG_P_HAIR_M);
        ggPersonStroke(c,headX-2,faceY-3,headX-2,faceY+2,
                       GG_P_HAIR_L,1);
        ggPersonRect(c,headX-2,faceY+4,5,2,GG_P_HAIR_D);
        if(quarter)ggPersonRect(c,headX+facing*skull,faceY,1,3,
                               GG_P_SKIN_D);
    }else{
        ggPersonOval(c,headX-1,faceY,skull-1,4,GG_P_SKIN_M);
        ggPersonOval(c,headX,faceY+3,jaw,2,GG_P_SKIN_M);
        if(profile){
            int nose=headX+facing*(skull+1);
            ggPersonRect(c,nose+(facing<0?-1:0),faceY,2,2,
                         GG_P_SKIN_M);
            ggPersonDot(c,nose,faceY,GG_P_SKIN_L);
            ggPersonRect(c,headX-facing*2,faceY,2,3,GG_P_SKIN_D);
            ggPersonDot(c,headX-facing,faceY,GG_P_SKIN_L);
        }else{
            ggPersonDot(c,headX-skull,faceY+1,GG_P_SKIN_M);
            ggPersonDot(c,headX+skull,faceY+1,GG_P_SKIN_D);
            ggPersonStroke(c,headX-3,faceY+1,headX-2,faceY+3,
                           GG_P_SKIN_L,1);
        }
        int noseX=headX+(quarter?facing:profile?facing*3:0);
        int blink=((unsigned)tick+(p.fingerprint>>11))%91u<3u;
        if(profile){
            int eye=headX+facing*2;
            ggPersonDot(c,eye,faceY-1,GG_P_HAIR_D);
            ggPersonDot(c,eye,faceY,blink?GG_P_SKIN_D:GG_P_IRIS);
            ggPersonDot(c,noseX,faceY+3,GG_P_FACE);
        }else{
            for(int side=-1;side<=1;side+=2){
                int eye=headX+side*2+(quarter?facing:0);
                ggPersonStroke(c,eye-1,faceY-2,eye,faceY-2,
                               GG_P_HAIR_D,1);
                ggPersonDot(c,eye-1,faceY,blink?GG_P_SKIN_D:GG_P_WHITE);
                ggPersonDot(c,eye,faceY,blink?GG_P_SKIN_D:GG_P_IRIS);
            }
            ggPersonDot(c,noseX,faceY+1,GG_P_SKIN_L);
            ggPersonDot(c,noseX+1,faceY+2,GG_P_SKIN_D);
            ggPersonRect(c,noseX-1,faceY+4,2,1,GG_P_FACE);
            if(p.face==1){
                ggPersonRect(c,noseX-2,faceY+3,4,1,GG_P_HAIR_D);
                ggPersonDot(c,noseX,faceY+5,GG_P_HAIR_D);
            }else if(p.face==3){
                ggPersonDot(c,headX+3,faceY+2,GG_P_BLUSH);
            }
        }
    }
    // Sculpted crown and side locks all stay attached. The same cut
    // wraps behind the skull rather than painting a face on its back.
    ggPersonOval(c,headX,faceY-4,skull+1,3,GG_P_HAIR_D);
    ggPersonOval(c,headX-1,faceY-5,skull,2,GG_P_HAIR_M);
    if(p.hair==0){
        ggPersonRect(c,headX-skull,faceY-4,2,4,GG_P_HAIR_D);
        ggPersonRect(c,headX+skull-1,faceY-4,2,3,GG_P_HAIR_D);
        ggPersonStroke(c,headX-3,faceY-5,headX+1,faceY-5,
                       GG_P_HAIR_L,1);
    }else if(p.hair==1||p.hair==6){
        for(int n=-2;n<=2;n++){
            int xx=headX+n*2,yy=faceY-5-((n+2)&1);
            ggPersonOval(c,xx,yy,2,2,n&1?GG_P_HAIR_M:GG_P_HAIR_L);
        }
        ggPersonRect(c,headX-skull,faceY-3,2,4,GG_P_HAIR_M);
        if(p.hair==6)ggPersonRect(c,headX+skull-1,faceY-3,2,5,
                                  GG_P_HAIR_D);
    }else if(p.hair==2){
        if(rear){
            ggPersonTaper(c,headX,faceY+1,shoulderY+1,skull,skull+1,
                           GG_P_HAIR_D);
            ggPersonStroke(c,headX-2,faceY-2,headX-3,shoulderY,
                           GG_P_HAIR_M,2);
            ggPersonStroke(c,headX+2,faceY,headX+3,shoulderY-1,
                           GG_P_HAIR_L,1);
        }else{
            ggPersonRect(c,headX-skull-1,faceY-3,2,11,GG_P_HAIR_D);
            ggPersonRect(c,headX+skull-1,faceY-3,2,12,GG_P_HAIR_M);
            ggPersonRect(c,headX+skull,faceY+2,1,6,GG_P_HAIR_L);
        }
    }else if(p.hair==3){
        ggPersonStroke(c,headX-4,faceY-5,headX+2,faceY-7,
                       GG_P_HAIR_M,2);
        ggPersonStroke(c,headX-3,faceY-5,headX+1,faceY-6,
                       GG_P_HAIR_L,1);
        if(!rear)ggPersonStroke(c,headX+2,faceY-5,headX-2,faceY-2,
                                GG_P_HAIR_D,2);
    }else if(p.hair==4){
        ggPersonOval(c,headX,faceY-5,skull+1,2,GG_P_HAIR_M);
        ggPersonRect(c,headX-skull,faceY-4,2,5,GG_P_HAIR_D);
        ggPersonStroke(c,headX-3,faceY-6,headX+2,faceY-5,
                       GG_P_HAIR_L,1);
    }else{
        int bunX=headX+(profile?-facing*4:rear?0:3);
        ggPersonOval(c,bunX,faceY-6,3,2,GG_P_HAIR_D);
        ggPersonOval(c,bunX-1,faceY-6,2,1,GG_P_HAIR_L);
        ggPersonRect(c,bunX-1,faceY-4,3,1,GG_P_TRIM);
        ggPersonRect(c,headX-skull,faceY-4,2,4,GG_P_HAIR_D);
    }
    if(profile){
        // Profile nape is on the trailing side, outside the single eye.
        ggPersonRect(c,headX-facing*(skull-1)-1,faceY-4,2,7,
                     GG_P_HAIR_D);
    }
    if(p.accessory==4&&!rear){
        ggPersonRect(c,headX-skull+1,faceY-3,skull*2-1,1,
                     GG_P_ACCENT);
    }
}
static inline unsigned int ggPersonOpaquePixels(const GgPersonCel*c){
    unsigned int n=0;
    for(int i=0;i<GG_PERSON_W*GG_PERSON_H;i++)
        if(c->pixels[i])n++;
    return n;
}
#endif
