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
    GG_CHRON_JADE_DARK, GG_CHRON_JADE, GG_CHRON_MINT, GG_CHRON_GLINT
};

static const u8 ggChronArtColor[][3]={
    {27,22,29}, {44,43,58}, {77,74,85},
    {74,43,43}, {129,77,60},
    {104,68,45}, {180,127,65}, {247,199,116},
    {58,73,88}, {138,171,182}, {218,238,236},
    {109,55,41}, {179,96,58}, {234,155,87},
    {120,63,56}, {193,119,85}, {239,168,116},
    {32,73,80}, {69,155,140}, {151,231,183}, {237,252,221}
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

static void ggChronArtSword(u8 *out,int phase,int t,int bob){
    const GgChronArtPose ready={306,171+bob,-41,-93},wound={329,183,48,-97};
    const GgChronArtPose crossing={283,166,-103,-79},end={227,180,-117,-44};
    const GgChronArtPose guard={293,157,-122,-21};
    GgChronArtPose pose=ready;
    switch(phase){
    case 1:pose=ggChronArtPose(ready,wound,ggChronArtEase(t));break;
    case 2:
        pose=t<128?ggChronArtPose(wound,crossing,t*2):
                   ggChronArtPose(crossing,end,(t-128)*2+1);
        break;
    case 3:pose=guard;break;
    case 4:pose=ggChronArtPose(end,ready,ggChronArtEase(t));break;
    }
    if(phase==2&&t>=42&&t<222){
        int x=336-(t-42)*128/180;
        GgChronArtPoint trail=GG_CHRON_ART_PT(x,140+(t-100)*(t-100)/900);
        GG_CHRON_ART_FACE(out,GG_CHRON_BRASS,ggChronArtAt(trail,-43,18),
            ggChronArtAt(trail,-15,-7),ggChronArtAt(trail,13,-13),
            ggChronArtAt(trail,42,-5),ggChronArtAt(trail,11,-2),
            ggChronArtAt(trail,-17,4));
        GG_CHRON_ART_FACE(out,GG_CHRON_GOLD,ggChronArtAt(trail,-38,14),
            ggChronArtAt(trail,-13,-5),ggChronArtAt(trail,13,-10),
            ggChronArtAt(trail,35,-5),ggChronArtAt(trail,9,-5),
            ggChronArtAt(trail,-14,1));
    }
#define GG_CHRON_BLADE_PT(a,s) ggChronArtBladePoint(pose,a,s)
    GgChronArtPoint hand=GG_CHRON_BLADE_PT(-18,0);
    int sleeveX=hand.x+45;
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,ggChronArtAt(hand,-13,1),
        ggChronArtAt(hand,13,-8),GG_CHRON_ART_PT(sleeveX+35,212),
        GG_CHRON_ART_PT(sleeveX-20,218));
    GG_CHRON_ART_FACE(out,GG_CHRON_SLEEVE,ggChronArtAt(hand,-10,2),
        ggChronArtAt(hand,10,-4),GG_CHRON_ART_PT(sleeveX+28,211),
        GG_CHRON_ART_PT(sleeveX-16,213));
    GG_CHRON_ART_FACE(out,GG_CHRON_SLEEVE_LIGHT,ggChronArtAt(hand,7,-3),
        GG_CHRON_ART_PT(sleeveX+27,211),GG_CHRON_ART_PT(sleeveX+9,208));
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
    for(int n=-22;n<=-12;n+=5)
        ggChronArtLine(out,GG_CHRON_BLADE_PT(n,-4),GG_CHRON_BLADE_PT(n+1,4),
                      2,GG_CHRON_BRASS);
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

static void ggChronArtBowLimb(u8 *out,GgChronArtPoint grip,int sign,int bend){
    GgChronArtPoint shoulder=ggChronArtAt(grip,4,sign*16);
    GgChronArtPoint bendPt=ggChronArtAt(grip,14+bend/2,sign*31);
    GgChronArtPoint tip=ggChronArtAt(grip,26+bend,sign*36);
    ggChronArtLine(out,grip,shoulder,9,GG_CHRON_INK);
    ggChronArtLine(out,shoulder,bendPt,8,GG_CHRON_INK);
    ggChronArtLine(out,bendPt,tip,6,GG_CHRON_INK);
    ggChronArtLine(out,grip,shoulder,5,GG_CHRON_YEW_DARK);
    ggChronArtLine(out,shoulder,bendPt,5,GG_CHRON_YEW);
    ggChronArtLine(out,bendPt,tip,3,GG_CHRON_YEW);
    ggChronArtLine(out,ggChronArtAt(grip,-2,0),ggChronArtAt(shoulder,-2,0),2,GG_CHRON_YEW_LIGHT);
    ggChronArtLine(out,ggChronArtAt(shoulder,-2,0),ggChronArtAt(bendPt,-2,0),2,GG_CHRON_YEW_LIGHT);
    ggChronArtLine(out,ggChronArtAt(bendPt,0,sign),ggChronArtAt(tip,0,sign),1,GG_CHRON_GOLD);
    ggChronArtLine(out,tip,ggChronArtAt(tip,3,-sign*3),3,GG_CHRON_BRASS);
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

static void ggChronArtBow(u8 *out,int phase,int t,int bob){
    int draw=0,raise=0;
    switch(phase){
    case 1:draw=raise=ggChronArtEase(t);break;
    case 2:draw=ggExpClamp(255-t*5,0,255);raise=255-t/3;break;
    case 3:draw=raise=210;break;
    case 4:raise=ggChronArtTween(170,0,ggChronArtEase(t));break;
    }
    GgChronArtPoint grip=GG_CHRON_ART_PT(178+raise*34/255,163-raise*6/255+bob);
    int bend=draw/30;
    GgChronArtPoint top=ggChronArtAt(grip,26+bend,-36);
    GgChronArtPoint bottom=ggChronArtAt(grip,26+bend,36);
    GgChronArtPoint nock=ggChronArtAt(grip,26+draw*19/255,draw*18/255);
    if(phase==2&&t>55&&t<142)nock.x+=t&8?2:-2;
    ggChronArtLine(out,top,nock,2,GG_CHRON_INK);
    ggChronArtLine(out,nock,bottom,2,GG_CHRON_INK);
    ggChronArtLine(out,top,nock,1,GG_CHRON_STEEL_LIGHT);
    ggChronArtLine(out,nock,bottom,1,GG_CHRON_STEEL_LIGHT);
    ggChronArtBowLimb(out,grip,-1,bend);
    ggChronArtBowLimb(out,grip,1,bend);
    if(phase==1||phase==3){
        ggChronArtArrow(out,nock,ggChronArtAt(grip,-25,-24-draw/20),0);
    }else if(phase==2&&t<182){
        GgChronArtPoint tip=GG_CHRON_ART_PT(193+t*7/182,125-t*5/182);
        GgChronArtPoint tail=ggChronArtAt(tip,45-t*33/182,32-t*21/182);
        ggChronArtArrow(out,tail,tip,1);
    }
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,GG_CHRON_ART_PT(116,212),
        ggChronArtAt(grip,-13,-7),ggChronArtAt(grip,7,-8),
        ggChronArtAt(grip,12,5),GG_CHRON_ART_PT(grip.x-8,213));
    GG_CHRON_ART_FACE(out,GG_CHRON_SLEEVE,GG_CHRON_ART_PT(121,212),
        ggChronArtAt(grip,-12,2),ggChronArtAt(grip,2,12),
        GG_CHRON_ART_PT(grip.x-12,211));
    GG_CHRON_ART_FACE(out,GG_CHRON_LEATHER,ggChronArtAt(grip,-12,-5),
        ggChronArtAt(grip,5,-5),ggChronArtAt(grip,9,3),
        ggChronArtAt(grip,-1,13),ggChronArtAt(grip,-16,7));
    for(int i=-4;i<=5;i+=3)
        ggChronArtLine(out,ggChronArtAt(grip,-8,i),ggChronArtAt(grip,5,i-2),
                       2,GG_CHRON_LEATHER_LIGHT);
    ggChronArtLine(out,ggChronArtAt(grip,-14,6),ggChronArtAt(grip,-3,13),
                   3,GG_CHRON_BRASS);
    if(phase==1||phase==3||(phase==2&&t<108)){
        GgChronArtPoint hand=ggChronArtAt(nock,phase==2?t/8:0,phase==2?t/10:0);
        GG_CHRON_ART_FACE(out,GG_CHRON_INK,ggChronArtAt(hand,-8,2),
            ggChronArtAt(hand,-4,-5),ggChronArtAt(hand,5,-7),
            ggChronArtAt(hand,14,-2),ggChronArtAt(hand,49,35),
            ggChronArtAt(hand,19,36));
        GG_CHRON_ART_FACE(out,GG_CHRON_LEATHER,ggChronArtAt(hand,-5,2),
            ggChronArtAt(hand,-2,-3),ggChronArtAt(hand,6,-5),
            ggChronArtAt(hand,12,0),ggChronArtAt(hand,17,12),
            ggChronArtAt(hand,4,11));
        ggChronArtLine(out,ggChronArtAt(hand,-2,0),ggChronArtAt(hand,7,-2),2,GG_CHRON_LEATHER_LIGHT);
        ggChronArtLine(out,ggChronArtAt(hand,5,9),ggChronArtAt(hand,13,6),2,GG_CHRON_BRASS);
    }
}

static void ggChronArtSpellHand(u8 *out,GgChronArtPoint palm,int spread){
    static const signed char fingers[5][7]={
        {-10,5,-22,1,-27,-8,-6},
        {-10,-5,-17,-18,-21,-29,-7},
        {-3,-8,-8,-23,-9,-37,-2},
        {5,-8,8,-20,9,-33,4},
        {12,-4,18,-13,20,-24,8}
    };
#define GG_CHRON_PALM_PT(x,y) ggChronArtAt(palm,x,y)
    GG_CHRON_ART_FACE(out,GG_CHRON_INK,GG_CHRON_PALM_PT(5,12),
        GG_CHRON_PALM_PT(18,5),GG_CHRON_PALM_PT(58,45),
        GG_CHRON_PALM_PT(23,49));
    GG_CHRON_ART_FACE(out,GG_CHRON_SLEEVE,GG_CHRON_PALM_PT(8,12),
        GG_CHRON_PALM_PT(18,8),GG_CHRON_PALM_PT(54,45),
        GG_CHRON_PALM_PT(25,45));
    GG_CHRON_ART_FACE(out,GG_CHRON_JADE_DARK,GG_CHRON_PALM_PT(7,13),
        GG_CHRON_PALM_PT(18,5),GG_CHRON_PALM_PT(38,24),
        GG_CHRON_PALM_PT(18,37));
    GG_CHRON_ART_FACE(out,GG_CHRON_JADE,GG_CHRON_PALM_PT(9,13),
        GG_CHRON_PALM_PT(18,8),GG_CHRON_PALM_PT(34,24),
        GG_CHRON_PALM_PT(21,30));
    ggChronArtLine(out,GG_CHRON_PALM_PT(19,31),GG_CHRON_PALM_PT(34,22),2,GG_CHRON_BRASS);
    for(int i=0;i<5;i++){
        const signed char *f=fingers[i];
        int shift=f[6]*spread/255;
        GgChronArtPoint base=GG_CHRON_PALM_PT(f[0],f[1]);
        GgChronArtPoint joint=GG_CHRON_PALM_PT(f[2]+shift/2,f[3]);
        GgChronArtPoint tip=GG_CHRON_PALM_PT(f[4]+shift,f[5]);
        int width=i==0?9:i==4?5:7;
        ggChronArtLine(out,base,joint,width+2,GG_CHRON_INK);
        ggChronArtLine(out,joint,tip,width,GG_CHRON_INK);
        ggChronArtLine(out,base,joint,width,GG_CHRON_SKIN_DARK);
        ggChronArtLine(out,joint,tip,width-2,GG_CHRON_SKIN);
        ggChronArtLine(out,ggChronArtAt(joint,-1,-1),ggChronArtAt(tip,-1,0),
                       1,GG_CHRON_SKIN_LIGHT);
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
    GG_CHRON_ART_FACE(out,GG_CHRON_SKIN_LIGHT,GG_CHRON_PALM_PT(-11,-4),
        GG_CHRON_PALM_PT(-1,-8),GG_CHRON_PALM_PT(9,-6),
        GG_CHRON_PALM_PT(5,4),GG_CHRON_PALM_PT(-6,7));
    ggChronArtLine(out,GG_CHRON_PALM_PT(-8,4),GG_CHRON_PALM_PT(0,0),1,GG_CHRON_SKIN_DARK);
    ggChronArtLine(out,GG_CHRON_PALM_PT(2,6),GG_CHRON_PALM_PT(11,3),1,GG_CHRON_SKIN_DARK);
#undef GG_CHRON_PALM_PT
}

static void ggChronArtWard(u8 *out,GgChronArtPoint center){
    static const signed char ring[8][2]={
        {0,-22},{19,-15},{28,0},{19,13},{0,19},{-19,13},{-28,0},{-19,-15}
    };
    for(int i=0;i<8;i++){
        int next=(i+1)&7;
        GgChronArtPoint a=ggChronArtAt(center,ring[i][0],ring[i][1]);
        GgChronArtPoint b=ggChronArtAt(center,ring[next][0],ring[next][1]);
        ggChronArtLine(out,a,b,4,GG_CHRON_JADE_DARK);
        ggChronArtLine(out,a,b,1,GG_CHRON_MINT);
        ggChronArtSpark(out,a,i%2?2:4,i%2?GG_CHRON_JADE:GG_CHRON_GLINT);
    }
}

static void ggChronArtSpell(u8 *out,int phase,int t,int frame,int bob){
    int reach=0,spread=0;
    switch(phase){
    case 1:spread=ggChronArtEase(t);reach=spread/3;break;
    case 2:spread=255;reach=ggChronArtTween(85,255,ggChronArtEase(ggExpClamp(t*2,0,255)));break;
    case 3:spread=reach=212;break;
    case 4:spread=reach=255-ggChronArtEase(t);break;
    }
    GgChronArtPoint palm=GG_CHRON_ART_PT(281-reach*48/255,170-reach*12/255+bob);
    ggChronArtSpellHand(out,palm,spread);
    if(phase==3){
        ggChronArtWard(out,GG_CHRON_ART_PT(203,146));
    }else if(phase==2&&t<205){
        GgChronArtPoint source=ggChronArtAt(palm,-7,-5);
        GgChronArtPoint flare=GG_CHRON_ART_PT(211-t/18,132-t/20);
        int radius=13-t/18;
        GG_CHRON_ART_FACE(out,GG_CHRON_JADE,ggChronArtAt(source,7,5),
            ggChronArtAt(flare,-radius,0),ggChronArtAt(flare,0,-radius),
            ggChronArtAt(flare,radius,0),ggChronArtAt(source,2,-6));
        ggChronArtLine(out,source,flare,2,GG_CHRON_MINT);
        ggChronArtSpark(out,flare,radius,GG_CHRON_MINT);
        ggChronArtSpark(out,flare,radius/2,GG_CHRON_GLINT);
    }else if(phase==1){
        GgChronArtPoint source=ggChronArtAt(palm,-7,-3);
        GgChronArtPoint ember=ggChronArtAt(source,-16-t/26,-12-t/48);
        int radius=3+t/26;
        ggChronArtLine(out,source,ember,1,GG_CHRON_JADE);
        ggChronArtSpark(out,ember,radius,GG_CHRON_JADE);
        ggChronArtSpark(out,ember,radius/2,GG_CHRON_MINT);
        for(int i=0;i<3;i++){
            int orbit=(frame/3+i*9)%27;
            int x=-13+orbit,y=orbit<14?-6+orbit/4:7-orbit/4;
            ggChronArtSpark(out,ggChronArtAt(ember,x,y+(i-1)*9),2,GG_CHRON_GLINT);
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
