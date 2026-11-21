// GodGuy 3DS physical inhabitants: low-poly articulated silhouettes.
// The playable world uses its SAME 3D camera, true world ground and z
// buffer. No billboard sprites, animation atlas, skeletal heap buffers,
// external textures, new framebuffer or fake screen-space depth.
// Film actor cels remain the fallback until filmed mesh closeups pass QA.
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
static void ggMeshActorAnimal(GgMeshContext*m,int x,int y,
                               int ground,int heading,int phase,
                               const u8 base[3][3],int quality){
    int coatR=ggMeshClamp(base[0][0]*8,44,213),
        coatG=ggMeshClamp(base[0][1]*8,48,220),
        coatB=ggMeshClamp(base[0][2]*8,38,189);
    int forwardX=ggExpCos[heading&255],forwardY=ggExpSin[heading&255];
    int sideX=-forwardY,sideY=forwardX;
    int save=m->material;m->material=GG_MAT_FABRIC;
    int len=quality>=2?29:25,half=17;
    ggMeshActorTube(m,x-forwardX*len/4096,y-forwardY*len/4096,
                    ground+9,half,
                    x+forwardX*len/4096,y+forwardY*len/4096,
                    ground+9,half,
                    coatR,coatG,coatB,quality>=2?8:6);
    for(int ix=-1;ix<=1;ix+=2)
      for(int iy=-1;iy<=1;iy+=2){
        int bx=x+forwardX*ix*19/4096+sideX*iy*12/4096;
        int by=y+forwardY*ix*19/4096+sideY*iy*12/4096;
        int bob=ggExpSin[(phase+ix*32+iy*48)&255]/1400;
        int toeX=bx+forwardX*bob/4096,toeY=by+forwardY*bob/4096;
        ggMeshActorTube(m,bx,by,ground+8,5,toeX,toeY,
                        ggMeshGroundAtQ8(toeX,toeY)+1,4,
                        coatR*4/5,coatG*4/5,coatB*4/5,5);
      }
    int hx=x+forwardX*36/4096,hy=y+forwardY*36/4096;
    ggMeshActorTube(m,hx,hy,ground+10,13,
                    hx,hy,ground+17,11,coatR,coatG,coatB,6);
    int noseX=hx+forwardX*15/4096,
        noseY=hy+forwardY*15/4096;
    ggMeshActorTube(m,noseX,noseY,ground+10,7,
                    noseX+forwardX*8/4096,
                    noseY+forwardY*8/4096,
                    ground+12,4,65,67,65,5);
    m->material=save;
}
static void ggMeshActorCharacter(GgMeshContext*m,int x,int y,int ground,
                                  int heading,int phase,int family,
                                  int humanoid,const u8 colors[3][3],
                                  int quality){
    int oldMat=m->material,oldSeed=m->materialSeed;
    m->materialSeed=family*991+phase/16;
    ggMeshContactShadow(m,x,y,humanoid?23:36,humanoid?6:4);
    if(humanoid)
        ggMeshActorHuman(m,x,y,ground,heading,phase,family,
                         colors,quality);
    else
        ggMeshActorAnimal(m,x,y,ground,heading,phase,colors,quality);
    m->material=oldMat;m->materialSeed=oldSeed;
}
#endif
