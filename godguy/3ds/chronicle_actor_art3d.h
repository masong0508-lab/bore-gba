// Field guards and soldiers use directional, grounded cels in the world pass.
// The native people renderer supplies perspective, air, contact and depth.
#ifndef GG_CHRONICLE_ACTOR_ART3D_H
#define GG_CHRONICLE_ACTOR_ART3D_H

enum { GG_CHRON_ACTOR_ALLY,GG_CHRON_ACTOR_FOE,GG_CHRON_ACTOR_CAPTAIN };
enum { GG_CHRON_ACTOR_READY,GG_CHRON_ACTOR_MOVE,GG_CHRON_ACTOR_WINDUP,
       GG_CHRON_ACTOR_STRIKE,GG_CHRON_ACTOR_RECOVER };

static int ggChronActorHeading(int dx,int dy){
    if(!dx&&!dy)return 0;
    if(abs(dx)>abs(dy)*2)return dx>0?0:128;
    if(abs(dy)>abs(dx)*2)return dy>0?64:192;
    return dy>0?(dx>0?32:96):(dx>0?224:160);
}

static void ggChronActorSword(GgPersonCel*c,int handX,int handY,
                              int tipX,int tipY,int side){
    ggPersonStroke(c,handX,handY,tipX,tipY,GG_P_OUTLINE,3);
    ggPersonStroke(c,handX,handY,tipX,tipY,GG_P_METAL,2);
    ggPersonStroke(c,handX+side,handY-1,tipX,tipY,GG_P_SHIRT,1);
    ggPersonRect(c,handX-1,handY-1,3,4,GG_P_BOOT_D);
    if(abs(tipY-handY)>abs(tipX-handX))
        ggPersonRect(c,handX-3,handY+(tipY<handY?-2:2),7,1,GG_P_TRIM);
    else
        ggPersonRect(c,handX+side*2,handY-3,1,7,GG_P_TRIM);
    ggPersonRect(c,handX-1,handY,3,2,GG_P_HAIR_M);
    ggPersonDot(c,handX,handY+2,GG_P_TRIM);
}

static void ggChronActorShield(GgPersonCel*c,int x,int y,int profile,int faction){
    int width=profile?3:6;
    // Allied shields taper to a point. The enemy uses a broad, round buckler.
    if(faction==GG_CHRON_ACTOR_ALLY){
        ggPersonTaper(c,x,y-7,y-5,width-2,width,GG_P_OUTLINE);
        ggPersonTaper(c,x,y-4,y+4,width,width-1,GG_P_OUTLINE);
        ggPersonTaper(c,x,y+5,y+9,width-2,0,GG_P_OUTLINE);
        ggPersonTaper(c,x,y-5,y+3,width-1,width-2,GG_P_ACCENT);
        ggPersonTaper(c,x,y+4,y+7,width-2,0,GG_P_ACCENT);
        ggPersonStroke(c,x-width+2,y-3,x,y,GG_P_SHIRT,1);
        ggPersonStroke(c,x,y,x+width-2,y-3,GG_P_SHIRT,1);
    }else{
        ggPersonOval(c,x,y,width,7,GG_P_OUTLINE);
        ggPersonOval(c,x,y,width-1,6,GG_P_ACCENT);
        ggPersonRect(c,x-width+2,y-1,width*2-3,2,GG_P_TRIM);
    }
    ggPersonDot(c,x,y+2,GG_P_METAL);
}

static void ggChronActorCel(GgPersonCel*c,uint32_t identity,int faction,
                            int turn,int pose,int phase){
    static const unsigned char coat[3][3][3]={
        {{9,22,24},{7,12,14},{21,28,28}},
        {{23,7,8},{16,7,9},{31,15,10}},
        {{23,6,15},{13,7,8},{31,20,11}}
    };
    // Shaded steel, raised rim, colored enamel, fittings and edge catches.
    static const unsigned char ink[3][6][3]={
        {{42,62,69},{88,120,127},{160,191,191},{36,121,128},{214,175,101},{216,232,222}},
        {{51,45,51},{102,88,89},{163,148,137},{143,55,52},{208,147,90},{232,220,191}},
        {{85,56,53},{146,105,73},{219,171,106},{124,42,64},{242,191,105},{245,229,177}}
    };
    static const unsigned char pen[6]={GG_P_HAIR_D,GG_P_HAIR_M,GG_P_HAIR_L,
                                       GG_P_ACCENT,GG_P_TRIM,GG_P_SHIRT};
    GgPersonTraits traits=ggPersonTraits(identity,(int)(identity&7u));
    memset(c,0,sizeof *c);
    ggPersonColors(c,traits,coat[faction]);
    for(int i=0;i<6;i++)
        ggPersonInk(c,pen[i],ink[faction][i][0],ink[faction][i][1],ink[faction][i][2]);

    int facing=turn>=1&&turn<=3?-1:turn>=5?1:0;
    int profile=turn==2||turn==6,rear=turn>=3&&turn<=5;
    int captain=faction==GG_CHRON_ACTOR_CAPTAIN;
    int phase8=phase>>5,moving=pose==GG_CHRON_ACTOR_MOVE;
    int windup=pose==GG_CHRON_ACTOR_WINDUP,strike=pose==GG_CHRON_ACTOR_STRIKE;
    int recover=pose==GG_CHRON_ACTOR_RECOVER;
    int bend=windup?1+phase/160:strike?2:recover?2-phase/128:0;
    int center=16+(windup?-facing:0)+(strike?facing*2:0);
    int waist=31+bend/2,shoulder=18+bend;
    int width=(profile?4:(turn&1)?6:7)+captain;
    int swordSide=profile?facing:rear?1:-1,shieldSide=-swordSide;
    static const signed char stride[8]={0,2,3,2,0,-2,-3,-2};
    static const unsigned char lifted[8]={0,1,2,1,0,0,0,0};

    // Hip weight can shift across the stride. At least one boot always
    // keeps its sole on the shared bottom row of the cel.
    for(int i=0;i<2;i++){
        int side=i?1:-1,step=(phase8+i*4)&7;
        int hip=center+side*(profile?2:3);
        int boot=16+side*(profile?3:5)+(moving?facing*stride[step]:0);
        if(strike)boot+=side;
        int sole=47-(moving?lifted[step]:0),knee=(hip+boot)/2;
        ggPersonStroke(c,hip,waist,knee,39,GG_P_OUTLINE,5);
        ggPersonStroke(c,knee,39,boot,sole-1,GG_P_OUTLINE,4);
        ggPersonStroke(c,hip-1,waist+1,knee-1,39,
                       i?GG_P_PANTS_D:GG_P_PANTS_L,3);
        ggPersonStroke(c,knee,39,boot,sole-3,GG_P_HAIR_D,2);
        ggPersonDot(c,knee-1,39,GG_P_HAIR_M);
        ggPersonRect(c,boot-2,sole-2,5,2,GG_P_BOOT_L);
        ggPersonRect(c,boot-2,sole,5,1,GG_P_OUTLINE);
    }

    if(captain||rear){
        int cape=rear?width+2:width+1;
        ggPersonTaper(c,center,shoulder,38,width,cape,GG_P_OUTLINE);
        ggPersonTaper(c,center,shoulder+1,37,width-1,cape-1,GG_P_COAT_D);
        ggPersonStroke(c,center-2,shoulder+5,center-3,36,GG_P_COAT_M,1);
    }
    ggPersonTaper(c,center,shoulder-1,waist,width+1,width-1,GG_P_OUTLINE);
    ggPersonTaper(c,center,shoulder,waist-1,width,width-2,GG_P_COAT_D);
    ggPersonTaper(c,center,shoulder+2,waist-4,width-1,width-3,GG_P_COAT_M);
    ggPersonRect(c,center-width+2,waist-1,width*2-3,2,GG_P_BOOT_D);
    ggPersonDot(c,center+facing,waist-1,GG_P_TRIM);
    if(rear){
        ggPersonStroke(c,center-3,shoulder+1,center+3,waist-3,GG_P_BOOT_D,2);
        ggPersonStroke(c,center-4,shoulder+1,center+2,waist-3,GG_P_TRIM,1);
    }else{
        ggPersonStroke(c,center-width+2,shoulder+2,center+facing,waist-3,
                       GG_P_HAIR_D,2);
        ggPersonStroke(c,center-width+2,shoulder+2,center+facing,waist-5,
                       GG_P_COAT_L,1);
        ggPersonDot(c,center+facing,shoulder+5,GG_P_TRIM);
    }

    for(int i=0;i<2;i++){
        int side=i?1:-1,x=center+side*width;
        ggPersonOval(c,x,shoulder+1,3,captain?4:3,GG_P_OUTLINE);
        ggPersonOval(c,x,shoulder,2,2,GG_P_HAIR_M);
        ggPersonDot(c,x-1,shoulder-1,captain?GG_P_TRIM:GG_P_HAIR_L);
    }
    int wristX=center+swordSide*(strike?width+2-phase/21:width+3);
    int wristY=windup?20-phase/96:strike?20+phase/32:
               recover?25+phase/110:29+(moving?stride[phase8]/2:0);
    int tipX=wristX+swordSide*(windup?-3-phase/96:strike?12-phase/10:3);
    int tipY=windup?wristY-14:strike?wristY-6+phase/12:
             wristY+(recover?11:13);
    ggPersonStroke(c,center+swordSide*width,shoulder+2,
                   wristX-swordSide,wristY,GG_P_OUTLINE,4);
    ggPersonStroke(c,center+swordSide*width,shoulder+2,
                   wristX-swordSide,wristY,GG_P_HAIR_M,2);
    ggChronActorSword(c,wristX,wristY,tipX,tipY,swordSide);
    int shieldX=center+shieldSide*(width+(profile?3:2));
    int shieldY=windup?25:strike?27:29+(moving?stride[(phase8+4)&7]/2:0);
    ggChronActorShield(c,shieldX,shieldY,profile,faction);

    int face=center+facing,head=10+bend;
    if(captain){
        ggPersonStroke(c,face-facing,head-6,face-4,head-9,GG_P_COAT_D,3);
        ggPersonStroke(c,face-1,head-7,face-4,head-9,GG_P_COAT_L,1);
    }
    ggPersonOval(c,face,head,profile?4:5,7,GG_P_OUTLINE);
    ggPersonOval(c,face,head+2,profile?3:4,4,
                 rear?GG_P_HAIR_D:GG_P_SKIN_D);
    if(!rear){
        ggPersonRect(c,face-2+facing,head+1,profile?3:5,3,GG_P_SKIN_M);
        ggPersonDot(c,face+facing*2,head+2,GG_P_FACE);
        if(!profile)ggPersonDot(c,face-2,head+2,GG_P_FACE);
        if(profile)ggPersonDot(c,face+facing*4,head+3,GG_P_SKIN_M);
    }
    ggPersonTaper(c,face,head-6,head,2,profile?4:5,GG_P_HAIR_M);
    ggPersonStroke(c,face-2,head-5,face-3,head-1,GG_P_HAIR_L,1);
    if(faction==GG_CHRON_ACTOR_ALLY){
        ggPersonRect(c,face-(profile?5:6),head,profile?10:13,2,GG_P_HAIR_D);
        ggPersonRect(c,face-(profile?4:5),head,profile?8:11,1,GG_P_HAIR_L);
    }else{
        ggPersonRect(c,face-4,head-1,9,2,GG_P_HAIR_D);
        ggPersonStroke(c,face-3,head-1,face+3,head-1,GG_P_TRIM,1);
        if(!rear)ggPersonStroke(c,face+facing,head,face+facing,head+4,
                              GG_P_HAIR_M,2);
    }
}

static int ggChronActorDraw(int wx,int wy,uint32_t identity,int faction,
                            int heading,int pose,int phase){
    if(!ggExpMeshActive)return 0;
    GgMeshContext*m=&ggMeshContext;
    int ground=ggMeshGroundAtQ8(wx,wy);
    GgMeshCameraVertex view=ggMeshCameraPoint(m,ggMeshV(wx,wy,ground));
    if(!ggMeshPersonInView(m,view.f,view.s))return 0;
    ggChronActorCel(&ggMeshPersonCel,identity,faction,
                    ggMeshPersonFacing(heading,ggExplore.angle),pose,
                    ggExpClamp(phase,0,255));
    return ggMeshPersonDrawGenerated(m,wx,wy,ground,view.f,view.s,heading,
                                     pose==GG_CHRON_ACTOR_MOVE);
}
#endif
