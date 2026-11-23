// Context actions for free walking and citizen POV. All targets and rewards
// come from the same terrain, creatures, buildings and stores as the map.
#ifndef GG_OUTDOOR3D_H
#define GG_OUTDOOR3D_H

enum { GG_OUTDOOR_NONE, GG_OUTDOOR_PERSON, GG_OUTDOOR_WILDLIFE,
       GG_OUTDOOR_FOOD, GG_OUTDOOR_FARM, GG_OUTDOOR_REST,
       GG_OUTDOOR_DOCK, GG_OUTDOOR_SITE, GG_OUTDOOR_LANDSCAPE };
typedef struct { int kind,x,y,person; const char *site; } GgOutdoorTarget;

static char ggOutdoorNotice[64];
static int ggOutdoorNoticeUntil,ggOutdoorCaptureA;
static int ggOutdoorWaypointX=-1,ggOutdoorWaypointY=-1;
static int ggOutdoorBreath,ggOutdoorWinded;

static void ggOutdoorReset(void){
    ggOutdoorNotice[0]=0;
    ggOutdoorNoticeUntil=ggOutdoorCaptureA=0;
    ggOutdoorWaypointX=ggOutdoorWaypointY=-1;
    ggOutdoorBreath=100;
    ggOutdoorWinded=0;
}

static void ggOutdoorSay(const char *text){
    snprintf(ggOutdoorNotice,sizeof ggOutdoorNotice,"%s",text);
    ggOutdoorNoticeUntil=ggExplore.frame+175;
}

static void ggOutdoorSetWaypoint(int x,int y){
    if((x==ggOutdoorWaypointX&&y==ggOutdoorWaypointY)||
       (x==(ggExplore.px>>8)&&y==(ggExplore.py>>8))){
        ggOutdoorWaypointX=ggOutdoorWaypointY=-1;
        ggOutdoorSay("WAYPOINT CLEARED");
        return;
    }
    if(x<1||y<1||x>=WW-1||y>=WH-1||world[y][x]<=T_WATER||world[y][x]==T_LAVA){
        ggOutdoorSay("MARK A PLACE ON LAND");
        return;
    }
    ggOutdoorWaypointX=x;ggOutdoorWaypointY=y;
    ggOutdoorSay("WAYPOINT MARKED");
}

static int ggOutdoorPace(int keys){
    if(!(keys&KEY_B))ggOutdoorWinded=0;
    else if(ggOutdoorBreath<8)ggOutdoorWinded=1;
    return ggOutdoorWinded?keys&~KEY_B:keys;
}

static void ggOutdoorMoveFeedback(int sprint,int oldX,int oldY){
    int moved=ggExplore.px!=oldX||ggExplore.py!=oldY;
    if(sprint&&moved)ggOutdoorBreath=ggExpClamp(ggOutdoorBreath-2,0,100);
    else if(!sprint&&!(ggExplore.frame&1)&&ggOutdoorBreath<100)ggOutdoorBreath++;
    if(ggOutdoorWaypointX>=0&&abs(ggOutdoorWaypointX-(ggExplore.px>>8))+
                              abs(ggOutdoorWaypointY-(ggExplore.py>>8))<=1){
        ggOutdoorWaypointX=ggOutdoorWaypointY=-1;
        ggOutdoorSay("WAYPOINT REACHED");
    }
}

static GgOutdoorTarget ggOutdoorTargetAt(int x,int y){
    GgOutdoorTarget target={GG_OUTDOOR_NONE,x,y,-1,NULL};
    if((unsigned)x>=WW||(unsigned)y>=WH)return target;
    int who=(int)occ[y][x]-1;
    if(who>=0&&who<nCr&&cr[who].alive&&crId[who]!=ggExplore.actorId&&
       (unsigned)cr[who].sp<(unsigned)nSp){
        target.person=who;
        target.kind=ggHybridEligible(who)?GG_OUTDOOR_PERSON:GG_OUTDOOR_WILDLIFE;
        return target;
    }
    int building=bld[y][x],terrain=world[y][x];
    if(ggBuildingIsPaving(building)){
        target.kind=GG_OUTDOOR_LANDSCAPE;
        target.site=ggBuildingName(building);
    }
    else if(building==B_FARM)target.kind=GG_OUTDOOR_FARM;
    else if(building==B_HALL||ggBuildingIsHome(building)||ggBuildingIsWing(building)||
            building==B_TEMPLE||building==B_INN){
        target.kind=GG_OUTDOOR_REST;
        if(building==B_INN)target.site=ggBuildingName(building);
    }
    else if(building==B_DOCK)target.kind=GG_OUTDOOR_DOCK;
    else{
        if(building==B_RUIN)target.site="OLD RUIN";
        else if(building==B_TOWER)target.site="WATCHTOWER";
        else if(building==B_STATUE)target.site="STONE MARKER";
        else if(building==B_LIBRARY)target.site="LIBRARY";
        else if(building>=B_WORKSHOP&&building<B_N)target.site=ggBuildingName(building);
        else if(terrain==T_MOUNT||terrain==T_SNOW||terrain==T_HILL)target.site="RIDGE";
        else if(terrain==T_SAND||terrain==T_WATER||terrain==T_DEEP)target.site="WATER'S EDGE";
        else if(terrain==T_FOREST||terrain==T_JUNGLE||terrain==T_SWAMP)target.site="FORAGING GROUND";
        else if(terrain==T_GRASS||terrain==T_DIRT||terrain==T_DESERT)target.site="TRAIL";
        if(ggExplore.actorId&&!building&&
           (terrain==T_FOREST||terrain==T_JUNGLE||terrain==T_SWAMP))
            target.kind=GG_OUTDOOR_FOOD;
        else if(target.site)target.kind=building?GG_OUTDOOR_SITE:GG_OUTDOOR_LANDSCAPE;
    }
    return target;
}

static int ggOutdoorTown(int x,int y){
    int v=(int)bldv[y][x]-1;
    return v>=0&&v<VIL_N&&vil[v].on?v:-1;
}

static void ggOutdoorSurvey(const GgOutdoorTarget *target){
    char entry[34],message[64];
    snprintf(entry,sizeof entry,"%.17s AT %d,%d",target->site,target->x,target->y);
    int recorded=0;
    for(int i=0;i<jrN;i++) if(!strncmp(jr[i],entry,sizeof jr[i])){ recorded=1;break; }
    if(!recorded)logEv(entry);

    int x=ggExpClamp(target->x*8-128,0,WW*8-256);
    int y=ggExpClamp(target->y*8-96,0,WH*8-192);
    int freePin=-1;
    for(int i=0;i<MAP_PIN_N;i++){
        if(!(mapPinMask&(1u<<i))){
            if(freePin<0)freePin=i;
        }else if(mapPin[i][0]==x&&mapPin[i][1]==y){
            snprintf(message,sizeof message,"%s / ALREADY MARKED",entry);
            ggOutdoorSay(message);
            return;
        }
    }
    if(freePin>=0){
        mapPin[freePin][0]=(u16)x;
        mapPin[freePin][1]=(u16)y;
        mapPinMask|=(u8)(1u<<freePin);
    }
    snprintf(message,sizeof message,"%s / %s",entry,freePin>=0?"MAP PIN":"JOURNAL");
    ggOutdoorSay(message);
    fxReq(FX_INSPECT);
}

static int ggOutdoorAtWar(const Cr *actor,int village){
    int people=sp[actor->sp].a,destination=vil[village].race;
    if((unsigned)people>=8||(unsigned)destination>=8)return 0;
    int home=vilNear(people,actor->x,actor->y,20);
    if(home>=0&&home!=village&&
       (vil[home].foe==village+1||vil[village].foe==home+1))return 1;
    return peopleVillagesAtWar(people,destination,home,village);
}

static int ggOutdoorFeed(Cr *actor,int amount){
    enum { FULL=120 };
    int room=FULL-actor->food;
    if(room<=0)return 0;
    if(amount>room)amount=room;
    actor->food+=(u8)amount;
    return amount;
}

static void ggOutdoorTalk(const GgOutdoorTarget *target,Cr *actor){
    Cr *resident=&cr[target->person];
    char name[48],village[32],message[64];
    crName(resident,name);
    int v=vilNear(sp[resident->sp].a,resident->x,resident->y,16);
    if(v<0){ snprintf(message,sizeof message,"%.31s / TRAVELER",name); }
    else{
        vilName(v,village);
        const char *status=vil[v].foe?"ON WATCH":vil[v].stock<16?"NEEDS FOOD":"HAS SUPPLIES";
        if(actor&&ggOutdoorAtWar(actor,v))status="GUARDS THE BORDER";
        snprintf(message,sizeof message,"%.20s / %.17s %s",name,village,status);
    }
    ggOutdoorSay(message);
    fxReq(FX_INSPECT);
}

static void ggOutdoorWatch(const GgOutdoorTarget *target,Cr *actor){
    Cr *animal=&cr[target->person];
    char name[48],message[64];
    crName(animal,name);
    int diet=arch[sp[animal->sp].a].diet;
    if(actor&&actor->food>=22&&animal->food<90&&(diet==0||diet==3)){
        actor->food-=8;
        ggOutdoorFeed(animal,15);
        snprintf(message,sizeof message,"%.36s ACCEPTS YOUR FOOD",name);
        fxReq(FX_FEED);
    }else snprintf(message,sizeof message,"WILDLIFE / %.39s",name);
    ggOutdoorSay(message);
}

static void ggOutdoorStores(const GgOutdoorTarget *target,Cr *actor){
    int v=ggOutdoorTown(target->x,target->y);
    if(v<0&&bld[target->y][target->x]==B_INN){
        if(!actor){ ggOutdoorSurvey(target);return; }
        int need=maxHp(actor)-actor->hp;
        if(need<=0){ ggOutdoorSay("YOU ARE ALREADY RESTED");return; }
        int rations=ggExpClamp((int)actor->food-16,0,6);
        if(!rations){ ggOutdoorSay("YOU NEED PROVISIONS TO REST HERE");return; }
        int heal=need<rations*3?need:rations*3;
        actor->food-=(u8)((heal+2)/3);
        actor->hp+=(u8)heal;
        ggOutdoorSay("YOU SHARE YOUR PROVISIONS AND REST AT THE INN");
        fxReq(FX_HEAL);
        return;
    }
    if(v<0){ ggOutdoorSay("THIS BUILDING HAS NO VILLAGE STORES");return; }
    if(!actor){
        char village[32],message[64];
        vilName(v,village);
        snprintf(message,sizeof message,"%.29s / %u SUPPLIES",village,(unsigned)vil[v].stock);
        ggOutdoorSay(message);
        return;
    }
    if(ggOutdoorAtWar(actor,v)){ ggOutdoorSay("THE VILLAGE IS ON WAR FOOTING");return; }
    int available=(int)vil[v].stock-6;
    if(available<=0){ ggOutdoorSay("THE VILLAGE NEEDS ITS LAST SUPPLIES");return; }

    if(target->kind==GG_OUTDOOR_REST){
        int need=maxHp(actor)-actor->hp;
        if(need<=0){ ggOutdoorSay("YOU ARE ALREADY RESTED");return; }
        int spend=available<8?available:8;
        int restored=spend*3<need?spend*3:need;
        actor->hp+=(u8)restored;
        vil[v].stock-=(u16)((restored+2)/3);
        ggOutdoorSay("YOU REST AT THE VILLAGE AND RECOVER HEALTH");
        fxReq(FX_HEAL);
    }else{
        int ration=available>10?20:available*2;
        int fed=ggOutdoorFeed(actor,ration);
        if(!fed){ ggOutdoorSay("YOUR FOOD IS ALREADY FULL");return; }
        vil[v].stock-=(u16)((fed+1)/2);
        ggOutdoorSay("YOU EAT FROM THE FARM'S STORES");
        fxReq(FX_FEED);
    }
}

static int ggOutdoorLanding(int x,int y,const Cr *actor,int *landX,int *landY){
    for(int d=0;d<4;d++){
        int nx=x+dxs[d],ny=y+dys[d];
        if(nx<1||ny<1||nx>=WW-1||ny>=WH-1||occ[ny][nx])continue;
        int building=bld[ny][nx];
        if(building&&!ggBuildingIsPaving(building))continue;
        int t=world[ny][nx];
        if(t<=T_WATER||t==T_LAVA||t==T_MOUNT||(unsigned)t>=T_N)continue;
        if(actor&&!ggHybridWalkable(actor,nx,ny))continue;
        *landX=nx;
        *landY=ny;
        return 1;
    }
    return 0;
}

// Run only on an explicit dock interaction while the board is paused.
// The settler-boat search uses these scratch arrays during simulation.
static int ggOutdoorFindFerry(int sx,int sy,const Cr *actor,int *landX,int *landY){
    enum { R=GG_FERRY_RADIUS, W=R*2+1, N=W*W };
    _Static_assert(N<=sizeof bfsDir&&N<=sizeof bfsQ/sizeof *bfsQ,
                   "Ferry search fits the boat workspace");
    int ox=sx-R,oy=sy-R,head=0,tail=0;
    memset(bfsDir,0xff,N);
    for(int d=0;d<4;d++){
        int x=sx+dxs[d],y=sy+dys[d];
        if(!isWater(x,y))continue;
        int tile=(y-oy)*W+x-ox;
        bfsDir[tile]=1;
        bfsQ[tail++]=(u16)tile;
    }
    while(head<tail){
        int at=bfsQ[head++],lx=at%W,ly=at/W;
        int distance=bfsDir[at];
        for(int d=0;d<4;d++){
            int nx=lx+dxs[d],ny=ly+dys[d];
            int x=ox+nx,y=oy+ny;
            if((unsigned)nx>=W||(unsigned)ny>=W||
               (unsigned)x>=WW||(unsigned)y>=WH)continue;
            if(!isWater(x,y)){
                if(distance>=2&&bld[y][x]==B_DOCK&&(x!=sx||y!=sy)){
                    int v=ggOutdoorTown(x,y);
                    if((v<0||!actor||!ggOutdoorAtWar(actor,v))&&
                       ggOutdoorLanding(x,y,actor,landX,landY))return 1;
                }
                continue;
            }
            int next=ny*W+nx;
            if(bfsDir[next]!=0xff||distance>=R+12)continue;
            bfsDir[next]=(u8)(distance+1);
            bfsQ[tail++]=(u16)next;
        }
    }
    return 0;
}

static void ggOutdoorFerry(const GgOutdoorTarget *target,Cr *actor){
    int v=ggOutdoorTown(target->x,target->y);
    if(actor&&v>=0&&ggOutdoorAtWar(actor,v)){
        ggOutdoorSay("THE DOCK IS CLOSED TO YOUR VILLAGE");return;
    }
    if(actor&&actor->food<20){
        ggOutdoorSay("YOU NEED FOOD FOR THE CROSSING");return;
    }
    int x,y;
    if(!ggOutdoorFindFerry(target->x,target->y,actor,&x,&y)){
        ggOutdoorSay("NO REACHABLE DOCK IN THIS STRETCH OF WATER");return;
    }
    if(actor){
        actor->food-=10;
        moveTo(actor,(int)(actor-cr),x,y);
    }
    ggExplore.px=x*256+128;
    ggExplore.py=y*256+128;
    ggExplore.stepPhase=0;
    winX=-99;
    ggOutdoorSay("THE FERRY LANDS AT THE OPPOSITE DOCK");
    fxReq(FX_RAIN);
}

static void ggOutdoorUseAt(int x,int y){
    ggExplore.feedbackT=0;
    GgOutdoorTarget target=ggOutdoorTargetAt(x,y);
    int who=ggHybridResolve();
    Cr *actor=who>=0?&cr[who]:NULL;
    switch(target.kind){
    case GG_OUTDOOR_PERSON: ggOutdoorTalk(&target,actor); break;
    case GG_OUTDOOR_WILDLIFE: ggOutdoorWatch(&target,actor); break;
    case GG_OUTDOOR_FOOD:
        if(actor&&ggOutdoorFeed(actor,18)){ ggOutdoorSay("YOU FORAGE A HANDFUL OF WILD FOOD");fxReq(FX_FEED); }
        else ggOutdoorSay("YOU ARE ALREADY FULL");
        break;
    case GG_OUTDOOR_FARM: case GG_OUTDOOR_REST: ggOutdoorStores(&target,actor); break;
    case GG_OUTDOOR_DOCK: ggOutdoorFerry(&target,actor); break;
    case GG_OUTDOOR_SITE: ggOutdoorSurvey(&target); break;
    case GG_OUTDOOR_LANDSCAPE:{
        char message[55];
        snprintf(message,sizeof message,"%s AT %d,%d",target.site,target.x,target.y);
        ggOutdoorSay(message);
        break;
    }
    default: ggOutdoorSay("WALK CLOSER OR AIM AT A SITE"); break;
    }
}

static void ggOutdoorHud(u8 *out,const char *contract,const char *interaction,int sculpt){
    static const char *const action[]={
        "LOOK", "TALK", "WATCH WILDLIFE", "FORAGE", "FARM STORES",
        "REST", "TAKE FERRY", "SURVEY AND PIN", "READ TERRAIN"
    };
    GgOutdoorTarget target=ggOutdoorTargetAt(ggExplore.lastTargetX,ggExplore.lastTargetY);
    char hint[56];
    if(!sculpt){
        if(contract){
            snprintf(hint,sizeof hint,"%.49s",contract);
            ggExpText(out,9,217,hint,178,219,224);
        }
        else if(ggOutdoorWaypointX>=0){
            int dx=ggOutdoorWaypointX-(ggExplore.px>>8);
            int dy=ggOutdoorWaypointY-(ggExplore.py>>8);
            const char *bearing=abs(dx)>abs(dy)?(dx>0?"EAST":"WEST"):(dy>0?"SOUTH":"NORTH");
            snprintf(hint,sizeof hint,"WAYPOINT %s %d",bearing,abs(dx)+abs(dy));
            ggExpText(out,9,217,hint,178,219,224);
        }else{
            int i=ggHybridResolve();
            if(i>=0)snprintf(hint,sizeof hint,"HEALTH %d/%d   FOOD %u",
                cr[i].hp,maxHp(&cr[i]),(unsigned)cr[i].food);
            else snprintf(hint,sizeof hint,"TAP THE MAP TO MARK A PLACE");
            ggExpText(out,9,217,hint,178,219,224);
        }
        if(ggOutdoorBreath<100){
            ggExpRect(out,318,219,72,3,47,61,70);
            ggExpRect(out,318,219,72*ggOutdoorBreath/100,3,215,173,83);
        }
    }
    snprintf(hint,sizeof hint,"%s  %s",sculpt?"L+A":"A",interaction?interaction:action[target.kind]);
    if(ggExplore.feedbackT&&(ggExplore.feedback==4||ggExplore.feedback==5))
        ggExpText(out,9,230,ggExplore.feedback==4?"WORLD SAVED":"SAVE FAILED",241,225,177);
    else if(!ggExplore.feedbackT&&ggExplore.frame<ggOutdoorNoticeUntil)
        ggExpText(out,9,230,ggOutdoorNotice,241,225,177);
    else ggExpText(out,9,230,hint,150,224,207);
}
#endif
