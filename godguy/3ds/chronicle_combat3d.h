#ifndef GG_CHRONICLE_COMBAT3D_H
#define GG_CHRONICLE_COMBAT3D_H

enum {
    GG_COMBAT_NONE, GG_COMBAT_HIT, GG_COMBAT_COVER, GG_COMBAT_MISS,
    GG_COMBAT_TOO_SOON, GG_COMBAT_STAMINA, GG_COMBAT_FOCUS,
    GG_COMBAT_BLOCK, GG_COMBAT_PARRY, GG_COMBAT_WARD,
    GG_COMBAT_SHOT_MISS, GG_COMBAT_RECOVERY, GG_COMBAT_BUSY
};
enum { GG_CHRON_BODY=GG_WORLD_PERSON_HEIGHT, GG_CHRON_SHOT_STEP=16,
       GG_CHRON_ARROW_FALL=35, GG_CHRON_STRIKE_FRAMES=5 };

static void ggChronCombatNotice(int notice){
    ggChron.combatNotice=notice;
    ggChron.combatNoticeT=48;
    ggChron.dirty=1;
}
static int ggChronFoeX(const GgChronFoe*f){return f->x*256+128;}
static int ggChronFoeY(const GgChronFoe*f){return f->y*256+128;}
static int ggChronCombatEye(void){
    return ggMeshGroundAtQ8(ggExplore.px,ggExplore.py)+GG_WORLD_EYE_HEIGHT;
}
static int ggChronRuinCover(GgMeshBuildingFootprint p,int top,int x,int y,int z){
    GgMeshRuinWall wall[3];GgMeshRuinPier pier[2];
    ggMeshRuinLayout(p.x0,p.y0,p.x1,p.y1,top,wall,pier);
    for(int i=0;i<2;i++)if(x>=pier[i].x0&&x<=pier[i].x1&&y>=pier[i].y0&&
                          y<=pier[i].y1&&z<=pier[i].top)return 1;
    for(int i=0;i<3;i++){
        const GgMeshRuinWall*w=&wall[i];
        int normal=w->axis?y:x,across=w->axis?x:y;
        if(normal<w->p0||normal>w->p1||across<w->u0||across>w->u1)continue;
        for(int j=0;j<5;j++){
            int a=w->u0+(w->u1-w->u0)*j/5,b=w->u0+(w->u1-w->u0)*(j+1)/5;
            if(across>b)continue;
            int lo=ggMeshBrokenWallCrown[j],hi=ggMeshBrokenWallCrown[j+1];
            if((z-w->top-lo)*(b-a)<=(hi-lo)*(across-a))return 1;
            break; // Another ruin wall can be taller at this corner.
        }
    }
    return 0;
}
static int ggChronStructureCover(int build,int tx,int ty,int x,int y,int z){
    if(!ggExpBuilding(build)||build==B_DOCK)return 0;
    GgMeshBuildingFootprint p=ggMeshBuildingFootprint(tx,ty,build);
    int ground=ggExpGround(ggMeshTerrainAt(tx,ty));
    int nominal=ground+ggExpBuilding(build)+(build==B_FARM?6:0);
    if(build==B_RUIN)return ggChronRuinCover(p,nominal,x,y,z);
    if(build==B_STATUE){
        int base=z<ground?ggMeshBuildingFoundation(p,ground):ground;
        return ggMeshStatueContains(x-(p.x0+p.x1)/2,y-(p.y0+p.y1)/2,
                                     z-ground,base-ground);
    }
    int within=x>=p.x0&&x<=p.x1&&y>=p.y0&&y<=p.y1;
    if(within&&z<=nominal)return 1;
    int temple=build==B_TEMPLE,hall=build==B_HALL;
    int plane=hall?p.y0:p.x0,normal=hall?y:x;
    int reach=temple?GG_MESH_TEMPLE_REACH:GG_MESH_HALL_REACH;
    int porch=(temple||hall)&&normal>=plane-reach-GG_MESH_PORTICO_STEP&&normal<=plane+8;
    if(!within&&!porch&&z<nominal-5)return 0;
    int floors[4];
    int top=ggMeshBuildingWallTop(p,tx,ty,build,ground,floors);
    if(porch){
        int center=hall?(p.x0+p.x1)/2:(p.y0+p.y1)/2;
        GgMeshPorticoShape shape=ggMeshPorticoLayout(hall,plane,center,
            floors[hall?2:0],top-(temple?8:12),temple);
        if(ggMeshPorticoContains(&shape,normal,hall?x:y,z,temple))return 1;
    }
    if(within&&z<=top)return 1;
    if(build==B_TOWER)
        return z>=top-5&&z<=top+3&&x>=p.x0-22&&x<=p.x1+22&&
               y>=p.y0-22&&y<=p.y1+22;
    if(!ggMeshHasRoof(build)||z<top-5)return 0;
    GgMeshRoofProfile roof=ggMeshRoofProfile(build,p.x0,p.y0,p.x1,p.y1,top);
    if(x<roof.left||x>roof.right||y<roof.y[0]||y>roof.y[roof.count-1])return 0;
    for(int i=0;i+1<roof.count;i++){
        if(y>roof.y[i+1])continue;
        int height=roof.z[i]+(roof.z[i+1]-roof.z[i])*(y-roof.y[i])/
                    (roof.y[i+1]-roof.y[i]);
        int edge=x-roof.left<roof.right-x?x-roof.left:roof.right-x;
        if(edge<roof.hip){
            int side=roof.z[0]+(roof.z[1]-roof.z[0])*edge/roof.hip;
            if(side<height)height=side;
        }
        return z<=height;
    }
    return 0;
}
static int ggChronBuildingAt(int x,int y,int z,int relativeToGround){
    int tx=x>>8,ty=y>>8;
    if((unsigned)tx>=WW||(unsigned)ty>=WH)return 1;
    int building=bld[ty][tx];
    // The temple faces west and the hall faces north. Each porch
    // can reach one neighboring tile on its authored entrance side.
    int templeReach=GG_MESH_TEMPLE_REACH+GG_MESH_PORTICO_STEP-GG_MESH_BUILDING_MARGIN;
    int hallReach=GG_MESH_HALL_REACH+GG_MESH_PORTICO_STEP-GG_MESH_BUILDING_MARGIN;
    int temple=(x&255)>=256-templeReach&&tx+1<WW&&bld[ty][tx+1]==B_TEMPLE;
    int hall=(y&255)>=256-hallReach&&ty+1<WH&&bld[ty+1][tx]==B_HALL;
    if(!building&&!temple&&!hall)return 0;
    if(relativeToGround)z+=ggMeshGroundAtQ8(x,y);
    return (building&&ggChronStructureCover(building,tx,ty,x,y,z))||
           (temple&&ggChronStructureCover(B_TEMPLE,tx+1,ty,x,y,z))||
           (hall&&ggChronStructureCover(B_HALL,tx,ty+1,x,y,z));
}
static int ggChronCombatCover(int x,int y,int z){
    int tx=x>>8,ty=y>>8;
    if((unsigned)tx>=WW||(unsigned)ty>=WH)return 1;
    int ground=ggMeshGroundAtQ8(x,y);
    if(z<=ground||ggChronBuildingAt(x,y,z,0))return 1;
    // Civilian bodies absorb a shot before it can reach a campaign foe.
    // Campaign weapons never apply damage to the underlying population.
    if(occ[ty][tx]&&abs((x&255)-128)<34&&abs((y&255)-128)<34&&
       z<=ground+GG_CHRON_BODY)return 1;
    return 0;
}
static int ggChronCombatLine(int ax,int ay,int az,int bx,int by,int bz){
    int dx=bx-ax,dy=by-ay;
    int span=abs(dx)>abs(dy)?abs(dx):abs(dy);
    int steps=(span+GG_CHRON_SHOT_STEP-1)/GG_CHRON_SHOT_STEP;
    for(int step=1;step<steps;step++)
        if(ggChronCombatCover(ax+dx*step/steps,ay+dy*step/steps,
                             az+(bz-az)*step/steps))return 0;
    return 1;
}

enum { GG_CHRON_TARGET_COVER=-2 };

static int ggChronChargeThreshold(int style){
    return style==GG_CHRON_BLADE?19:style==GG_CHRON_BOW?8:10;
}
static int ggChronAttackCost(int style,int charge){
    int charged=charge>=ggChronChargeThreshold(style);
    if(style==GG_CHRON_BLADE)return charged?23:9;
    if(style==GG_CHRON_BOW)return charged?8+charge/8:6+charge/4;
    return charged?18+charge/5:12+charge/4;
}
static int ggChronCombatCanAfford(int style,int cost){
    int ember=style==GG_CHRON_EMBER;
    if((ember?ggChron.focus:ggChron.stamina)>=cost)return 1;
    ggChronCombatNotice(ember?GG_COMBAT_FOCUS:GG_COMBAT_STAMINA);
    return 0;
}

static int ggChronCombatTarget(int range,int wide,int *distance){
    int angle=ggExplore.angle&255;
    int cosine=ggExpCos[angle],sine=ggExpSin[angle];
    int eye=ggChronCombatEye(),nearest=range+1,found=-1,blocked=0;
    for(int i=0;i<ggChron.foeN;i++){
        const GgChronFoe*f=&ggChron.foe[i];
        if(f->hp<=0)continue;
        int x=ggChronFoeX(f),y=ggChronFoeY(f);
        int dx=x-ggExplore.px,dy=y-ggExplore.py;
        if(abs(dx)>range||abs(dy)>range)continue;
        if(dx*dx+dy*dy>range*range)continue;
        int forward=(dx*cosine+dy*sine)/4096;
        int side=abs((dy*cosine-dx*sine)/4096);
        if(forward<24||forward>=nearest||side>(wide?forward/2+18:forward/16+26))
            continue;
        int ground=ggMeshGroundAtQ8(x,y);
        if(wide&&abs(ground+GG_WORLD_EYE_HEIGHT-eye)>8)continue;
        if(!ggChronCombatLine(ggExplore.px,ggExplore.py,eye,x,y,ground+10)){
            blocked=1;
            continue;
        }
        found=i;nearest=forward;
    }
    if(distance)*distance=nearest;
    return found<0&&blocked?GG_CHRON_TARGET_COVER:found;
}
static void ggChronDamageFoe(int index,int damage,int stagger){
    GgChronFoe*f=&ggChron.foe[index];
    f->hp=(signed char)ggExpClamp(f->hp-damage,0,127);
    ggChron.score+=5;
    if(f->hp<=0){
        ggChron.foeWindup[index]=0;
        ggChron.morale=ggExpClamp(ggChron.morale+3,0,100);
        ggChron.score+=20;
    }else if(stagger){
        ggChron.foeWindup[index]=0;
        if(f->wait<7)f->wait=7;
    }
    if(f->hp>0)ggChron.foeAlert[index]=60;
    ggChronCombatNotice(GG_COMBAT_HIT);
}
static void ggChronIncomingHit(int index,int damage){
    GgChronFoe*f=&ggChron.foe[index];
    int dx=ggChronFoeX(f)-ggExplore.px,dy=ggChronFoeY(f)-ggExplore.py;
    int angle=ggExplore.angle&255;
    int front=dx*ggExpCos[angle]+dy*ggExpSin[angle];
    int side=dy*ggExpCos[angle]-dx*ggExpSin[angle];
    int defending=ggChron.guardTicks&&!ggChron.charge&&!ggChron.swing;
    int style=ggChron.style;
    if(defending&&style==GG_CHRON_BLADE&&front>0&&abs(side)<=front&&
       ggChronCombatCanAfford(style,9)){
        ggChron.stamina-=9;
        ggChron.attackFlash=0;
        if(ggChron.guardTicks<=8){
            f->wait=(signed char)ggExpClamp(f->wait+8,12,63);
            ggChronCombatNotice(GG_COMBAT_PARRY);
            return;
        }
        damage=(damage+2)/3;
        ggChronCombatNotice(GG_COMBAT_BLOCK);
    }else if(defending&&style==GG_CHRON_EMBER&&ggChronCombatCanAfford(style,13)){
        ggChron.focus-=13;
        ggChron.attackFlash=0;
        damage=(damage+3)/4;
        ggChronCombatNotice(GG_COMBAT_WARD);
    }
    ggChron.hp-=damage;
    ggChron.morale=ggExpClamp(ggChron.morale-1,0,100);
    ggChron.dirty=1;
}

static int ggChronShotHit(const GgChronShot*p,int x,int y,int z){
    if(ggChronCombatCover(x,y,z)){
        ggChronCombatNotice(GG_COMBAT_COVER);
        return 1;
    }
    for(int i=0;i<ggChron.foeN;i++){
        const GgChronFoe*f=&ggChron.foe[i];
        if(f->hp<=0||abs(x-ggChronFoeX(f))>42||abs(y-ggChronFoeY(f))>42)
            continue;
        int ground=ggMeshGroundAtQ8(ggChronFoeX(f),ggChronFoeY(f));
        if(z>ground+GG_CHRON_BODY||z<=ground)continue;
        ggChronDamageFoe(i,p->damage,p->stagger);
        return 1;
    }
    return 0;
}
static void ggChronShotsStep(void){
    for(int i=0;i<GG_CHRON_SHOTS;i++){
        GgChronShot*p=&ggChron.shots[i];
        if(!p->life)continue;
        int speed=abs(p->vx)>abs(p->vy)?abs(p->vx):abs(p->vy);
        int steps=(speed+GG_CHRON_SHOT_STEP-1)/GG_CHRON_SHOT_STEP;
        int hit=0;
        for(int step=1;step<=steps;step++){
            int x=p->x+p->vx*step/steps,y=p->y+p->vy*step/steps;
            int z=(p->z+p->vz*step/steps)>>8;
            if(ggChronShotHit(p,x,y,z)){hit=1;break;}
        }
        if(hit)p->life=0;
        else if(--p->life==0)ggChronCombatNotice(GG_COMBAT_SHOT_MISS);
        p->x+=p->vx;p->y+=p->vy;p->z+=p->vz;
        if(p->style==GG_CHRON_BOW)p->vz-=GG_CHRON_ARROW_FALL;
        ggChron.dirty=1;
    }
}
static int ggChronReleaseShot(int style,int charge,int steady){
    int bow=style==GG_CHRON_BOW,charged=charge>=ggChronChargeThreshold(style);
    int cost=ggChronAttackCost(style,charge);
    int *meter=bow?&ggChron.stamina:&ggChron.focus;
    if(!ggChronCombatCanAfford(style,cost))return 0;
    GgChronShot *shot=NULL;
    for(int i=0;i<GG_CHRON_SHOTS;i++)if(!ggChron.shots[i].life){
        shot=&ggChron.shots[i];break;
    }
    if(!shot){ggChronCombatNotice(GG_COMBAT_BUSY);return 0;}
    int aim=ggExplore.angle&255;
    int sway=bow&&!steady?(ggChron.frame%7)-3:0;
    int a=(aim+sway)&255,speed=bow?68+charge:charged?114:96;
    int range=(charged?(bow?8:6):(bow?4:3))*256,forward;
    int damage=charged?(bow?13+charge/2:21+charge/3):
                       (bow?9:13)+charge/2;
    int foe=ggChronCombatTarget(range,0,&forward);
    GgChronShot released={.x=ggExplore.px,.y=ggExplore.py,
        .z=(ggChronCombatEye()-2)*256,.vx=ggExpCos[a]*speed/4096,
        .vy=ggExpSin[a]*speed/4096,.life=(unsigned char)(range/speed+2),
        .style=(unsigned char)style,.damage=(unsigned char)damage,
        .stagger=(unsigned char)(!bow&&charge>=20)};
    if(foe>=0){
        int frames=(forward+speed/2)/speed;
        if(frames<1)frames=1;
        int tx=ggChronFoeX(&ggChron.foe[foe]),ty=ggChronFoeY(&ggChron.foe[foe]);
        int z=(ggMeshGroundAtQ8(tx,ty)+10)*256;
        int drift=sway*forward/112;
        tx-=ggExpSin[aim]*drift/4096;
        ty+=ggExpCos[aim]*drift/4096;
        released.vx=(tx-released.x)/frames;
        released.vy=(ty-released.y)/frames;
        int drop=bow?GG_CHRON_ARROW_FALL*frames*(frames-1)/2:0;
        released.vz=(z-released.z+drop)/frames;
    }else if(bow)released.vz=210;
    *shot=released;
    *meter-=cost;
    ggChron.attackWait=charged?(bow?14:21):(bow?11:15);
    ggChron.attackFlash=GG_CHRON_STRIKE_FRAMES;
    ggChron.recoverySpan=ggChron.attackWait-GG_CHRON_STRIKE_FRAMES;
    ggChron.dirty=1;
    return 1;
}
static void ggChronCombatStow(void){
    ggChron.charge=ggChron.swing=ggChron.guardTicks=0;
    ggChron.attackFlash=ggChron.recoverySpan=0;
}
static void ggChronCombatStep(int raw,int down){
    int wasGuarding=ggChron.guardTicks!=0,priorCharge=ggChron.charge;
    if(ggChron.combatNoticeT>0)ggChron.combatNoticeT--;
    ggChronShotsStep();
    if((down&GG_3DS_ZR)&&!ggChron.swing){
        ggChron.style=(ggChron.style+1)%GG_CHRON_STYLES;
        ggChron.charge=ggChron.guardTicks=0;
        ggChron.attackFlash=ggChron.recoverySpan=0;
        if(ggChron.attackWait<5)ggChron.attackWait=5;
        ggChron.dirty=1;
    }
    if((raw&GG_3DS_ZL)&&(ggChron.guardTicks||(down&GG_3DS_ZL))){
        // Defense resumes when the attack settles, without a new parry window.
        int attacking=ggChron.style!=GG_CHRON_BOW&&(ggChron.swing||ggChron.charge);
        if(attacking&&ggChron.guardTicks<=8)ggChron.guardTicks=9;
        else if(ggChron.guardTicks<60)ggChron.guardTicks++;
    }else ggChron.guardTicks=0;

    if(ggChron.swing&&--ggChron.swing==0){
        int foe=ggChronCombatTarget(ggChron.heavy?410:336,1,NULL);
        int damage=(ggChron.heavy?31:16)+(ggChron.morale>=75?3:0);
        if(foe>=0)ggChronDamageFoe(foe,damage,ggChron.heavy);
        else ggChronCombatNotice(foe==GG_CHRON_TARGET_COVER?GG_COMBAT_COVER:GG_COMBAT_MISS);
        ggChron.attackFlash=GG_CHRON_STRIKE_FRAMES;
    }
    if(ggChron.charge&&!(raw&KEY_A)){
        if(ggChron.style==GG_CHRON_BLADE){
            ggChron.heavy=ggChron.charge>=ggChronChargeThreshold(ggChron.style);
            int cost=ggChronAttackCost(ggChron.style,ggChron.charge);
            if(ggChronCombatCanAfford(ggChron.style,cost)){
                ggChron.stamina-=cost;
                ggChron.swing=ggChron.heavy?6:3;
                ggChron.attackWait=ggChron.heavy?31:16;
                ggChron.recoverySpan=ggChron.attackWait-ggChron.swing-GG_CHRON_STRIKE_FRAMES;
            }
        }else ggChronReleaseShot(ggChron.style,ggChron.charge,
                                wasGuarding||ggChron.guardTicks);
        ggChron.charge=0;
    }
    if((down&KEY_A)&&!ggChron.charge){
        if(ggChron.attackWait)ggChronCombatNotice(GG_COMBAT_RECOVERY);
        else if(ggChronCombatCanAfford(ggChron.style,ggChronAttackCost(ggChron.style,1)))
            ggChron.charge=1;
    }else if((raw&KEY_A)&&ggChron.charge&&ggChron.charge<36)
        ggChron.charge++;

    if(!ggChron.charge&&!ggChron.guardTicks&&(ggChron.frame&7)==0&&ggChron.focus<100)
        ggChron.focus++;
    if(ggChron.swing||ggChron.charge!=priorCharge||ggChron.attackFlash||
       (ggChron.recoverySpan&&ggChron.attackWait>0&&!(ggChron.attackWait&1))||
       wasGuarding!=(ggChron.guardTicks!=0)||
       (ggChron.guardTicks>0&&ggChron.guardTicks<=8))
        ggChron.dirty=1;
}

#endif
