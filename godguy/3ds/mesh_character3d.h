// GodGuy 3DS physical inhabitants: low-poly articulated silhouettes.
// The playable world uses its SAME 3D camera, true world ground and z
// buffer. Animals have closed species-shaped volumes; human citizens use
// the companion procedural cel painter. No atlas or skeletal allocations.
#ifndef GG_MESH_CHARACTER3D_H
#define GG_MESH_CHARACTER3D_H
static void ggMeshActorTube(GgMeshContext*m,
                             int ax,int ay,int az,int br,
                             int bx,int by,int bz,int tr,
                             int r,int g,int b,int sides){
    if(sides<4)sides=4;
    for(int i=0;i<sides;i++){
        int j=(i+1)%sides;
        int p=i*256/sides,q=j*256/sides;
        int cx=ggExpCos[p],cy=ggExpSin[p],
            dx=ggExpCos[q],dy=ggExpSin[q];
        GgMeshVertex a=ggMeshV(ax+cx*br/4096,ay+cy*br/4096,az);
        GgMeshVertex bb=ggMeshV(ax+dx*br/4096,ay+dy*br/4096,az);
        GgMeshVertex c=ggMeshV(bx+dx*tr/4096,by+dy*tr/4096,bz);
        GgMeshVertex d=ggMeshV(bx+cx*tr/4096,by+cy*tr/4096,bz);
        int shade=200+(i&1)*30+(i%3)*8;
        ggMeshQuad(m,a,bb,c,d,
                   ggMeshClamp(r*shade/240,0,255),
                   ggMeshClamp(g*shade/240,0,255),
                   ggMeshClamp(b*shade/240,0,255));
    }
}
static void ggMeshActorHuman(GgMeshContext*m,int x,int y,
                              int ground,int heading,int phase,
                              int family,const u8 base[3][3],
                              int quality){
    int dr=ggMeshClamp(base[0][0]*8,20,220),
        dg=ggMeshClamp(base[0][1]*8,30,220),
        db=ggMeshClamp(base[0][2]*8,24,210);
    int hr=ggMeshClamp(base[1][0]*8,24,185),
        hg=ggMeshClamp(base[1][1]*8,22,165),
        hb=ggMeshClamp(base[1][2]*8,20,155);
    int sr=ggMeshClamp(base[2][0]*8,108,219),
        sg=ggMeshClamp(base[2][1]*8,88,186),
        sb=ggMeshClamp(base[2][2]*8,68,166);
    int fx=ggExpCos[heading&255],fy=ggExpSin[heading&255];
    int rx=-fy,ry=fx;
    int swing=ggExpSin[phase&255]/360;
    int shoulder=family%5==0?21:family%5==1?18:16;
    int h0=ground+8,h1=ground+16;
    int save=m->material;
    m->material=GG_MAT_FABRIC;
    // Adult shoulder / waist taper: bounded 8-sided physical torso.
    ggMeshActorTube(m,x,y,h0,11,x,y,h1,shoulder,dr,dg,db,
                    quality>=2?8:6);
    // A second coat hem wraps around a grounded hip volume.
    ggMeshActorTube(m,x,y,ground+7,14,x,y,ground+9,12,
                    dr*3/4,dg*3/4,db*3/4,6);
    for(int side=-1;side<=1;side+=2){
        int sx=x+rx*side*9/4096,sy=y+ry*side*9/4096;
        int forwardStep=side*swing;
        int footX=sx+fx*forwardStep/4096;
        int footY=sy+fy*forwardStep/4096;
        int footZ=ggMeshGroundAtQ8(footX,footY);
        int kneeX=(sx+footX)/2+fx*forwardStep/8192;
        int kneeY=(sy+footY)/2+fy*forwardStep/8192;
        ggMeshActorTube(m,sx,sy,ground+8,6,
                        kneeX,kneeY,ground+4,5,
                        dr*4/5,dg*4/5,db*4/5,6);
        ggMeshActorTube(m,kneeX,kneeY,ground+4,5,
                        footX,footY,footZ+2,5,68,68,75,6);
        // Truly world-planted boot, never an unrelated floating cel.
        ggMeshGroundBox(m,footX-9,footY-6,footX+9,footY+9,
                        footZ+2,52,55,63);
        int armX=x+rx*side*shoulder/4096,
            armY=y+ry*side*shoulder/4096;
        int elbowX=armX+rx*side*8/4096+
                    fx*(-side*swing)/8192;
        int elbowY=armY+ry*side*8/4096+
                    fy*(-side*swing)/8192;
        int wristX=elbowX+fx*(-side*swing)/8192,
            wristY=elbowY+fy*(-side*swing)/8192;
        ggMeshActorTube(m,armX,armY,ground+15,5,
                        elbowX,elbowY,ground+11,4,
                        dr,dg,db,6);
        ggMeshActorTube(m,elbowX,elbowY,ground+11,4,
                        wristX,wristY,ground+8+side*swing/9,3,
                        dr*4/5,dg*4/5,db*4/5,6);
        ggMeshBox(m,wristX-3,wristY-3,wristX+3,wristY+3,
                  ground+7+side*swing/9,
                  ground+10+side*swing/9,sr,sg,sb);
    }
    // Physical neck, cheek volume, tapered jaw and cranial planes.
    m->material=GG_MAT_NONE;
    ggMeshActorTube(m,x,y,ground+16,6,x,y,ground+19,6,
                    sr*4/5,sg*4/5,sb*4/5,6);
    ggMeshActorTube(m,x,y,ground+18,13,
                    x,y,ground+22,17,sr,sg,sb,8);
    ggMeshActorTube(m,x,y,ground+22,17,
                    x,y,ground+24,12,
                    sr*19/20,sg*19/20,sb*19/20,8);
    // Hairstyles are 3D crowns with varying physical silhouettes.
    ggMeshActorTube(m,x,y,ground+23,17,
                    x,y,ground+25+(family&1),12,
                    hr,hg,hb,quality>=2?8:6);
    int forwardX=x+fx*18/4096,
        forwardY=y+fy*18/4096;
    // A projecting nose and two separated eyes remain attached to
    // the head during real yaw turns, not to a screen-facing bitmap.
    ggMeshActorTube(m,forwardX,forwardY,ground+20,3,
                    forwardX+fx*5/4096,
                    forwardY+fy*5/4096,ground+20,1,
                    ggMeshClamp(sr+12,0,255),sg,sb,4);
    if(quality>=2){
        for(int side=-1;side<=1;side+=2){
            int ex=forwardX+rx*side*7/4096,
                ey=forwardY+ry*side*7/4096;
            ggMeshBox(m,ex-2,ey-2,ex+2,ey+2,ground+21,ground+22,
                      34,38,41);
        }
    }
    m->material=save;
}
// Creature dimensions are local forward/across XY plus actual world Z.
typedef struct {int x,y,z,fx,fy;} GgMeshCreaturePose;
static GgMeshVertex ggMeshCreatureV(const GgMeshCreaturePose*p,
                                   int along,int across,int height){
    return ggMeshV(p->x+(p->fx*along-p->fy*across)/4096,
                   p->y+(p->fy*along+p->fx*across)/4096,p->z+height);
}
static int ggMeshCreatureFoot(const GgMeshCreaturePose*p,
                              int along,int across){
    GgMeshVertex v=ggMeshCreatureV(p,along,across,0);
    return ggMeshGroundAtQ8(v.x,v.y)-p->z-1;
}
typedef struct {int along,across,height,width,rise;} GgMeshCreatureSection;
static GgMeshVertex ggMeshCreatureSectionPoint(const GgMeshCreaturePose*p,
                                               GgMeshCreatureSection s,int angle){
    return ggMeshCreatureV(p,s.along,s.across+ggExpCos[angle]*s.width/4096,
                           s.height+ggExpSin[angle]*s.rise/4096);
}
// Tapered torso sections share their edges and close to a point at the
// head and tail. The palette keeps broad back, flank and belly planes.
static void ggMeshCreatureSections(GgMeshContext*m,const GgMeshCreaturePose*p,
                                    const GgMeshCreatureSection*section,
                                    int count,int sides,int r,int g,int b){
    for(int i=1;i<count;i++){
        GgMeshCreatureSection a=section[i-1],c=section[i];
        for(int n=0;n<sides;n++){
            int t=n*256/sides,u=((n+1)*256/sides)&255;
            int mid=(t+128/sides)&255;
            int shade=217+ggExpSin[mid]/120+ggExpCos[mid]/440;
            int rr=r*shade/256,gg=g*shade/256,bb=b*shade/256;
            GgMeshVertex a0=ggMeshCreatureSectionPoint(p,a,t);
            GgMeshVertex c0=ggMeshCreatureSectionPoint(p,c,t);
            if(!a.width){
                ggMeshTriangle(m,a0,ggMeshCreatureSectionPoint(p,c,u),
                                c0,rr,gg,bb);
            }else if(!c.width){
                ggMeshTriangle(m,a0,ggMeshCreatureSectionPoint(p,a,u),
                                c0,rr,gg,bb);
            }else{
                ggMeshQuad(m,a0,ggMeshCreatureSectionPoint(p,a,u),
                            ggMeshCreatureSectionPoint(p,c,u),c0,rr,gg,bb);
            }
        }
    }
}
static void ggMeshCreatureHull(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int along,int across,int height,
                               int length,int width,int rise,
                               int r,int g,int b,int sides){
    GgMeshCreatureSection section[]={
        {along-length,across,height,0,0},
        {along-length/2,across,height,width,rise},
        {along+length/2,across,height,width,rise},
        {along+length,across,height,0,0}
    };
    ggMeshCreatureSections(m,p,section,4,sides,r,g,b);
}
typedef struct {int along,across,height,radius;} GgMeshCreatureJoint;
static int ggMeshCreatureLength(int x,int y){
    x=abs(x);y=abs(y);
    return x>y?x+y/2:y+x/2;
}
static void ggMeshCreatureJointRing(const GgMeshCreaturePose*p,
                                    GgMeshCreatureJoint joint,
                                    GgMeshCreatureJoint from,
                                    GgMeshCreatureJoint to,GgMeshVertex ring[4]){
    int a=to.along-from.along,u=to.across-from.across;
    // World height units project roughly seven times wider than XY units.
    int rise=(to.height-from.height)*7;
    int run=ggMeshCreatureLength(a,u);
    int sa=0,su=joint.radius,ta=joint.radius,tu=0,h=0;
    if(run){
        int span=ggMeshCreatureLength(run,rise);
        sa=-u*joint.radius/run;
        su=a*joint.radius/run;
        ta=-a*rise*joint.radius/(run*span);
        tu=-u*rise*joint.radius/(run*span);
        h=(joint.radius*run+span*3)/(span*7);
        if(!h&&run>=abs(rise))h=1;
    }
    ring[0]=ggMeshCreatureV(p,joint.along+sa,joint.across+su,joint.height);
    ring[1]=ggMeshCreatureV(p,joint.along+ta,joint.across+tu,joint.height+h);
    ring[2]=ggMeshCreatureV(p,joint.along-sa,joint.across-su,joint.height);
    ring[3]=ggMeshCreatureV(p,joint.along-ta,joint.across-tu,joint.height-h);
}
// Each bend has one shared ring. Four-sided bones keep a visible elbow,
// hock or knee without overlapping open tubes, even at a horizontal joint.
static void ggMeshCreatureChain(GgMeshContext*m,const GgMeshCreaturePose*p,
                                 const GgMeshCreatureJoint*joint,int count,
                                 int r,int g,int b){
    GgMeshVertex rear[4],front[4];
    ggMeshCreatureJointRing(p,joint[0],joint[0],joint[1],rear);
    ggMeshQuad(m,rear[3],rear[2],rear[1],rear[0],r*4/5,g*4/5,b*4/5);
    for(int i=1;i<count;i++){
        ggMeshCreatureJointRing(p,joint[i],joint[i-1],
                                joint[i+1<count?i+1:i],front);
        for(int n=0;n<4;n++){
            int next=(n+1)&3;
            int shade=n==0?251:n==1?229:n==2?191:210;
            ggMeshQuad(m,rear[n],rear[next],front[next],front[n],
                        r*shade/256,g*shade/256,b*shade/256);
        }
        for(int n=0;n<4;n++)rear[n]=front[n];
    }
    ggMeshQuad(m,rear[0],rear[1],rear[2],rear[3],r,g,b);
}
static void ggMeshCreatureLimb(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int a,int u,int h,int b,int v,int z,
                               int thick,int taper,int r,int g,int blue){
    GgMeshCreatureJoint joint[]={{a,u,h,thick},{b,v,z,taper}};
    ggMeshCreatureChain(m,p,joint,2,r,g,blue);
}
// Wing and flipper membranes have a raised ridge and a closed underside.
// World-space edge points let feathers, membranes and flukes use the
// same few triangles without sharing their silhouettes.
static void ggMeshCreatureMembrane(GgMeshContext*m,const GgMeshVertex edge[4],
                                   GgMeshVertex top,GgMeshVertex bottom,
                                   int r,int g,int b){
    for(int n=0;n<4;n++){
        int next=(n+1)&3;
        int shade=n<2?256:226;
        ggMeshTriangle(m,edge[n],edge[next],top,r*shade/256,g*shade/256,b*shade/256);
        ggMeshTriangle(m,edge[next],edge[n],bottom,r*3/4,g*3/4,b*3/4);
    }
}
static void ggMeshCreatureFin(GgMeshContext*m,const GgMeshCreaturePose*p,
                              const GgMeshVertex edge[4],int along,int across,
                              int height,int thickness,int r,int g,int b){
    GgMeshVertex top=ggMeshCreatureV(p,along,across,height+thickness);
    GgMeshVertex bottom=ggMeshCreatureV(p,along,across,height-thickness);
    ggMeshCreatureMembrane(m,edge,top,bottom,r,g,b);
}
static void ggMeshCreatureEye(GgMeshContext*m,const GgMeshCreaturePose*p,
                              int along,int across,int height){
    int out=across<0?-2:2;
    GgMeshVertex a=ggMeshCreatureV(p,along-2,across,height);
    GgMeshVertex b=ggMeshCreatureV(p,along+2,across,height);
    GgMeshVertex c=ggMeshCreatureV(p,along,across+out,height);
    GgMeshVertex cap=ggMeshCreatureV(p,along,across+out/2,height+1);
    ggMeshTriangle(m,a,b,c,27,32,37);
    ggMeshTriangle(m,a,cap,b,27,32,37);
    ggMeshTriangle(m,b,cap,c,34,40,45);
    ggMeshTriangle(m,c,cap,a,34,40,45);
}
// Classification follows the authored sprite SHAPE, never its random
// family color. A red fish is still a fish and an ant has six legs.
enum {GG_CREATURE_QUADRUPED,GG_CREATURE_BIRD,GG_CREATURE_FISH,
      GG_CREATURE_CRAWLER,GG_CREATURE_SERPENT,GG_CREATURE_BLOB,
      GG_CREATURE_WINGED,GG_CREATURE_ORIGINAL};
static int ggMeshCreatureKind(int shape){
    switch(shape){
    case 1:case 14:case 51:case 57:case 60:return GG_CREATURE_BIRD;
    case 2:case 13:case 61:case 62:return GG_CREATURE_FISH;
    case 4:case 12:case 37:case 50:case 64:case 76:
        return GG_CREATURE_CRAWLER;
    case 3:case 77:return GG_CREATURE_SERPENT;
    case 6:case 17:case 47:case 48:case 72:case 73:case 75:
        return GG_CREATURE_BLOB;
    case 53:case 63:case 65:case 66:return GG_CREATURE_WINGED;
    case 0:case 7:case 8:case 9:case 10:case 15:case 20:case 21:
    case 22:case 23:case 52:case 55:case 56:case 58:case 59:case 67:
        return GG_CREATURE_QUADRUPED;
    // Monsters, dragons, mages and whimsical special creatures keep
    // their actual authored silhouette until each has its own model.
    default:return GG_CREATURE_ORIGINAL;
    }
}
static void ggMeshCreatureQuadruped(GgMeshContext*m,
                                    const GgMeshCreaturePose*p,
                                    int shape,int phase,int quality,
                                    int r,int g,int b){
    int rabbit=shape==10,shell=shape==15||shape==59;
    int low=shape==7||shell||shape==52;
    int hoof=shape==9||shape==23||shape==58||shape==67;
    int heavy=shape==8||shape==21||shape==22||shape==23;
    int cat=shape==55,canine=shape==20||shape==56;
    int length=rabbit?20:low?25:heavy?42:33;
    int width=rabbit?13:heavy?20:low?16:14;
    int height=low?3:rabbit?5:hoof?12:cat?7:9;
    int depth=low?2:rabbit?3:heavy?5:4;
    int sides=quality>=2?8:6;
    GgMeshCreatureSection torso[]={
        {-length,0,height,0,0},
        {-length*2/3,0,height,rabbit?width:width-2,depth},
        {0,0,height-(hoof||canine),width-(canine?4:0),depth-1},
        {length*2/3,0,height+canine,width,depth},
        {length,0,height+canine,0,0}
    };
    ggMeshCreatureSections(m,p,torso,5,sides,r,g,b);
    for(int front=-1;front<=1;front+=2)
      for(int side=-1;side<=1;side+=2){
        int gait=ggExpSin[(phase+(front==side?128:0))&255]/(hoof?580:750);
        int hip=front*(length-10);
        int across=side*(width-3+(rabbit&&front<0?2:0));
        int knee=hip+(front<0?6:-3)-gait/2;
        int toe=hip+gait+(rabbit&&front<0?5:2);
        int foot=ggMeshCreatureFoot(p,toe,across);
        int ankle=toe-(front<0?4:2);
        int thigh=low?4:hoof?5:front<0?8:6;
        GgMeshCreatureJoint leg[]={
            {hip,across-side*2,height,thigh},
            {knee,across,(height+foot+2)/2,thigh-2},
            {ankle,across,foot+2,3},
            {toe,across,foot+1,hoof?3:4}
        };
        if(hoof)leg[2]=leg[3];
        ggMeshCreatureChain(m,p,leg,hoof?3:4,r*3/4,g*3/4,b*3/4);
      }

    int head=length+(low||rabbit?3:canine?8:6);
    int headZ=height+(low?1:rabbit?2:hoof?5:canine?3:2);
    int headWidth=rabbit?8:hoof?9:heavy?12:low?8:10;
    int muzzle=canine?18:hoof?13:shape==21?14:low?9:7;
    ggMeshCreatureLimb(m,p,length-11,0,height,head-3,0,headZ,
                       hoof?14:headWidth+3,headWidth-1,r*9/10,g*9/10,b*9/10);
    GgMeshCreatureSection skull[]={
        {head-10,0,headZ,0,0},
        {head-5,0,headZ,headWidth,low?2:3},
        {head+4,0,headZ,headWidth-1,low?2:3},
        {head+muzzle-3,0,headZ-(hoof?2:1),headWidth/2,1},
        {head+muzzle,0,headZ-(hoof?2:1),0,0}
    };
    ggMeshCreatureSections(m,p,skull,5,quality>=2?6:4,r,g,b);
    ggMeshCreatureHull(m,p,head+muzzle-2,0,headZ-(hoof?2:1),
                       4,headWidth/2,1,r*3/5,g*3/5,b*3/5,4);
    for(int side=-1;side<=1;side+=2){
        ggMeshCreatureEye(m,p,head+3,side*(headWidth-1),headZ+1);
        if(!low){
            int ear=rabbit?7:canine||cat?4:2;
            ggMeshCreatureLimb(m,p,head-4,side*(headWidth-4),headZ+1,
                               head-8,side*(headWidth+2),headZ+ear,
                               rabbit?5:4,heavy?3:1,r*4/5,g*4/5,b*4/5);
        }
        if(shape==9||shape==58){
            GgMeshCreatureJoint horn[]={
                {head-5,side*5,headZ+2,3},
                {head-8,side*10,headZ+5,2},
                {head-14,side*15,headZ+8,1}
            };
            ggMeshCreatureChain(m,p,horn,3,171,150,113);
            if(shape==9&&quality>=2)
                ggMeshCreatureLimb(m,p,head-8,side*10,headZ+5,
                                   head-4,side*14,headZ+8,2,1,178,157,120);
        }
    }
    if(shape==67)
        ggMeshCreatureLimb(m,p,head+3,0,headZ+2,
                           head+10,0,headZ+8,3,1,221,207,159);
    if(shell){
        // Lower plastron and a flattened, overhanging carapace rim.
        GgMeshCreatureSection carapace[]={
            {-length-1,0,height,0,0},
            {-length*2/3,0,height+1,width+1,4},
            {length/2,0,height+1,width+3,5},
            {length+1,0,height,0,0}
        };
        ggMeshCreatureSections(m,p,carapace,4,sides,r*3/4,g*4/5,b*3/4);
    }
    int tail=ggExpSin[(phase+40)&255]/620;
    if(rabbit)
        ggMeshCreatureHull(m,p,-length+1,0,height+1,6,5,1,
                           r*9/10,g*9/10,b*9/10,4);
    else if(shape!=52&&shape!=8){
        GgMeshCreatureJoint tailBones[]={
            {-length+5,0,height,4},
            {-length-6,tail/2,height+(cat?2:0),3},
            {-length-(shell?10:canine?22:18),tail,height+(cat?6:-2),1}
        };
        ggMeshCreatureChain(m,p,tailBones,3,r*4/5,g*4/5,b*4/5);
    }
}
static void ggMeshCreatureBat(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int phase,int quality,int r,int g,int b){
    int height=13+ggExpSin[phase&255]/1500;
    int flap=ggExpSin[phase&255]/580;
    ggMeshCreatureHull(m,p,-2,0,height,18,9,3,r,g,b,quality>=2?8:6);
    ggMeshCreatureHull(m,p,15,0,height+1,9,7,2,r*9/10,g*9/10,b*9/10,4);
    for(int side=-1;side<=1;side+=2){
        GgMeshVertex shoulder=ggMeshCreatureV(p,5,side*7,height+1);
        GgMeshVertex wrist=ggMeshCreatureV(p,9,side*21,height+2+flap/2);
        GgMeshVertex finger=ggMeshCreatureV(p,0,side*40,height+flap);
        GgMeshVertex knee=ggMeshCreatureV(p,-15,side*17,height-1+flap/2);
        GgMeshVertex inner[]={shoulder,wrist,knee,
                             ggMeshCreatureV(p,-18,side*4,height-1)};
        GgMeshVertex outer[]={wrist,finger,
                             ggMeshCreatureV(p,-14,side*28,height-2+flap),knee};
        ggMeshCreatureFin(m,p,inner,-1,side*14,height+flap/3,1,r,g,b);
        ggMeshCreatureFin(m,p,outer,-3,side*26,height+flap*3/4,2,
                          r*4/5,g*4/5,b*4/5);
        ggMeshCreatureLimb(m,p,11,side*5,height+2,
                           10,side*9,height+6,4,1,r,g,b);
        ggMeshCreatureEye(m,p,18,side*6,height+2);
    }
}
static void ggMeshCreatureBird(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int shape,int phase,int quality,
                               int r,int g,int b){
    if(shape==14){
        ggMeshCreatureBat(m,p,phase,quality,r,g,b);
        return;
    }
    int tall=shape==60,penguin=shape==51,chicken=shape==57;
    int center=tall?11:penguin?7:5;
    int headZ=tall?22:penguin?13:chicken?10:8;
    int head=tall?17:penguin?8:16;
    GgMeshCreatureSection chest[]={
        {-24,0,center+1,0,0},
        {-13,0,center,8,penguin?5:3},
        {3,0,center,12,penguin?6:4},
        {14,0,center+1,8,3},
        {20,0,center+2,0,0}
    };
    ggMeshCreatureSections(m,p,chest,5,quality>=2?6:4,r,g,b);
    GgMeshCreatureJoint neck[]={
        {6,0,center+1,penguin?9:8},
        {head-5,0,(center+headZ)/2,tall?4:6},
        {head,0,headZ-1,5}
    };
    ggMeshCreatureChain(m,p,neck,3,r,g,b);
    ggMeshCreatureHull(m,p,head,0,headZ,9,7,2,r,g,b,quality>=2?6:4);
    ggMeshCreatureHull(m,p,head+9,0,headZ-1,chicken?6:8,4,1,207,163,80,4);
    if(chicken)
        ggMeshCreatureLimb(m,p,head-3,0,headZ+1,
                           head-2,0,headZ+4,5,1,169,74,60);
    for(int side=-1;side<=1;side+=2){
        int toe=ggExpSin[(phase+side*64)&255]/(tall?700:1300);
        int across=side*(tall?8:6);
        int foot=ggMeshCreatureFoot(p,toe+5,across);
        GgMeshCreatureJoint leg[]={
            {-2,across,center-1,tall?4:3},
            {toe-4,across,(center+foot)/2,2},
            {toe,across,foot+2,2},
            {toe+5,across,foot+1,3}
        };
        ggMeshCreatureChain(m,p,leg,4,168,133,77);
        ggMeshCreatureEye(m,p,head+3,side*6,headZ+1);

        int flap=ggExpSin[phase&255]/(tall||penguin?2600:1400);
        int span=penguin?17:tall?18:23;
        GgMeshVertex feathers[]={
            ggMeshCreatureV(p,7,side*8,center+2),
            ggMeshCreatureV(p,-5,side*(span-3),center+1+flap),
            ggMeshCreatureV(p,-22,side*span,center+flap),
            ggMeshCreatureV(p,-23,side*7,center)};
        ggMeshCreatureFin(m,p,feathers,-8,side*(span-7),center+1+flap,1,
                          r*4/5,g*4/5,b*4/5);
    }
    if(!penguin){
        GgMeshVertex tail[]={
            ggMeshCreatureV(p,-13,-4,center),
            ggMeshCreatureV(p,-32,-8,center+(chicken?4:1)),
            ggMeshCreatureV(p,-32,8,center+(chicken?4:1)),
            ggMeshCreatureV(p,-13,4,center)};
        ggMeshCreatureFin(m,p,tail,-23,0,center+(chicken?3:1),1,
                          r*4/5,g*4/5,b*4/5);
    }
}
static void ggMeshCreatureFish(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int shape,int phase,int quality,int water,
                               int r,int g,int b){
    GgMeshCreaturePose swimmer=*p;
    swimmer.z=water?ggMeshGroundCenter(T_WATER)-3:p->z-1;
    p=&swimmer;
    int seal=shape==61,whale=shape==13,piranha=shape==62;
    int length=whale?55:seal?34:30;
    int width=whale?19:seal?13:10;
    int height=whale?5:4;
    int tail=ggExpSin[phase&255]/520;
    int tailZ=height+(whale||seal?ggExpSin[phase&255]/1500:0);
    GgMeshCreatureSection hull[]={
        {-length-3,tail,tailZ,0,0},
        {-length,tail,tailZ,3,1},
        {-length*2/3,tail/2,height,width/2,2},
        {-length/4,0,height,width,whale?4:3},
        {length/3,0,height,width,whale?4:piranha?4:3},
        {length*3/4,0,height,piranha?width:width/2,2},
        {length,0,height-1,0,0}
    };
    ggMeshCreatureSections(m,p,hull,7,quality>=2?8:6,r,g,b);
    if(seal||whale){
        for(int side=-1;side<=1;side+=2){
            int span=whale?23:13;
            GgMeshVertex fluke[]={
                ggMeshCreatureV(p,-length+3,tail,tailZ),
                ggMeshCreatureV(p,-length-8,tail+side*span,tailZ),
                ggMeshCreatureV(p,-length-19,tail+side*(span-4),tailZ-1),
                ggMeshCreatureV(p,-length-13,tail,tailZ)};
            ggMeshCreatureFin(m,p,fluke,-length-9,tail+side*(span/2),tailZ,1,
                              r*4/5,g*4/5,b*4/5);
        }
    }else{
        // Bony fish have a vertical fork and lateral thickness.
        GgMeshVertex fluke[]={
            ggMeshCreatureV(p,-length+4,tail,tailZ),
            ggMeshCreatureV(p,-length-15,tail*2,tailZ+4),
            ggMeshCreatureV(p,-length-5,tail,tailZ),
            ggMeshCreatureV(p,-length-15,tail*2,tailZ-4)};
        GgMeshVertex right=ggMeshCreatureV(p,-length-3,tail+3,tailZ);
        GgMeshVertex left=ggMeshCreatureV(p,-length-3,tail-3,tailZ);
        ggMeshCreatureMembrane(m,fluke,right,left,r*4/5,g*4/5,b*4/5);
        ggMeshCreatureLimb(m,p,8,0,height+2,-11,0,height+6,5,1,
                           r*4/5,g*4/5,b*4/5);
    }
    for(int side=-1;side<=1;side+=2){
        GgMeshVertex flipper[]={
            ggMeshCreatureV(p,8,side*(width-2),height),
            ggMeshCreatureV(p,-7,side*(width+8),height-1),
            ggMeshCreatureV(p,-18,side*(width+9),height-2),
            ggMeshCreatureV(p,-13,side*(width-2),height-1)};
        ggMeshCreatureFin(m,p,flipper,-7,side*(width+2),height-1,1,
                          r*4/5,g*4/5,b*4/5);
        ggMeshCreatureEye(m,p,length/3+4,side*(width-1),height+1);
    }
}
static void ggMeshCreatureCrawler(GgMeshContext*m,const GgMeshCreaturePose*p,
                                  int shape,int phase,int quality,
                                  int r,int g,int b){
    int crab=shape==50,scorpion=shape==37,spider=shape==12;
    int legs=crab||scorpion||spider?4:3;
    int body=crab?18:spider?16:12;
    int span=crab?36:spider?29:26;
    ggMeshCreatureHull(m,p,-10,0,4,crab?17:20,body,crab?3:4,
                       r,g,b,quality>=2?8:6);
    ggMeshCreatureHull(m,p,11,0,4,crab?13:12,crab?15:9,2,
                       r*4/5,g*4/5,b*4/5,4);
    for(int side=-1;side<=1;side+=2){
        for(int leg=0;leg<legs;leg++){
            int along=13-leg*8;
            int reach=leg<legs/2?1:-1;
            int twitch=ggExpSin[(phase+leg*53+side*40)&255]/1100;
            int knee=along+reach*(leg==0||leg==legs-1?9:3);
            int toe=knee+reach*6+twitch;
            int toeAcross=side*(span+(leg==0||leg==legs-1?-3:4));
            int foot=ggMeshCreatureFoot(p,toe,toeAcross);
            GgMeshCreatureJoint limb[]={
                {along,side*(body-4),3,3},
                {knee,toeAcross-side*8,spider?7:5,2},
                {toe,toeAcross,foot+1,1}
            };
            ggMeshCreatureChain(m,p,limb,3,r*3/5,g*3/5,b*3/5);
        }
        ggMeshCreatureEye(m,p,16,side*(crab?12:7),5);
        if(crab||scorpion){
            int tip=crab?28:36,across=side*(crab?24:18);
            ggMeshCreatureLimb(m,p,15,side*8,4,tip-6,across,6,
                               4,3,r,g,b);
            ggMeshCreatureHull(m,p,tip,across,6,9,crab?6:5,2,
                               r*9/10,g*9/10,b*9/10,4);
        }else if(!spider){
            ggMeshCreatureLimb(m,p,19,side*4,5,32,side*10,8,
                               2,1,r*4/5,g*4/5,b*4/5);
        }
    }
    if(scorpion){
        GgMeshCreatureJoint sting[]={
            {-21,0,5,5},{-35,0,8,4},{-32,0,13,3},{-23,0,12,1}
        };
        ggMeshCreatureChain(m,p,sting,4,r,g,b);
    }
}
static void ggMeshCreatureSerpent(GgMeshContext*m,const GgMeshCreaturePose*p,
                                  int shape,int phase,int quality,
                                  int r,int g,int b){
    GgMeshCreatureSection spine[9];
    int worm=shape==77;
    for(int i=0;i<8;i++){
        int along=(i-4)*(worm?9:12);
        int wave=ggExpSin[(phase+(7-i)*34)&255]/(worm?720:350);
        int rise=i<2?1:2;
        int belly=ggMeshCreatureFoot(p,along,wave)+1;
        int wide=i==0?0:i<4?4+i:8;
        spine[i]=(GgMeshCreatureSection){along,wave,belly+rise+(i>=6?i-5:0),
                                         wide,rise};
    }
    GgMeshCreatureSection last=spine[7];
    spine[8]=(GgMeshCreatureSection){last.along+8,last.across,last.height-1,0,0};
    ggMeshCreatureSections(m,p,spine,9,quality>=2?8:6,r,g,b);
    for(int side=-1;side<=1;side+=2)
        ggMeshCreatureEye(m,p,last.along+2,last.across+side*7,last.height+1);
}
static void ggMeshCreatureWinged(GgMeshContext*m,const GgMeshCreaturePose*p,
                                 int shape,int phase,int r,int g,int b){
    int butterfly=shape==63,bee=shape==53;
    int height=10+ggExpSin[phase&255]/1300;
    int flap=ggExpSin[(phase*3)&255]/780;
    int span=butterfly?30:19;
    ggMeshCreatureHull(m,p,-5,0,height,17,bee?9:6,bee?3:2,r,g,b,6);
    ggMeshCreatureHull(m,p,10,0,height,8,5,2,r*3/4,g*3/4,b*3/4,4);
    if(bee)
        ggMeshCreatureHull(m,p,-11,0,height,6,9,3,98,74,40,6);
    for(int side=-1;side<=1;side+=2){
        GgMeshVertex base=ggMeshCreatureV(p,1,side*4,height);
        GgMeshVertex tip=ggMeshCreatureV(p,12,side*span,height+flap);
        GgMeshVertex turn=ggMeshCreatureV(p,-7,side*(span-4),height+flap);
        GgMeshVertex wing[]={base,ggMeshCreatureV(p,13,side*12,height+flap/2+1),
                             tip,turn};
        int wr=butterfly?r:189,wg=butterfly?g:204,wb=butterfly?b:207;
        ggMeshCreatureFin(m,p,wing,6,side*(span-10),height+flap*2/3,1,wr,wg,wb);
        if(butterfly){
            GgMeshVertex hind[]={base,turn,
                ggMeshCreatureV(p,-19,side*(span-7),height+flap-1),
                ggMeshCreatureV(p,-19,side*7,height)};
            ggMeshCreatureFin(m,p,hind,-11,side*(span-12),height+flap/2,1,
                              r*4/5,g*4/5,b*4/5);
        }
        ggMeshCreatureEye(m,p,12,side*4,height+1);
        ggMeshCreatureLimb(m,p,15,side*2,height,23,side*6,height+3,
                           1,1,58,54,51);
    }
}
static void ggMeshCreatureBlob(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int shape,int phase,int quality,int r,int g,int b){
    int ghost=shape==17||shape==48;
    int breath=ggExpSin[phase&255]/2600;
    int height=ghost?9+breath:3+breath;
    int width=ghost?16:24-breath;
    GgMeshCreatureSection mass[]={
        {-27,0,height-2,0,0},
        {-16,0,height,width-3,ghost?4:3},
        {0,0,height,width,ghost?6:4+breath},
        {16,0,height,width-2,ghost?4:3},
        {25,0,height-1,0,0}
    };
    ggMeshCreatureSections(m,p,mass,5,quality>=2?8:6,r,g,b);
    for(int side=-1;side<=1;side+=2)
        ggMeshCreatureEye(m,p,14,side*(width-5),height+2);
}
static void ggMeshActorAnimal(GgMeshContext*m,int x,int y,
                               int ground,int heading,int phase,
                               const u8 base[3][3],int quality){
    GgMeshCreaturePose p={x,y,ground,ggExpCos[heading&255],
                                    ggExpSin[heading&255]};
    int r=ggMeshClamp(base[0][0]*7+16,44,226);
    int g=ggMeshClamp(base[0][1]*7+14,48,223);
    int b=ggMeshClamp(base[0][2]*7+12,38,218);
    ggMeshCreatureQuadruped(m,&p,0,phase,quality,r,g,b);
}
static void ggMeshActorSpecies(GgMeshContext*m,int x,int y,int ground,
                               int heading,int phase,int family,int shape,
                               const u8 base[3][3],int quality){
    GgMeshCreaturePose p={x,y,ground,ggExpCos[heading&255],
                                    ggExpSin[heading&255]};
    int r=ggMeshClamp(base[0][0]*7+16,44,226);
    int g=ggMeshClamp(base[0][1]*7+14,48,223);
    int b=ggMeshClamp(base[0][2]*7+12,38,218);
    int oldMat=m->material,oldSeed=m->materialSeed;
    m->material=GG_MAT_NONE;m->materialSeed=family*991;
    int kind=ggMeshCreatureKind(shape);
    int water=kind==GG_CREATURE_FISH&&ggMeshTerrainAt(x>>8,y>>8)<=T_WATER;
    int airborne=shape==14||kind==GG_CREATURE_WINGED||shape==17||shape==48;
    if(!water)
        ggMeshContactShadow(m,x,y,kind==GG_CREATURE_QUADRUPED?42:27,
                            airborne?9:3);
    switch(kind){
    case GG_CREATURE_BIRD:
        ggMeshCreatureBird(m,&p,shape,phase,quality,r,g,b);break;
    case GG_CREATURE_FISH:
        ggMeshCreatureFish(m,&p,shape,phase,quality,water,r,g,b);break;
    case GG_CREATURE_CRAWLER:
        ggMeshCreatureCrawler(m,&p,shape,phase,quality,r,g,b);break;
    case GG_CREATURE_SERPENT:
        ggMeshCreatureSerpent(m,&p,shape,phase,quality,r,g,b);break;
    case GG_CREATURE_BLOB:
        ggMeshCreatureBlob(m,&p,shape,phase,quality,r,g,b);break;
    case GG_CREATURE_WINGED:
        ggMeshCreatureWinged(m,&p,shape,phase,r,g,b);break;
    case GG_CREATURE_QUADRUPED:
        ggMeshCreatureQuadruped(m,&p,shape,phase,quality,r,g,b);break;
    default:break; // the population's original-sprite painter owns these
    }
    m->material=oldMat;m->materialSeed=oldSeed;
}
static void ggMeshActorCharacter(GgMeshContext*m,int x,int y,int ground,
                                  int heading,int phase,int family,
                                  int humanoid,const u8 colors[3][3],
                                  int quality){
    int oldMat=m->material,oldSeed=m->materialSeed;
    // Actor clothing identity must not shimmer every pose frame.
    m->materialSeed=family*991;
    ggMeshContactShadow(m,x,y,humanoid?23:36,humanoid?6:4);
    if(humanoid)
        ggMeshActorHuman(m,x,y,ground,heading,phase,family,
                         colors,quality);
    else
        ggMeshActorAnimal(m,x,y,ground,heading,phase,colors,quality);
    m->material=oldMat;m->materialSeed=oldSeed;
}
#endif
