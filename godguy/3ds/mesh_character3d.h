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
        int footZ=ggMeshGroundAtQ8(footX,footY)+1;
        int kneeX=(sx+footX)/2+fx*forwardStep/8192;
        int kneeY=(sy+footY)/2+fy*forwardStep/8192;
        ggMeshActorTube(m,sx,sy,ground+8,6,
                        kneeX,kneeY,ground+4,5,
                        dr*4/5,dg*4/5,db*4/5,6);
        ggMeshActorTube(m,kneeX,kneeY,ground+4,5,
                        footX,footY,footZ+2,5,68,68,75,6);
        // Truly world-planted boot, never an unrelated floating cel.
        ggMeshBox(m,footX-9,footY-6,footX+9,footY+9,
                  footZ,footZ+2,52,55,63);
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
// Quadruped barrels run along the animal's forward axis. Unlike the
// vertical human limb tube, this ring is perpendicular to movement and
// includes actual upper/lower volume, not a flat horizontal ellipse.
static void ggMeshActorHorizontalBody(GgMeshContext*m,
                                      int ax,int ay,int bx,int by,
                                      int z,int halfWidth,int halfHeight,
                                      int r,int g,int b,int sides){
    int dx=bx-ax,dy=by-ay;
    int mag=(dx<0?-dx:dx)+(dy<0?-dy:dy);
    if(mag<1)return;
    int rightX=-dy*4096/mag,rightY=dx*4096/mag;
    for(int n=0;n<sides;n++){
        int a=n*256/sides,bangle=((n+1)*256/sides)&255;
        int p=ggExpCos[a]*halfWidth/4096,
            q=ggExpCos[bangle]*halfWidth/4096;
        int h=ggExpSin[a]*halfHeight/4096,
            j=ggExpSin[bangle]*halfHeight/4096;
        GgMeshVertex f0=ggMeshV(ax+rightX*p/4096,
                                 ay+rightY*p/4096,z+h);
        GgMeshVertex f1=ggMeshV(ax+rightX*q/4096,
                                 ay+rightY*q/4096,z+j);
        GgMeshVertex t0=ggMeshV(bx+rightX*p/4096,
                                 by+rightY*p/4096,z+h);
        GgMeshVertex t1=ggMeshV(bx+rightX*q/4096,
                                 by+rightY*q/4096,z+j);
        int shade=195+((n+3)%4)*14;
        ggMeshQuad(m,f0,f1,t1,t0,r*shade/256,
                   g*shade/256,b*shade/256);
    }
}
// Creature dimensions are local forward/across XY plus actual world Z.
// One tiny pose on the stack rotates every limb/fin with its owner.
typedef struct {int x,y,z,fx,fy;} GgMeshCreaturePose;
static GgMeshVertex ggMeshCreatureV(const GgMeshCreaturePose*p,
                                   int along,int across,int height){
    return ggMeshV(p->x+(p->fx*along-p->fy*across)/4096,
                   p->y+(p->fy*along+p->fx*across)/4096,p->z+height);
}
static int ggMeshCreatureFoot(const GgMeshCreaturePose*p,
                              int along,int across){
    GgMeshVertex v=ggMeshCreatureV(p,along,across,0);
    return ggMeshGroundAtQ8(v.x,v.y)-p->z+1;
}
// Two ring sections and tapered end fans make a CLOSED rounded hull.
// The old open barrel exposed the background through noses and rumps.
// Six sides costs 24 triangles; no temporary mesh/vertex array is used.
static void ggMeshCreatureHull(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int along,int across,int height,
                               int length,int width,int rise,
                               int r,int g,int b,int sides){
    for(int n=0;n<sides;n++){
        int a=n*256/sides,bangle=((n+1)*256/sides)&255;
        int u=ggExpCos[a]*width/4096,v=ggExpCos[bangle]*width/4096;
        int h=ggExpSin[a]*rise/4096,j=ggExpSin[bangle]*rise/4096;
        GgMeshVertex r0=ggMeshCreatureV(p,along-length/2,across+u,height+h);
        GgMeshVertex r1=ggMeshCreatureV(p,along-length/2,across+v,height+j);
        GgMeshVertex f0=ggMeshCreatureV(p,along+length/2,across+u,height+h);
        GgMeshVertex f1=ggMeshCreatureV(p,along+length/2,across+v,height+j);
        GgMeshVertex tail=ggMeshCreatureV(p,along-length,across,height);
        GgMeshVertex nose=ggMeshCreatureV(p,along+length,across,height);
        int shade=ggExpSin[a]>=0?256:213;
        int rr=r*shade/256,gg=g*shade/256,bb=b*shade/256;
        ggMeshQuad(m,r0,r1,f1,f0,rr,gg,bb);
        ggMeshTriangle(m,tail,r1,r0,rr*9/10,gg*9/10,bb*9/10);
        ggMeshTriangle(m,nose,f0,f1,rr,gg,bb);
    }
}
static void ggMeshCreatureLimb(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int a,int u,int h,int b,int v,int z,
                               int thick,int taper,int r,int g,int blue){
    GgMeshVertex from=ggMeshCreatureV(p,a,u,h);
    GgMeshVertex to=ggMeshCreatureV(p,b,v,z);
    ggMeshActorTube(m,from.x,from.y,from.z,thick,to.x,to.y,to.z,taper,
                    r,g,blue,4);
}
static void ggMeshCreatureEye(GgMeshContext*m,const GgMeshCreaturePose*p,
                              int along,int across,int height){
    GgMeshVertex e=ggMeshCreatureV(p,along,across,height);
    int material=m->material;m->material=GG_MAT_NONE;
    ggMeshBox(m,e.x-2,e.y-2,e.x+2,e.y+2,e.z,e.z+1,27,32,37);
    m->material=material;
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
    int low=shape==7||shape==15||shape==52||shape==59;
    int rabbit=shape==10,shelled=shape==15||shape==59;
    int hoof=shape==9||shape==23||shape==58||shape==67;
    int heavy=shape==8||shape==21||shape==22||shape==23;
    int bodyZ=low?5:rabbit?7:hoof?11:8;
    int length=rabbit?28:low?34:heavy?47:42;
    int width=rabbit?20:heavy?25:low?24:19;
    ggMeshCreatureHull(m,p,0,0,bodyZ,length,width,heavy?4:3,
                       r,g,b,quality>=2?8:6);
    // Jointed fore/hind limbs, with actual terrain under each toe.
    for(int front=-1;front<=1;front+=2)
      for(int side=-1;side<=1;side+=2){
        int gait=ggExpSin[(phase+front*32+side*64)&255]/650;
        int hip=front*(length-13),across=side*(width-4);
        if(rabbit&&front<0)across=side*(width-2);
        int knee=hip+front*4-gait/2,toe=hip+gait;
        int foot=ggMeshCreatureFoot(p,toe,across);
        ggMeshCreatureLimb(m,p,hip,across,bodyZ-1,knee,across,
                           (bodyZ+foot)/2,5,4,r*4/5,g*4/5,b*4/5);
        ggMeshCreatureLimb(m,p,knee,across,(bodyZ+foot)/2,
                           toe,across,foot,4,3,r*3/5,g*3/5,b*3/5);
      }
    // A wolf/cat has its muzzle near the shoulder line; only hoofed
    // animals have a tall neck. Rabbits have a compact body and haunches.
    int head=length+(shelled?2:rabbit?4:9);
    int headZ=bodyZ+(low?1:rabbit?3:hoof?5:2);
    ggMeshCreatureLimb(m,p,length-7,0,bodyZ,head-3,0,headZ,
                       11,9,r*9/10,g*9/10,b*9/10);
    ggMeshCreatureHull(m,p,head,0,headZ,shelled?12:rabbit?13:18,
                       low?13:rabbit?12:13,rabbit?3:2,
                       r,g,b,quality>=2?6:4);
    ggMeshCreatureHull(m,p,head+(rabbit||shelled?9:15),0,headZ-1,
                       rabbit||shelled?5:9,7,1,
                       r*3/5,g*3/5,b*3/5,4);
    for(int side=-1;side<=1;side+=2){
        if(!low){
            int ear=rabbit?6:3;
            GgMeshVertex a=ggMeshCreatureV(p,head-5,side*7,headZ+1);
            GgMeshVertex c=ggMeshCreatureV(p,head+2,side*13,headZ+2);
            GgMeshVertex tip=ggMeshCreatureV(p,head-4,side*15,headZ+ear);
            ggMeshTriangle(m,a,c,tip,r*4/5,g*4/5,b*4/5);
            ggMeshTriangle(m,c,a,tip,r,g,b);
        }
        ggMeshCreatureEye(m,p,head+5,side*(low?14:12),headZ+1);
        if(shape==9||shape==58){
            ggMeshCreatureLimb(m,p,head-5,side*7,headZ+2,
                               head-11,side*13,headZ+7,3,1,171,150,113);
        }
    }
    if(shape==67)
        ggMeshCreatureLimb(m,p,head+5,0,headZ+2,
                           head+11,0,headZ+9,3,1,221,207,159);
    if(shelled){
        ggMeshCreatureHull(m,p,-4,0,bodyZ+1,length-3,width+2,3,
                           r*3/4,g*3/4,b*3/4,quality>=2?8:6);
    }
    // Tail follows yaw and a slow gait sway, attached at the rump.
    int tail=ggExpSin[(phase+40)&255]/550;
    if(rabbit)
        ggMeshCreatureHull(m,p,-length,0,bodyZ+1,6,6,1,
                           r,g,b,4);
    else if(shape!=52)
        ggMeshCreatureLimb(m,p,-length+4,0,bodyZ+1,
                           -length-(shelled?9:19),tail,
                           bodyZ+(shape==55?4:-2),
                           4,2,r*4/5,g*4/5,b*4/5);
}
static void ggMeshCreatureBird(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int shape,int phase,int quality,
                               int r,int g,int b){
    int tall=shape==60,bat=shape==14;
    int center=tall?13:8,headZ=tall?25:shape==57?14:11;
    ggMeshCreatureHull(m,p,0,0,center,tall?26:20,13,4,
                       r,g,b,quality>=2?6:4);
    for(int side=-1;side<=1;side+=2){
        if(!bat){
            int toe=ggExpSin[(phase+side*64)&255]/1300;
            ggMeshCreatureLimb(m,p,0,side*7,center-2,
                               toe,side*7,ggMeshCreatureFoot(p,toe,side*7),
                               2,1,169,132,71);
        }
        int flap=ggExpSin[phase&255]/1800;
        GgMeshVertex root=ggMeshCreatureV(p,4,side*9,center+1);
        GgMeshVertex tip=ggMeshCreatureV(p,-10,side*(bat?40:23),center+flap);
        GgMeshVertex back=ggMeshCreatureV(p,-25,side*8,center-1);
        ggMeshTriangle(m,root,tip,back,r*4/5,g*4/5,b*4/5);
        ggMeshTriangle(m,back,tip,root,r*9/10,g*9/10,b*9/10);
    }
    ggMeshCreatureLimb(m,p,13,0,center+1,19,0,headZ-1,
                       5,5,r,g,b);
    ggMeshCreatureHull(m,p,20,0,headZ,9,7,3,r,g,b,quality>=2?6:4);
    if(bat){
        ggMeshCreatureHull(m,p,28,0,headZ-1,5,5,1,
                           r*3/5,g*3/5,b*3/5,4);
    }else{
        GgMeshVertex beak=ggMeshCreatureV(p,36,0,headZ-1);
        ggMeshTriangle(m,ggMeshCreatureV(p,26,-4,headZ),
                       ggMeshCreatureV(p,26,4,headZ),beak,207,163,80);
        ggMeshTriangle(m,ggMeshCreatureV(p,26,4,headZ),
                       ggMeshCreatureV(p,26,-4,headZ),beak,160,112,51);
    }
    for(int side=-1;side<=1;side+=2){
        ggMeshCreatureEye(m,p,23,side*6,headZ+1);
        if(bat){
            ggMeshTriangle(m,ggMeshCreatureV(p,16,side*3,headZ+1),
                           ggMeshCreatureV(p,20,side*10,headZ+6),
                           ggMeshCreatureV(p,24,side*5,headZ+2),
                           r*4/5,g*4/5,b*4/5);
        }else{
            ggMeshTriangle(m,ggMeshCreatureV(p,-14,side*5,center),
                           ggMeshCreatureV(p,-35,side*11,center+2),
                           ggMeshCreatureV(p,-27,0,center),r*4/5,g*4/5,b*4/5);
        }
    }
}
static void ggMeshCreatureFish(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int shape,int phase,int quality,
                               int r,int g,int b){
    int seal=shape==61,whale=shape==13;
    int length=whale?56:seal?43:43;
    ggMeshCreatureHull(m,p,0,0,4,length,whale?20:12,whale?4:3,
                       r,g,b,quality>=2?8:6);
    int tail=ggExpSin[phase&255]/700;
    GgMeshVertex root=ggMeshCreatureV(p,-length+6,0,4);
    if(seal||whale){
        ggMeshTriangle(m,root,ggMeshCreatureV(p,-length-18,-18+tail,4),
                       ggMeshCreatureV(p,-length-14,0,6),r*3/4,g*3/4,b*3/4);
        ggMeshTriangle(m,root,ggMeshCreatureV(p,-length-14,0,6),
                       ggMeshCreatureV(p,-length-18,18+tail,4),r*3/4,g*3/4,b*3/4);
    }else{
        GgMeshVertex upper=ggMeshCreatureV(p,-length-16,tail,9);
        GgMeshVertex lower=ggMeshCreatureV(p,-length-16,tail,0);
        ggMeshTriangle(m,root,upper,lower,r*3/4,g*3/4,b*3/4);
        ggMeshTriangle(m,root,lower,upper,r*4/5,g*4/5,b*4/5);
        ggMeshTriangle(m,ggMeshCreatureV(p,8,0,6),
                       ggMeshCreatureV(p,-12,0,10),
                       ggMeshCreatureV(p,-21,0,6),r*3/4,g*3/4,b*3/4);
    }
    for(int side=-1;side<=1;side+=2){
        ggMeshTriangle(m,ggMeshCreatureV(p,8,side*9,4),
                       ggMeshCreatureV(p,-12,side*26,2),
                       ggMeshCreatureV(p,-16,side*7,4),r*4/5,g*4/5,b*4/5);
        ggMeshCreatureEye(m,p,length/2+7,side*9,5);
    }
}
static void ggMeshCreatureCrawler(GgMeshContext*m,const GgMeshCreaturePose*p,
                                  int shape,int phase,int quality,
                                  int r,int g,int b){
    int legs=shape==12||shape==37||shape==50?4:3;
    ggMeshCreatureHull(m,p,-9,0,4,20,shape==50?23:14,2,
                       r,g,b,quality>=2?6:4);
    ggMeshCreatureHull(m,p,14,0,4,13,10,2,r*4/5,g*4/5,b*4/5,4);
    for(int side=-1;side<=1;side+=2)
      for(int leg=0;leg<legs;leg++){
        int along=14-leg*9;
        int twitch=ggExpSin[(phase+leg*53+side*40)&255]/1100;
        int footAlong=along+(leg<legs/2?8:-8)+twitch;
        ggMeshCreatureLimb(m,p,along,side*8,3,
                           along+(leg<legs/2?4:-4),side*28,4,
                           2,2,r*3/5,g*3/5,b*3/5);
        ggMeshCreatureLimb(m,p,along+(leg<legs/2?4:-4),side*28,4,
                           footAlong,side*40,
                           ggMeshCreatureFoot(p,footAlong,side*40),
                           2,1,r*3/5,g*3/5,b*3/5);
      }
    for(int side=-1;side<=1;side+=2){
        ggMeshCreatureEye(m,p,22,side*7,5);
        ggMeshCreatureLimb(m,p,24,side*5,5,34,side*12,8,
                           1,1,r*4/5,g*4/5,b*4/5);
    }
    if(shape==37){
        ggMeshCreatureLimb(m,p,-22,0,7,-35,0,13,4,3,r,g,b);
        ggMeshCreatureLimb(m,p,-35,0,13,-23,0,17,3,1,r,g,b);
    }
}
static void ggMeshCreatureSerpent(GgMeshContext*m,const GgMeshCreaturePose*p,
                                  int phase,int quality,int r,int g,int b){
    for(int part=0;part<5;part++){
        int along=28-part*15;
        int wave=ggExpSin[(phase+part*30)&255]/400;
        ggMeshCreatureHull(m,p,along,wave,part?3:5,13,part==4?5:9,
                           part==4?1:2,r,g,b,quality>=2?6:4);
        if(!part)for(int side=-1;side<=1;side+=2)
            ggMeshCreatureEye(m,p,along+5,wave+side*7,6);
    }
}
static void ggMeshCreatureWinged(GgMeshContext*m,const GgMeshCreaturePose*p,
                                 int shape,int phase,int r,int g,int b){
    int height=12+ggExpSin[phase&255]/2300;
    ggMeshCreatureHull(m,p,0,0,height,19,7,2,r,g,b,4);
    int wingZ=height+ggExpSin[(phase*3)&255]/850;
    for(int side=-1;side<=1;side+=2){
        int span=shape==63?32:25;
        GgMeshVertex root=ggMeshCreatureV(p,0,side*4,height+1);
        GgMeshVertex tip=ggMeshCreatureV(p,14,side*span,wingZ);
        GgMeshVertex back=ggMeshCreatureV(p,-20,side*(span-4),wingZ-1);
        int wr=shape==63?r:189,wg=shape==63?g:204,wb=shape==63?b:207;
        ggMeshTriangle(m,root,tip,back,wr,wg,wb);
        ggMeshTriangle(m,root,back,tip,wr*4/5,wg*4/5,wb*4/5);
        ggMeshCreatureEye(m,p,14,side*5,height+1);
        ggMeshCreatureLimb(m,p,17,side*3,height,25,side*8,height+3,
                           1,1,58,54,51);
    }
}
static void ggMeshCreatureBlob(GgMeshContext*m,const GgMeshCreaturePose*p,
                               int phase,int quality,int r,int g,int b){
    int breath=ggExpSin[phase&255]>0;
    ggMeshCreatureHull(m,p,0,0,4,29,28+breath,4+breath,
                       r,g,b,quality>=2?8:6);
    for(int side=-1;side<=1;side+=2)
        ggMeshCreatureEye(m,p,20,side*10,6+breath);
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
    ggMeshContactShadow(m,x,y,kind==GG_CREATURE_QUADRUPED?48:32,4);
    switch(kind){
    case GG_CREATURE_BIRD:
        ggMeshCreatureBird(m,&p,shape,phase,quality,r,g,b);break;
    case GG_CREATURE_FISH:
        ggMeshCreatureFish(m,&p,shape,phase,quality,r,g,b);break;
    case GG_CREATURE_CRAWLER:
        ggMeshCreatureCrawler(m,&p,shape,phase,quality,r,g,b);break;
    case GG_CREATURE_SERPENT:
        ggMeshCreatureSerpent(m,&p,phase,quality,r,g,b);break;
    case GG_CREATURE_BLOB:
        ggMeshCreatureBlob(m,&p,phase,quality,r,g,b);break;
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
