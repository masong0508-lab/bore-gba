// First-person weapons composited over the native 400x240 Chronicle view.
#ifndef GG_CHRONICLE_COMBAT_ART3D_H
#define GG_CHRONICLE_COMBAT_ART3D_H

typedef struct { int x,y; } GgChronArtPoint;
typedef struct { int x,y,dx,dy; } GgChronArtPose;
typedef struct { int y0,y1,x,step; } GgChronArtEdge;

enum {
    GG_CHRON_INK, GG_CHRON_SLEEVE, GG_CHRON_SLEEVE_LIGHT,
    GG_CHRON_LEATHER, GG_CHRON_LEATHER_LIGHT,
    GG_CHRON_BRASS_DARK, GG_CHRON_BRASS, GG_CHRON_GOLD,
    GG_CHRON_STEEL_DARK, GG_CHRON_STEEL, GG_CHRON_STEEL_LIGHT,
    GG_CHRON_YEW_DARK, GG_CHRON_YEW, GG_CHRON_YEW_LIGHT,
    GG_CHRON_SKIN_DARK, GG_CHRON_SKIN, GG_CHRON_SKIN_LIGHT,
    GG_CHRON_COAL, GG_CHRON_COPPER, GG_CHRON_FIRE, GG_CHRON_FLAME,
    GG_CHRON_GLINT
};

static const u8 ggChronArtColor[][3]={
    {34,27,29}, {54,53,55}, {91,85,78},
    {82,51,43}, {145,95,66},
    {104,68,45}, {180,127,65}, {247,199,116},
    {70,80,88}, {143,161,165}, {225,234,224},
    {106,57,41}, {161,98,60}, {229,165,98},
    {128,75,57}, {191,126,89}, {233,170,116},
    {91,49,45}, {173,84,49}, {239,137,59}, {255,194,93},
    {255,238,169}
};

#define GG_CHRON_ART_PT(x,y) ((GgChronArtPoint){(x),(y)})
#define GG_CHRON_ART_FACE(out,ink,...) \
    ggChronArtFill(out,(GgChronArtPoint[]){__VA_ARGS__}, \
        (int)(sizeof((GgChronArtPoint[]){__VA_ARGS__})/sizeof(GgChronArtPoint)),ink)

static void ggChronArtRect(u8 *out,int x,int y,int w,int h,int ink){
    int first=ggExpClamp(y,115,195),last=ggExpClamp(y+h,115,195);
    const u8 *color=ggChronArtColor[ink];
    if(last>first)ggExpRect(out,x,first,w,last-first,color[0],color[1],color[2]);
}

// Each convex face needs at most eight incremental edges and one span per row.
static void ggChronArtFill(u8 *out,const GgChronArtPoint *p,int count,int ink){
    GgChronArtEdge edge[8];
    int first=195,last=115,edges=0;
    for(int i=0;i<count;i++){
        GgChronArtPoint a=p[i],b=p[(i+1)%count];
        if(a.y==b.y)continue;
        if(a.y>b.y){GgChronArtPoint swap=a;a=b;b=swap;}
        int y0=ggExpClamp(a.y,115,195),y1=ggExpClamp(b.y,115,195);
        if(y0>=y1)continue;
        int step=(b.x-a.x)*256/(b.y-a.y);
        edge[edges++]=(GgChronArtEdge){y0,y1,a.x*256+step*(y0-a.y),step};
        if(y0<first)first=y0;
        if(y1>last)last=y1;
    }
    for(int y=first;y<last;y++){
        int left=400*256,right=0;
        for(int i=0;i<edges;i++){
            GgChronArtEdge *e=&edge[i];
            if(y<e->y0||y>=e->y1)continue;
            if(e->x<left)left=e->x;
            if(e->x>right)right=e->x;
            e->x+=e->step;
        }
        int start=left/256,end=(right+255)/256;
        if(end>start)ggChronArtRect(out,start,y,end-start,1,ink);
    }
}

static void ggChronArtLine(u8 *out,GgChronArtPoint a,GgChronArtPoint b,int width,int ink){
    int dx=abs(b.x-a.x),dy=-abs(b.y-a.y);
    int sx=a.x<b.x?1:-1,sy=a.y<b.y?1:-1,error=dx+dy;
    for(;;){
        ggChronArtRect(out,a.x-width/2,a.y-width/2,width,width,ink);
        if(a.x==b.x&&a.y==b.y)break;
        int step=2*error;
        if(step>=dy){error+=dy;a.x+=sx;}
        if(step<=dx){error+=dx;a.y+=sy;}
    }
}

static GgChronArtPoint ggChronArtAt(GgChronArtPoint p,int x,int y){
    return GG_CHRON_ART_PT(p.x+x,p.y+y);
}

static int ggChronArtTween(int from,int to,int t){
    return from+(to-from)*t/255;
}

static int ggChronArtEase(int t){
    return t*t*(765-2*t)/65025;
}

static int ggChronArtEaseOut(int t){
    return t*(510-t)/255;
}

static GgChronArtPose ggChronArtPose(GgChronArtPose a,GgChronArtPose b,int t){
    return (GgChronArtPose){
        ggChronArtTween(a.x,b.x,t),ggChronArtTween(a.y,b.y,t),
        ggChronArtTween(a.dx,b.dx,t),ggChronArtTween(a.dy,b.dy,t)
    };
}

static GgChronArtPoint ggChronArtBladePoint(GgChronArtPose p,int along,int side){
    return GG_CHRON_ART_PT(p.x+(p.dx*along-p.dy*side)/128,
                           p.y+(p.dy*along+p.dx*side)/128);
}

static void ggChronArtSpark(u8 *out,GgChronArtPoint p,int radius,int ink){
    GG_CHRON_ART_FACE(out,ink,ggChronArtAt(p,0,-radius),
        ggChronArtAt(p,radius/2+1,0),ggChronArtAt(p,0,radius),
        ggChronArtAt(p,-radius/2-1,0));
}

static void ggChronArtRibbon(u8 *out,GgChronArtPoint a,GgChronArtPoint b,
                            int nearWidth,int farWidth,int ink){
    int dx=b.x-a.x,dy=b.y-a.y,length=abs(dx)>abs(dy)?abs(dx):abs(dy);
    if(!length)return;
    int ax=dy*nearWidth/(2*length),ay=-dx*nearWidth/(2*length);
    int bx=dy*farWidth/(2*length),by=-dx*farWidth/(2*length);
    GG_CHRON_ART_FACE(out,ink,ggChronArtAt(a,-ax,-ay),ggChronArtAt(b,-bx,-by),
        ggChronArtAt(b,bx,by),ggChronArtAt(a,ax,ay));
}

static GgChronArtPose ggChronArtSwordSwing(int t){
    const GgChronArtPose start={312,169,-54,-86},gather={283,164,-92,-82};
    const GgChronArtPose strike={261,157,-114,-40},carry={224,174,-118,-46};
    const GgChronArtPose end={227,186,-115,-27};
    if(t<68)return ggChronArtPose(start,gather,t*255/68);
    if(t<128)return ggChronArtPose(gather,strike,(t-68)*255/60);
    if(t<208)return ggChronArtPose(strike,carry,(t-128)*255/80);
    return ggChronArtPose(carry,end,(t-208)*255/47);
}

static void ggChronArtSword(u8 *out,int phase,int t,int bob){
    const GgChronArtPose ready={307,173+bob,-45,-91},wound={347,182,48,-100};
    const GgChronArtPose guard={279,153,-119,-15};
    GgChronArtPose pose=ready;
    switch(phase){
    case 1:pose=ggChronArtPose(ready,wound,ggChronArtEaseOut(t));break;
    case 2:pose=ggChronArtSwordSwing(t);break;
    case 3:pose=ggChronArtPose(ready,guard,ggChronArtEaseOut(t));break;
    case 4:pose=ggChronArtPose(ggChronArtSwordSwing(255),ready,ggChronArtEase(t));break;
    }
    if(phase==2&&t>=45&&t<230){
        GgChronArtPose old=ggChronArtSwordSwing(t-42);
        GgChronArtPoint tip=ggChronArtBladePoint(pose,81,0);
        GgChronArtPoint past=ggChronArtBladePoint(old,82,-4);
        GgChronArtPoint inside=ggChronArtBladePoint(pose,35,1);
        int crest=t<158?5:3;
        GG_CHRON_ART_FACE(out,GG_CHRON_BRASS,
            ggChronArtAt(tip,-3,3),ggChronArtAt(tip,-4,-crest),
            ggChronArtAt(past,9,-3),ggChronArtAt(past,4,4),inside);
        GG_CHRON_ART_FACE(out,GG_CHRON_GOLD,
            ggChronArtAt(tip,-5,-4),ggChronArtAt(tip,1,1),
            ggChronArtAt(past,3,-2),ggChronArtAt(past,8,-4));
        if(t>=105&&t<179){
            ggChronArtLine(out,ggChronArtAt(tip,-13,8),
                ggChronArtAt(tip,6,-5),2,GG_CHRON_FLAME);
            ggChronArtSpark(out,ggChronArtAt(tip,-7,4),4,GG_CHRON_GLINT);
            ggChronArtSpark(out,ggChronArtAt(tip,-18,-3),2,GG_CHRON_GOLD);
        }
    }
#define GG_CHRON_BLADE_PT(a,s) ggChronArtBladePoint(pose,a,s)
    GgChronArtPoint hand=GG_CHRON_BLADE_PT(-18,0);
    int sleeveX=ggExpClamp(302+hand.x/7,330,372);
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,ggChronArtAt(hand,-13,1),
        ggChronArtAt(hand,13,-8),GG_CHRON_ART_PT(sleeveX+35,212),
        GG_CHRON_ART_PT(sleeveX-20,218));
    GG_CHRON_ART_FACE(out,GG_CHRON_SLEEVE,ggChronArtAt(hand,-10,2),
        ggChronArtAt(hand,10,-4),GG_CHRON_ART_PT(sleeveX+28,211),
        GG_CHRON_ART_PT(sleeveX-16,213));
    GG_CHRON_ART_FACE(out,GG_CHRON_SLEEVE_LIGHT,ggChronArtAt(hand,7,-3),
        GG_CHRON_ART_PT(sleeveX+27,211),GG_CHRON_ART_PT(sleeveX+9,208));
    GgChronArtPoint cuff=ggChronArtAt(hand,10,8);
    GgChronArtPoint cuffEdge=ggChronArtAt(cuff,17,11);
    ggChronArtRibbon(out,ggChronArtAt(cuff,-11,4),ggChronArtAt(cuffEdge,-10,1),
                     12,7,GG_CHRON_STEEL_DARK);
    ggChronArtRibbon(out,cuff,cuffEdge,11,9,GG_CHRON_STEEL);
    ggChronArtLine(out,ggChronArtAt(cuff,-10,6),ggChronArtAt(cuff,7,-1),
                   2,GG_CHRON_BRASS);
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,GG_CHRON_BLADE_PT(-2,-11),
        GG_CHRON_BLADE_PT(10,-12),GG_CHRON_BLADE_PT(62,-9),
        GG_CHRON_BLADE_PT(80,0),GG_CHRON_BLADE_PT(65,9),
        GG_CHRON_BLADE_PT(10,12),GG_CHRON_BLADE_PT(-2,11));
    GG_CHRON_ART_FACE(out,GG_CHRON_STEEL_LIGHT,GG_CHRON_BLADE_PT(4,-9),
        GG_CHRON_BLADE_PT(61,-7),GG_CHRON_BLADE_PT(78,0),
        GG_CHRON_BLADE_PT(14,0),GG_CHRON_BLADE_PT(4,-3));
    GG_CHRON_ART_FACE(out,GG_CHRON_STEEL,GG_CHRON_BLADE_PT(4,-3),
        GG_CHRON_BLADE_PT(14,0),GG_CHRON_BLADE_PT(78,0),
        GG_CHRON_BLADE_PT(63,7),GG_CHRON_BLADE_PT(4,9));
    GG_CHRON_ART_FACE(out,GG_CHRON_STEEL_DARK,GG_CHRON_BLADE_PT(14,0),
        GG_CHRON_BLADE_PT(69,1),GG_CHRON_BLADE_PT(46,3),
        GG_CHRON_BLADE_PT(14,3));
    ggChronArtLine(out,GG_CHRON_BLADE_PT(12,-8),GG_CHRON_BLADE_PT(59,-6),1,GG_CHRON_GLINT);
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,GG_CHRON_BLADE_PT(-34,-7),
        GG_CHRON_BLADE_PT(4,-8),GG_CHRON_BLADE_PT(4,8),
        GG_CHRON_BLADE_PT(-34,7));
    GG_CHRON_ART_FACE(out,GG_CHRON_LEATHER,GG_CHRON_BLADE_PT(-31,-4),
        GG_CHRON_BLADE_PT(2,-5),GG_CHRON_BLADE_PT(2,5),
        GG_CHRON_BLADE_PT(-31,4));
    GG_CHRON_ART_FACE(out,GG_CHRON_BRASS,GG_CHRON_BLADE_PT(-35,-6),
        GG_CHRON_BLADE_PT(-31,-5),GG_CHRON_BLADE_PT(-29,4),
        GG_CHRON_BLADE_PT(-34,7),GG_CHRON_BLADE_PT(-38,0));
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,GG_CHRON_BLADE_PT(-30,-5),
        GG_CHRON_BLADE_PT(-12,-13),GG_CHRON_BLADE_PT(-6,-8),
        GG_CHRON_BLADE_PT(-8,7),GG_CHRON_BLADE_PT(-18,14),
        GG_CHRON_BLADE_PT(-31,6));
    GG_CHRON_ART_FACE(out,GG_CHRON_LEATHER_LIGHT,GG_CHRON_BLADE_PT(-28,-5),
        GG_CHRON_BLADE_PT(-12,-11),GG_CHRON_BLADE_PT(-8,-6),
        GG_CHRON_BLADE_PT(-10,1),GG_CHRON_BLADE_PT(-23,8));
    GG_CHRON_ART_FACE(out,GG_CHRON_BRASS_DARK,GG_CHRON_BLADE_PT(-27,1),
        GG_CHRON_BLADE_PT(-11,-4),GG_CHRON_BLADE_PT(-12,8),
        GG_CHRON_BLADE_PT(-21,11),GG_CHRON_BLADE_PT(-29,5));
    for(int n=-25;n<=-10;n+=5){
        ggChronArtRibbon(out,GG_CHRON_BLADE_PT(n,-5),GG_CHRON_BLADE_PT(n+1,5),
                         3,3,GG_CHRON_BRASS);
        ggChronArtLine(out,GG_CHRON_BLADE_PT(n,-6),GG_CHRON_BLADE_PT(n,-2),
                       1,GG_CHRON_GOLD);
    }
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,GG_CHRON_BLADE_PT(-5,-22),
        GG_CHRON_BLADE_PT(2,-20),GG_CHRON_BLADE_PT(6,-8),
        GG_CHRON_BLADE_PT(6,8),GG_CHRON_BLADE_PT(2,20),
        GG_CHRON_BLADE_PT(-5,22),GG_CHRON_BLADE_PT(-8,0));
    GG_CHRON_ART_FACE(out,GG_CHRON_BRASS,GG_CHRON_BLADE_PT(-4,-19),
        GG_CHRON_BLADE_PT(1,-18),GG_CHRON_BLADE_PT(4,-7),
        GG_CHRON_BLADE_PT(4,7),GG_CHRON_BLADE_PT(1,18),
        GG_CHRON_BLADE_PT(-4,19),GG_CHRON_BLADE_PT(-6,0));
    ggChronArtLine(out,GG_CHRON_BLADE_PT(0,-17),GG_CHRON_BLADE_PT(3,-7),2,GG_CHRON_GOLD);
    ggChronArtLine(out,GG_CHRON_BLADE_PT(3,7),GG_CHRON_BLADE_PT(0,17),2,GG_CHRON_GOLD);
    ggChronArtSpark(out,GG_CHRON_BLADE_PT(0,0),5,GG_CHRON_GOLD);
    ggChronArtSpark(out,GG_CHRON_BLADE_PT(0,0),2,GG_CHRON_LEATHER);
#undef GG_CHRON_BLADE_PT
}

// A slight roll brings the bow square to the aim point as the arrow draws.
static GgChronArtPoint ggChronArtBowPoint(GgChronArtPoint grip,int roll,int x,int y){
    return ggChronArtAt(grip,x+y*roll/64,y-x*roll/128);
}

static void ggChronArtBowLimb(u8 *out,GgChronArtPoint grip,int roll,int sign,int bend){
    GgChronArtPoint shoulder=ggChronArtBowPoint(grip,roll,4,sign*14);
    GgChronArtPoint knee=ggChronArtBowPoint(grip,roll,14+bend/2,sign*28);
    GgChronArtPoint tip=ggChronArtBowPoint(grip,roll,29+bend,sign*(36-bend/4));
    ggChronArtRibbon(out,grip,shoulder,13,12,GG_CHRON_INK);
    ggChronArtRibbon(out,shoulder,knee,12,9,GG_CHRON_INK);
    ggChronArtRibbon(out,knee,tip,9,5,GG_CHRON_INK);
    ggChronArtRibbon(out,grip,shoulder,9,8,GG_CHRON_YEW_DARK);
    ggChronArtRibbon(out,shoulder,knee,8,6,GG_CHRON_YEW);
    ggChronArtRibbon(out,knee,tip,6,3,GG_CHRON_YEW);
    ggChronArtLine(out,ggChronArtAt(grip,-3,0),ggChronArtAt(shoulder,-3,0),2,GG_CHRON_YEW_LIGHT);
    ggChronArtLine(out,ggChronArtAt(shoulder,-3,0),ggChronArtAt(knee,-2,1),2,GG_CHRON_YEW_LIGHT);
    ggChronArtLine(out,ggChronArtAt(knee,-2,1),tip,1,GG_CHRON_GOLD);
    ggChronArtLine(out,tip,ggChronArtAt(tip,4,-sign*3),3,GG_CHRON_BRASS);
    ggChronArtLine(out,ggChronArtAt(shoulder,-4,0),ggChronArtAt(shoulder,3,-1),2,GG_CHRON_GOLD);
}

static void ggChronArtArrow(u8 *out,GgChronArtPoint nock,GgChronArtPoint tip,int flying){
    int dx=tip.x-nock.x,dy=tip.y-nock.y;
    int length=abs(dx)>abs(dy)?abs(dx):abs(dy);
    if(length<8)return;
    GgChronArtPoint neck=ggChronArtAt(tip,-dx*9/length,-dy*9/length);
    ggChronArtLine(out,nock,tip,3,GG_CHRON_INK);
    ggChronArtLine(out,nock,tip,1,flying?GG_CHRON_GOLD:GG_CHRON_YEW_LIGHT);
    GG_CHRON_ART_FACE(out,GG_CHRON_STEEL_DARK,tip,
        ggChronArtAt(neck,dy*4/length,-dx*4/length),
        ggChronArtAt(neck,-dy*4/length,dx*4/length));
    ggChronArtLine(out,tip,neck,1,GG_CHRON_STEEL_LIGHT);
    if(!flying){
        GgChronArtPoint feather=ggChronArtAt(nock,dx*9/length,dy*9/length);
        GG_CHRON_ART_FACE(out,GG_CHRON_GOLD,nock,feather,
            ggChronArtAt(feather,-dy*4/length,dx*4/length),
            ggChronArtAt(nock,-dy*3/length,dx*3/length));
    }
}

static void ggChronArtBowHand(u8 *out,GgChronArtPoint hand,int loose){
    GgChronArtPoint wrist=ggChronArtAt(hand,17,12);
    ggChronArtRibbon(out,wrist,ggChronArtAt(hand,45,47),20,34,GG_CHRON_INK);
    ggChronArtRibbon(out,wrist,ggChronArtAt(hand,45,47),16,28,GG_CHRON_SLEEVE);
    ggChronArtRibbon(out,wrist,ggChronArtAt(hand,25,23),18,20,GG_CHRON_BRASS_DARK);
    ggChronArtLine(out,ggChronArtAt(wrist,-6,5),ggChronArtAt(wrist,6,-4),2,GG_CHRON_YEW_LIGHT);
    if(loose){
        GgChronArtPoint index=ggChronArtAt(hand,-12,-12);
        GgChronArtPoint middle=ggChronArtAt(hand,-14,-5);
        ggChronArtLine(out,ggChronArtAt(hand,4,0),index,6,GG_CHRON_INK);
        ggChronArtLine(out,ggChronArtAt(hand,2,4),middle,6,GG_CHRON_INK);
        ggChronArtLine(out,ggChronArtAt(hand,4,0),index,4,GG_CHRON_LEATHER_LIGHT);
        ggChronArtLine(out,ggChronArtAt(hand,2,4),middle,3,GG_CHRON_LEATHER_LIGHT);
    }
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,ggChronArtAt(hand,-7,0),
        ggChronArtAt(hand,1,-7),ggChronArtAt(hand,10,-6),
        ggChronArtAt(hand,16,0),ggChronArtAt(hand,22,11),
        ggChronArtAt(hand,13,17),ggChronArtAt(hand,2,12));
    GG_CHRON_ART_FACE(out,GG_CHRON_LEATHER,ggChronArtAt(hand,-4,0),
        ggChronArtAt(hand,1,-5),ggChronArtAt(hand,9,-4),
        ggChronArtAt(hand,14,0),ggChronArtAt(hand,19,11),
        ggChronArtAt(hand,12,14),ggChronArtAt(hand,3,10));
    ggChronArtLine(out,ggChronArtAt(hand,-2,0),ggChronArtAt(hand,8,-3),2,GG_CHRON_LEATHER_LIGHT);
    ggChronArtLine(out,ggChronArtAt(hand,4,5),ggChronArtAt(hand,13,2),2,GG_CHRON_LEATHER_LIGHT);
}

static void ggChronArtBow(u8 *out,int phase,int t,int bob){
    int draw=0,raise=0,kick=0,bend=0;
    if(phase==1){
        draw=ggChronArtEaseOut(t);
        raise=ggChronArtEaseOut(ggExpClamp(t+20,0,255));
    }else if(phase==2){
        raise=ggChronArtTween(245,130,t);
        kick=t<55?ggChronArtTween(6,13,t*255/55):ggChronArtTween(13,0,(t-55)*255/200);
        bend=t<65?-7+t/6:t<140?4-(t-65)/15:0;
    }else if(phase==3){
        raise=ggChronArtEaseOut(t);
    }else if(phase==4){
        raise=ggChronArtTween(130,0,ggChronArtEase(t));
    }
    bend+=draw/21;
    int roll=ggChronArtTween(-32,-4,raise)-kick;
    GgChronArtPoint grip=GG_CHRON_ART_PT(164+raise*47/255-kick,171-raise*22/255+bob+kick/2);
    int halfHeight=36-bend/4;
    GgChronArtPoint top=ggChronArtBowPoint(grip,roll,29+bend,-halfHeight);
    GgChronArtPoint bottom=ggChronArtBowPoint(grip,roll,29+bend,halfHeight);
    GgChronArtPoint nock=ggChronArtBowPoint(grip,roll,29+bend+draw*18/255,draw*13/255);
    if(phase==2)nock.x+=bend/2;
    ggChronArtLine(out,top,nock,3,GG_CHRON_INK);
    ggChronArtLine(out,nock,bottom,3,GG_CHRON_INK);
    ggChronArtLine(out,top,nock,1,GG_CHRON_STEEL_LIGHT);
    ggChronArtLine(out,nock,bottom,1,GG_CHRON_STEEL_LIGHT);
    ggChronArtBowLimb(out,grip,roll,-1,bend);
    ggChronArtBowLimb(out,grip,roll,1,bend);

    if(phase==1){
        GgChronArtPoint tip=ggChronArtAt(grip,12-draw/8,-26-draw/46);
        ggChronArtArrow(out,nock,tip,0);
        ggChronArtBowHand(out,ggChronArtAt(nock,3,2),0);
    }else if(phase==2){
        if(t<120){
            GgChronArtPoint tip=GG_CHRON_ART_PT(198+t/51,124-t/19);
            GgChronArtPoint tail=ggChronArtAt(tip,33-t/5,26-t/6);
            ggChronArtArrow(out,tail,tip,1);
        }
        if(t<150){
            int recoil=ggChronArtEaseOut(t*255/150);
            GgChronArtPoint hand=GG_CHRON_ART_PT(255+recoil*34/255,169+recoil*36/255);
            ggChronArtBowHand(out,hand,1);
        }
    }
    GgChronArtPoint wrist=ggChronArtAt(grip,-11,8);
    GgChronArtPoint elbow=GG_CHRON_ART_PT(121,216);
    ggChronArtRibbon(out,wrist,elbow,22,40,GG_CHRON_INK);
    ggChronArtRibbon(out,wrist,elbow,18,34,GG_CHRON_SLEEVE);
    ggChronArtRibbon(out,ggChronArtAt(wrist,5,3),ggChronArtAt(elbow,9,-2),6,17,GG_CHRON_SLEEVE_LIGHT);
    ggChronArtRibbon(out,wrist,ggChronArtAt(wrist,-10,13),19,23,GG_CHRON_BRASS_DARK);
    ggChronArtLine(out,ggChronArtAt(wrist,-15,9),ggChronArtAt(wrist,4,15),2,GG_CHRON_BRASS);
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,ggChronArtAt(grip,-16,-3),
        ggChronArtAt(grip,-7,-9),ggChronArtAt(grip,7,-8),ggChronArtAt(grip,11,2),
        ggChronArtAt(grip,3,11),ggChronArtAt(grip,-8,12),ggChronArtAt(grip,-17,6));
    GG_CHRON_ART_FACE(out,GG_CHRON_LEATHER,ggChronArtAt(grip,-14,-2),
        ggChronArtAt(grip,-7,-7),ggChronArtAt(grip,5,-6),ggChronArtAt(grip,9,2),
        ggChronArtAt(grip,2,9),ggChronArtAt(grip,-12,8));
    for(int i=-5;i<=4;i+=3){
        ggChronArtLine(out,ggChronArtAt(grip,-8,i),ggChronArtAt(grip,5,i-2),
                       2,GG_CHRON_LEATHER_LIGHT);
    }
}

static GgChronArtPoint ggChronArtPalmPoint(GgChronArtPoint palm,int turn,int x,int y){
    return ggChronArtAt(palm,x-y*turn/80,y+x*turn/80);
}

static void ggChronArtSpellHand(u8 *out,GgChronArtPoint palm,int turn,int spread,
                               int light){
    static const signed char fingers[5][7]={
        {-10,5,-22,1,-27,-8,-6},
        {-10,-5,-17,-18,-21,-29,-7},
        {-3,-8,-8,-23,-9,-37,-2},
        {5,-8,8,-20,9,-33,4},
        {12,-4,18,-13,20,-24,8}
    };
#define GG_CHRON_PALM_PT(x,y) ggChronArtPalmPoint(palm,turn,x,y)
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,GG_CHRON_PALM_PT(5,12),
        GG_CHRON_PALM_PT(18,5),GG_CHRON_PALM_PT(58,45),
        GG_CHRON_PALM_PT(23,49));
    GG_CHRON_ART_FACE(out,GG_CHRON_SLEEVE,GG_CHRON_PALM_PT(8,12),
        GG_CHRON_PALM_PT(18,8),GG_CHRON_PALM_PT(54,45),
        GG_CHRON_PALM_PT(25,45));
    GG_CHRON_ART_FACE(out,GG_CHRON_COAL,GG_CHRON_PALM_PT(7,13),
        GG_CHRON_PALM_PT(18,5),GG_CHRON_PALM_PT(38,24),
        GG_CHRON_PALM_PT(18,37));
    GG_CHRON_ART_FACE(out,GG_CHRON_COPPER,GG_CHRON_PALM_PT(9,13),
        GG_CHRON_PALM_PT(18,8),GG_CHRON_PALM_PT(34,24),
        GG_CHRON_PALM_PT(21,30));
    ggChronArtLine(out,GG_CHRON_PALM_PT(18,27),GG_CHRON_PALM_PT(31,19),2,GG_CHRON_BRASS);
    ggChronArtSpark(out,GG_CHRON_PALM_PT(20,19),3,GG_CHRON_GOLD);
    for(int i=0;i<5;i++){
        const signed char *f=fingers[i];
        int shift=f[6]*spread/255;
        GgChronArtPoint base=GG_CHRON_PALM_PT(f[0],f[1]);
        int curl=(255-spread)*(i==0?4:9+i)/255;
        GgChronArtPoint joint=GG_CHRON_PALM_PT(f[2]+shift/2,f[3]+curl/3);
        GgChronArtPoint tip=GG_CHRON_PALM_PT(f[4]+shift,f[5]+curl);
        int width=i==0?9:i==4?5:8;
        ggChronArtRibbon(out,base,joint,width+3,width+1,GG_CHRON_INK);
        ggChronArtRibbon(out,joint,tip,width+1,width-1,GG_CHRON_INK);
        ggChronArtRibbon(out,base,joint,width,width-1,GG_CHRON_SKIN_DARK);
        ggChronArtRibbon(out,joint,tip,width-1,width-3,GG_CHRON_SKIN);
        ggChronArtLine(out,ggChronArtAt(joint,-1,-1),ggChronArtAt(tip,-1,0),1,
                       light&&i<3?GG_CHRON_FLAME:GG_CHRON_SKIN_LIGHT);
    }
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,GG_CHRON_PALM_PT(-16,-6),
        GG_CHRON_PALM_PT(-2,-12),GG_CHRON_PALM_PT(12,-10),
        GG_CHRON_PALM_PT(18,-2),GG_CHRON_PALM_PT(18,9),
        GG_CHRON_PALM_PT(25,17),GG_CHRON_PALM_PT(10,23),
        GG_CHRON_PALM_PT(-12,9));
    GG_CHRON_ART_FACE(out,GG_CHRON_SKIN,GG_CHRON_PALM_PT(-14,-5),
        GG_CHRON_PALM_PT(-1,-11),GG_CHRON_PALM_PT(11,-8),
        GG_CHRON_PALM_PT(16,-1),GG_CHRON_PALM_PT(16,9),
        GG_CHRON_PALM_PT(21,16),GG_CHRON_PALM_PT(10,19),
        GG_CHRON_PALM_PT(-11,7));
    GG_CHRON_ART_FACE(out,light?GG_CHRON_FLAME:GG_CHRON_SKIN_LIGHT,GG_CHRON_PALM_PT(-11,-4),
        GG_CHRON_PALM_PT(-1,-8),GG_CHRON_PALM_PT(9,-6),
        GG_CHRON_PALM_PT(5,4),GG_CHRON_PALM_PT(-6,7));
    ggChronArtLine(out,GG_CHRON_PALM_PT(-8,4),GG_CHRON_PALM_PT(0,0),1,GG_CHRON_SKIN_DARK);
    ggChronArtLine(out,GG_CHRON_PALM_PT(2,6),GG_CHRON_PALM_PT(11,3),1,GG_CHRON_SKIN_DARK);
#undef GG_CHRON_PALM_PT
}

static void ggChronArtWard(u8 *out,GgChronArtPoint center,int t){
    static const signed char ring[8][2]={
        {0,-22},{19,-15},{28,0},{19,13},{0,19},{-19,13},{-28,0},{-19,-15}
    };
    int size=110+ggChronArtEaseOut(t)*145/255;
    for(int i=0;i<8;i++){
        int next=(i+1)&7;
        GgChronArtPoint a=ggChronArtAt(center,ring[i][0]*size/255,ring[i][1]*size/255);
        GgChronArtPoint b=ggChronArtAt(center,ring[next][0]*size/255,ring[next][1]*size/255);
        ggChronArtLine(out,a,b,3,GG_CHRON_COPPER);
        ggChronArtLine(out,a,b,1,GG_CHRON_FLAME);
        ggChronArtSpark(out,a,i%2?2:4,i%2?GG_CHRON_FIRE:GG_CHRON_GLINT);
    }
}

static void ggChronArtFlame(u8 *out,GgChronArtPoint p,int r,int lean){
    GG_CHRON_ART_FACE(out,GG_CHRON_COPPER,ggChronArtAt(p,-r,-1),
        ggChronArtAt(p,lean-r/3,-r-3),ggChronArtAt(p,lean+r/3,-r/2),
        ggChronArtAt(p,r+2,1),ggChronArtAt(p,r/2,r/2+2),ggChronArtAt(p,-r/2,r/2+1));
    GG_CHRON_ART_FACE(out,GG_CHRON_FIRE,ggChronArtAt(p,-r+2,0),
        ggChronArtAt(p,lean-r/3,-r),ggChronArtAt(p,lean+r/4,-r/2),
        ggChronArtAt(p,r,1),ggChronArtAt(p,r/2,r/2),ggChronArtAt(p,-r/2,r/2));
    GG_CHRON_ART_FACE(out,GG_CHRON_FLAME,ggChronArtAt(p,-r/2,1),
        ggChronArtAt(p,lean/2,-r*3/4),ggChronArtAt(p,r*2/3,0),
        ggChronArtAt(p,r/3,r/2),ggChronArtAt(p,-r/3,r/2));
    ggChronArtSpark(out,p,r/3+1,GG_CHRON_GLINT);
}

static void ggChronArtSpell(u8 *out,int phase,int t,int frame,int bob){
    int reach=0,spread=0,turn=-7,kick=0;
    if(phase==1){
        reach=ggChronArtEaseOut(t)/2+14;
        spread=95+ggChronArtEaseOut(t)*160/255;
        turn=-18;
    }else if(phase==2){
        int travel=t<68?t*255/68:(t-68)*255/187;
        reach=t<68?ggChronArtTween(181,255,travel):ggChronArtTween(255,108,travel);
        spread=t<68?255:ggChronArtTween(255,87,travel);
        turn=t<68?-38:ggChronArtTween(-38,-24,travel);
        kick=t>88?(t-88)/16:0;
    }else if(phase==3){
        reach=57+ggChronArtEaseOut(t)*148/255;
        spread=220;
        turn=-15;
    }else if(phase==4){
        int recover=ggChronArtEase(t);
        reach=ggChronArtTween(108,0,recover);
        spread=ggChronArtTween(87,0,recover);
        turn=ggChronArtTween(-24,-7,recover);
        kick=ggChronArtTween(10,0,recover);
    }
    GgChronArtPoint palm=GG_CHRON_ART_PT(285-reach*62/255,
                                  173-reach*20/255+bob+kick);
    ggChronArtSpellHand(out,palm,turn,spread,
                       (phase==1&&t>=34)||(phase==2&&t<123)||phase==3);
    if(phase==3){
        ggChronArtWard(out,ggChronArtAt(palm,-25,-5),t);
    }else if(phase==2&&t<155){
        GgChronArtPoint source=ggChronArtPalmPoint(palm,turn,-8,-6);
        GgChronArtPoint flame=GG_CHRON_ART_PT(215-t/9,136-t/12);
        int radius=11-t/17;
        GG_CHRON_ART_FACE(out,GG_CHRON_COPPER,ggChronArtAt(source,4,4),
            ggChronArtAt(flame,0,5),ggChronArtAt(flame,-4,-5),
            ggChronArtAt(flame,5,-3),ggChronArtAt(source,-1,-6));
        if(t<82)ggChronArtRibbon(out,source,flame,4,6,GG_CHRON_FLAME);
        ggChronArtFlame(out,flame,radius,-4);
        for(int i=0;i<3;i++){
            GgChronArtPoint mote=ggChronArtAt(flame,(i-1)*10+i+t/14,
                                             5+i*4-t/24);
            ggChronArtSpark(out,mote,2+i%2,i==1?GG_CHRON_GLINT:GG_CHRON_FIRE);
        }
    }else if(phase==1){
        GgChronArtPoint ember=ggChronArtPalmPoint(palm,turn,-13-t/42,-12-t/32);
        int radius=3+t/25;
        ggChronArtFlame(out,ember,radius,((frame/4)&3)-2);
        for(int i=0;i<3;i++){
            int drift=(frame/3+i*8)%22;
            GgChronArtPoint mote=ggChronArtAt(ember,(i-1)*(radius+3),3-drift);
            ggChronArtSpark(out,mote,2,i%2?GG_CHRON_FIRE:GG_CHRON_GLINT);
        }
    }
}

static void ggChronCombatWeaponDraw(u8 *out,int style,int phase,int progression,int frame){
    int t=ggExpClamp(progression,0,255);
    int sway=(frame/6)&7,bob=(sway<4?sway:7-sway)-1;
    if(style==1)ggChronArtBow(out,phase,t,bob);
    else if(style==2)ggChronArtSpell(out,phase,t,frame,bob);
    else ggChronArtSword(out,phase,t,bob);
}

static void ggChronCombatReticle(u8 *out,int style,int target,int progress){
    int ink=target?GG_CHRON_GOLD:GG_CHRON_STEEL_LIGHT;
    int spread=7-ggExpClamp(progress,0,255)*3/255;
    int y=120;
    ggChronArtRect(out,198-spread,y-1,6,3,GG_CHRON_INK);
    ggChronArtRect(out,197+spread,y-1,6,3,GG_CHRON_INK);
    ggChronArtRect(out,199,y-2,3,5,GG_CHRON_INK);
    ggChronArtRect(out,199-spread,y,4,1,ink);
    ggChronArtRect(out,198+spread,y,4,1,ink);
    ggChronArtRect(out,200,y,1,1,ink);
    if(style==1)ggChronArtRect(out,200,y+3,1,3,ink);
    if(style==2)ggChronArtRect(out,200,y-4,1,2,ink);
}

#undef GG_CHRON_ART_FACE
#undef GG_CHRON_ART_PT
#endif
