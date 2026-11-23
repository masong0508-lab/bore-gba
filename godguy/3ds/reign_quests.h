// Residents and Reign expeditions share the live settlements and courier work.
#ifndef GG_REIGN_QUESTS_H
#define GG_REIGN_QUESTS_H
#ifndef GG_QUEST_SAVE_PATH
#define GG_QUEST_SAVE_PATH "sdmc:/godguy_quests.dat"
#endif
enum { GG_QUEST_IDLE, GG_QUEST_FETCH, GG_QUEST_RETURN,
       GG_QUEST_CARGO=12, GG_QUEST_PAY=16, GG_QUEST_COMPLETE_MAX=99999 };
typedef struct { u32 location,identity; } GgQuestTown;
typedef struct {
    int loaded,stage,home,target,completed,pendingPay;
    u32 seed,size;
    GgQuestTown homeTown,targetTown;
    int talk,talkVillage,talkPerson,choice,lastKeys;
    char response[104];
} GgReignQuest;
static GgReignQuest ggQuest;
typedef struct {
    u32 magic,version,stage,home,target,completed,checksum;
} GgQuestDiskV1;
typedef struct {
    u32 magic,version,stage,home,target,completed,seed,size,slot;
    GgQuestTown homeTown,targetTown;
    u32 savedTick,savedRng,savedStocks,checksum;
} GgQuestDisk;
static u32 ggQuestChecksum(const void *data,size_t bytes){
    const u8 *p=data;
    u32 hash=2166136261u;
    for(size_t i=0;i<bytes;i++)hash=(hash^p[i])*16777619u;
    return hash;
}
static int ggQuestTownLive(int v){
    return (unsigned)v<VIL_N&&vil[v].on&&
           (unsigned)vil[v].x<WW&&(unsigned)vil[v].y<WH;
}
static GgQuestTown ggQuestTownIdentity(int v){
    GgQuestTown town={(u32)vil[v].x|((u32)vil[v].y<<16),
        (u32)vil[v].race|((u32)vil[v].nm1<<8)|((u32)vil[v].nm2<<16)};
    return town;
}
static int ggQuestTownMatches(int v,GgQuestTown expected){
    if(!ggQuestTownLive(v))return 0;
    GgQuestTown town=ggQuestTownIdentity(v);
    return town.location==expected.location&&town.identity==expected.identity;
}
static u32 ggQuestWorldSize(void){return (u32)WW|((u32)WH<<16);}
static int ggQuestRouteValid(void){
    return ggQuest.stage!=GG_QUEST_IDLE&&ggQuest.home!=ggQuest.target&&
           ggQuest.seed==worldSeed&&ggQuest.size==ggQuestWorldSize()&&
           ggQuestTownMatches(ggQuest.home,ggQuest.homeTown)&&
           ggQuestTownMatches(ggQuest.target,ggQuest.targetTown);
}
static int ggQuestObjective(void){
    if(!ggQuestRouteValid())return -1;
    return ggQuest.stage==GG_QUEST_FETCH?ggQuest.target:ggQuest.home;
}
static u32 ggQuestStocks(void){
    return (u32)vil[ggQuest.home].stock|((u32)vil[ggQuest.target].stock<<16);
}
static void ggQuestWorldReset(const char *path){
    int completed=ggQuest.completed;
    memset(&ggQuest,0,sizeof ggQuest);
    ggQuest.loaded=1;ggQuest.completed=completed;
    FILE *f=fopen(GG_QUEST_SAVE_PATH,"rb");
    if(!f)return;
    GgQuestDisk d={0};
    size_t bytes=fread(&d,1,sizeof d,f);
    fclose(f);
    if(d.magic!=0x47515154u)return;
    if(d.version==1&&bytes==sizeof(GgQuestDiskV1)){
        GgQuestDiskV1 old;
        memcpy(&old,&d,sizeof old);
        // Only the delivery history was bound to the player in v1.
        if(old.completed<=GG_QUEST_COMPLETE_MAX&&
           old.checksum==ggQuestChecksum(&old,sizeof old-sizeof old.checksum)&&
           old.completed>(u32)completed)ggQuest.completed=(int)old.completed;
        return;
    }
    if(d.version!=2||bytes!=sizeof d||d.completed>GG_QUEST_COMPLETE_MAX||
       d.checksum!=ggQuestChecksum(&d,sizeof d-sizeof d.checksum))return;
    if(d.completed>(u32)completed)ggQuest.completed=(int)d.completed;
    if(!path||d.slot!=ggQuestChecksum(path,strlen(path))||
       d.stage<GG_QUEST_FETCH||d.stage>GG_QUEST_RETURN||
       d.home>=VIL_N||d.target>=VIL_N)return;
    ggQuest.stage=(int)d.stage;ggQuest.home=(int)d.home;ggQuest.target=(int)d.target;
    ggQuest.seed=d.seed;ggQuest.size=d.size;
    ggQuest.homeTown=d.homeTown;ggQuest.targetTown=d.targetTown;
    if(!ggQuestRouteValid()||d.savedTick!=(u32)tick||d.savedRng!=rs||
       d.savedStocks!=ggQuestStocks())ggQuest.stage=GG_QUEST_IDLE;
}
static void ggQuestBegin(void){
    if(!ggQuest.loaded)ggQuestWorldReset(NULL);
    ggQuest.talk=0;ggQuest.choice=0;ggQuest.lastKeys=0;
    ggQuest.response[0]=0;
}
// A new cargo receipt may be written after the native save's actual stock
// debit is durable. Loading an older snapshot cannot recreate the cargo.
static int ggQuestSave(const char *path){
    GgQuestDisk d={0};
    d.magic=0x47515154u;d.version=2;d.completed=(u32)ggQuest.completed;
    if(path&&ggQuestRouteValid()){
        d.stage=(u32)ggQuest.stage;d.home=(u32)ggQuest.home;d.target=(u32)ggQuest.target;
        d.seed=ggQuest.seed;d.size=ggQuest.size;
        d.homeTown=ggQuest.homeTown;d.targetTown=ggQuest.targetTown;
        d.slot=ggQuestChecksum(path,strlen(path));
        d.savedTick=(u32)tick;d.savedRng=rs;d.savedStocks=ggQuestStocks();
    }
    d.checksum=ggQuestChecksum(&d,sizeof d-sizeof d.checksum);
    FILE *f=fopen(GG_QUEST_SAVE_PATH ".tmp","wb");
    if(!f)return 0;
    int ok=fwrite(&d,sizeof d,1,f)==1;
    if(fclose(f))ok=0;
    if(ok){
        remove(GG_QUEST_SAVE_PATH ".bak");
        rename(GG_QUEST_SAVE_PATH,GG_QUEST_SAVE_PATH ".bak");
        ok=rename(GG_QUEST_SAVE_PATH ".tmp",GG_QUEST_SAVE_PATH)==0;
        if(ok)remove(GG_QUEST_SAVE_PATH ".bak");
        else rename(GG_QUEST_SAVE_PATH ".bak",GG_QUEST_SAVE_PATH);
    }
    if(!ok)remove(GG_QUEST_SAVE_PATH ".tmp");
    return ok;
}
static void ggQuestWorldSaved(const char *path){
    if(!ggQuest.loaded)return;
    int delivered=ggQuest.pendingPay;
    int completed=ggQuest.completed;
    ggQuest.completed=ggExpClamp(completed+delivered,0,GG_QUEST_COMPLETE_MAX);
    if(!ggQuestSave(path)){
        ggQuest.completed=completed;
        return;
    }
    if(!delivered)return;
    // A native reload must never preserve pay while undoing the transfer.
    ggQuest.pendingPay=0;
    ggReignLoad();
    ggReign.credits=ggExpClamp(ggReign.credits+GG_QUEST_PAY*delivered,0,9999);
    ggReign.renown=ggExpClamp(ggReign.renown+6*delivered,0,9999);
    ggReign.influence=ggExpClamp(ggReign.influence+3*delivered,0,9999);
    ggReignSave();
}
static int ggQuestThreatNear(void){
    const int px=ggExplore.px>>8,py=ggExplore.py>>8;
    for(int i=0;i<ggChron.foeN;i++)if(ggChron.foe[i].hp>0&&
       ggChronNear(px,py,ggChron.foe[i].x,ggChron.foe[i].y)<=4)
        return 1;
    return 0;
}
static int ggQuestCitizenNear(void){
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    int chosen=-1,best=999;
    for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){
        int x=px+dx,y=py+dy;
        if((unsigned)x>=WW||(unsigned)y>=WH||!occ[y][x])continue;
        int i=(int)occ[y][x]-1;
        if(!ggHybridEligible(i)||crId[i]==ggExplore.actorId)continue;
        int dist=abs(dx)+abs(dy);
        if(dist<best){best=dist;chosen=i;}
    }
    return chosen;
}
static const signed char ggQuestDX[4]={1,0,-1,0},ggQuestDY[4]={0,1,0,-1};
static int ggQuestCivicTown(int x,int y){
    if((unsigned)x>=WW||(unsigned)y>=WH)return -1;
    int kind=bld[y][x];
    if(kind!=B_HALL&&kind!=B_MARKET&&kind!=B_WORKSHOP&&kind!=B_BARRACKS)
        return -1;
    int v=(int)bldv[y][x]-1;
    return ggQuestTownLive(v)?v:-1;
}
static int ggQuestLand(int x,int y,const Cr *actor){
    if(x<1||y<1||x>=WW-1||y>=WH-1)return 0;
    int terrain=world[y][x];
    return terrain>T_WATER&&terrain<T_N&&terrain!=T_LAVA&&
           ggExpBuildingWalkable(bld[y][x])&&
           (!actor||(ggHybridWalkable(actor,x,y)&&
                      (!occ[y][x]||((int)occ[y][x]<=nCr&&
                       crId[occ[y][x]-1]==ggExplore.actorId))));
}
static int ggQuestGentleStep(int x,int y,int nx,int ny){
    return abs(ggExpGround(world[y][x])-ggExpGround(world[ny][nx]))<=GG_WORLD_WALK_RISE;
}
static int ggQuestGatePass(int x,int y,int dx,int dy){
    int kind=bld[y][x];
    if(!ggBuildingIsGate(kind))return 1;
    if(!ggBuildingGateAllows(kind,dx,dy))return 0;
    // Sample the actual arch along its centerline, including both lips.
    for(int offset=-64;offset<=64;offset+=32)
        if(!ggExpGateStepAllowed(x*256+128+offset*dx,y*256+128+offset*dy))return 0;
    return 1;
}
static int ggQuestSurplus(int home,int source){
    // Sending a load narrows the difference without making the receiver
    // a new supplier. More work requires a real surplus to grow again.
    return (int)vil[source].stock>=(int)vil[home].stock+2*GG_QUEST_CARGO;
}
// Only acceptance needs a flood fill. Test both directions from the
// player's position to an actual civic doorstep, within the expedition grid.
static int ggQuestSource(int home){
    const int px=ggExplore.px>>8,py=ggExplore.py>>8;
    const int originX=px-16,originY=py-16;
    int actorIndex=ggHybridResolve();
    const Cr *actor=actorIndex>=0?&cr[actorIndex]:NULL;
    if(!ggQuestTownLive(home)||!ggQuestLand(px,py,actor))return -1;
    unsigned char reached[GG_CHRON_CELLS]={0};
    unsigned short queue[GG_CHRON_CELLS];
    const int root=16*GG_CHRON_GRID+16;
    int head=0,tail=0;
    reached[root]=1;queue[tail++]=(unsigned short)root;
    while(head<tail){
        int p=queue[head++];
        int cx=p%GG_CHRON_GRID,cy=p/GG_CHRON_GRID;
        for(int k=0;k<4;k++){
            int dx=ggQuestDX[k],dy=ggQuestDY[k];
            int nx=cx+dx,ny=cy+dy;
            if((unsigned)nx>=GG_CHRON_GRID||
               (unsigned)ny>=GG_CHRON_GRID)continue;
            int i=ny*GG_CHRON_GRID+nx;
            int wx=originX+nx,wy=originY+ny;
            int fromX=originX+cx,fromY=originY+cy;
            int v=ggQuestCivicTown(wx,wy);
            if(v>=0&&v!=home&&ggQuestSurplus(home,v)&&
               ggChronNear(vil[home].x,vil[home].y,vil[v].x,vil[v].y)>=7&&
               ggQuestGentleStep(fromX,fromY,wx,wy)&&
               ggQuestGatePass(fromX,fromY,dx,dy))return v;
            if(reached[i]||!ggQuestLand(wx,wy,actor)||
               !ggQuestGentleStep(fromX,fromY,wx,wy)||
               !ggQuestGatePass(fromX,fromY,dx,dy)||!ggQuestGatePass(wx,wy,dx,dy))continue;
            reached[i]=1;queue[tail++]=(unsigned short)i;
        }
    }
    return -1;
}
static void ggQuestResponse(const char *line){
    snprintf(ggQuest.response,sizeof ggQuest.response,"%.90s",line);
}
static void ggQuestUseTown(void){
    int v=ggQuest.talkVillage;
    if(!ggQuestTownLive(v)){
        ggQuestResponse("A traveler has stories, but no town contract.");
        return;
    }
    if(ggQuest.stage!=GG_QUEST_IDLE&&!ggQuestRouteValid()){
        ggQuest.stage=GG_QUEST_IDLE;
        ggQuestResponse("Contract closed. Its settlements have changed.");
        return;
    }
    if(ggQuest.stage==GG_QUEST_IDLE){
        int dest=ggQuestSource(v);
        if(dest<0){
            ggQuestResponse("No safe route to a town with spare supplies.");
            return;
        }
        ggQuest.home=v;ggQuest.target=dest;
        ggQuest.homeTown=ggQuestTownIdentity(v);
        ggQuest.targetTown=ggQuestTownIdentity(dest);
        ggQuest.seed=worldSeed;ggQuest.size=ggQuestWorldSize();
        ggQuest.stage=GG_QUEST_FETCH;
        ggQuestResponse("Contract accepted. Visit the source settlement.");
    }else if(ggQuest.stage==GG_QUEST_FETCH){
        if(v!=ggQuest.target){
            ggQuestResponse("Find the source town shown in your journal.");
        }else if(!ggQuestSurplus(ggQuest.home,v)){
            ggQuestResponse("This village needs its supplies. Return later.");
        }else{
            ggReignVillageStock(v,-GG_QUEST_CARGO);
            ggQuest.stage=GG_QUEST_RETURN;
            ggQuestResponse("Cargo collected. Save your world to keep it.");
        }
    }else if(v!=ggQuest.home){
        ggQuestResponse("Your delivery belongs to the original town.");
    }else if(vil[v].stock>65535-GG_QUEST_CARGO){
        ggQuestResponse("The town's granary is full. Return later.");
    }else{
        ggReignVillageStock(v,GG_QUEST_CARGO);
        ggReignVillageLoyalty(v,5);
        ggQuest.pendingPay=ggExpClamp(ggQuest.pendingPay+1,0,GG_QUEST_COMPLETE_MAX);
        ggQuest.stage=GG_QUEST_IDLE;
        ggQuestResponse("DELIVERED 12 SUPPLIES. SAVE FOR 16 CROWNS.");
    }
}
static void ggQuestOfferRelief(void){
    int v=ggQuest.talkVillage;
    if(!ggQuestTownLive(v)){ggQuestResponse("Find a living village to share supplies.");return;}
    if(ggChron.rations<1){
        ggQuestResponse("No spare rations. Find supplies on the road.");
        return;
    }
    if(vil[v].stock>=65527){
        ggQuestResponse("Their granaries are already filled.");return;
    }
    // Real, finite item cost and actual native-world effects.
    // Does not farm coins or endorsements through repeated clicks.
    ggChron.rations--;
    ggReignVillageStock(v,8);
    ggReignVillageLoyalty(v,3);
    ggChron.morale=ggExpClamp(ggChron.morale+3,0,100);
    ggChron.dirty=1;
    ggQuestResponse("Your meal feeds families. Their trust grows.");
}
static void ggQuestAboutTown(void){
    int v=ggQuest.talkVillage;
    if(ggQuestTownLive(v)){
        if(ggQuest.stage==GG_QUEST_FETCH&&v==ggQuestObjective())
            ggQuestResponse("Our caravan has the supplies you requested.");
        else if(ggQuest.stage==GG_QUEST_RETURN&&v==ggQuestObjective())
            ggQuestResponse("The town awaits its missing shipment.");
        else if(vil[v].stock<20)
            ggQuestResponse("Our children need food before new laws.");
        else if(vilL[v]<-15)
            ggQuestResponse("People here do not trust their governors.");
        else if(ggReign.infamy>ggReign.legitimacy+20)
            ggQuestResponse("Your old raids make promises hard to trust.");
        else if(ggReign.role==GG_ROLE_HEALER)
            ggQuestResponse("Our families remember your care for the ill.");
        else if(ggReign.role==GG_ROLE_FARMER)
            ggQuestResponse("Your harvest keeps these households alive.");
        else if(ggReign.office>=2)
            ggQuestResponse("You hold office, but the people still vote.");
        else if(ggQuest.completed>0)
            ggQuestResponse("Your deliveries connected our neighbors.");
        else ggQuestResponse("These farms and families make our world.");
    }else if(ggQuest.talkPerson>=0)
        ggQuestResponse("I have a home and a life. Keep our roads safe.");
    else ggQuestResponse("The roads hold stories; towns offer work.");
}
static int ggQuestTalkTarget(int *town,int *person){
    int x,y;
    ggExpTarget(&x,&y);
    *town=ggQuestCivicTown(x,y);
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    for(int i=0;i<4&&*town<0;i++){
        int dx=ggQuestDX[i],dy=ggQuestDY[i];
        x=px+dx;y=py+dy;
        int v=ggQuestCivicTown(x,y);
        if(v>=0&&ggQuestGentleStep(px,py,x,y)&&ggQuestGatePass(px,py,dx,dy))
            *town=v;
    }
    *person=ggQuestCitizenNear();
    return *town>=0||*person>=0;
}
static int ggQuestAimedAtFoe(void){
    return ggChron.style!=GG_CHRON_BLADE&&
           ggChronCombatTarget((ggChron.style==GG_CHRON_BOW?8:6)*256,0,NULL)>=0;
}
static void ggQuestOpenTown(int town,int person){
    ggQuest.talk=1;ggQuest.choice=0;
    ggQuest.talkVillage=town;ggQuest.talkPerson=person;
    ggQuestResponse(town>=0?"Ask about courier work or local life.":
                            "A life is more than a job or a prophecy.");
}
static int ggQuestWalkActionAt(int x,int y){
    return ggExplore.active&&!ggChron.active&&ggQuestCivicTown(x,y)>=0&&
           abs(x-(ggExplore.px>>8))<=3&&abs(y-(ggExplore.py>>8))<=3;
}
static int ggQuestWalkOpen(int x,int y){
    if(!ggQuestWalkActionAt(x,y))return 0;
    int aimX,aimY;
    ggExpTarget(&aimX,&aimY);
    if(aimX!=x||aimY!=y)return 0;
    if(!ggQuest.loaded)ggQuestBegin();
    ggReignLoad();
    ggQuestOpenTown(ggQuestCivicTown(x,y),-1);
    return 1;
}
static void ggQuestTalkStep(int down,int choices){
    if(down&(KEY_B|KEY_SELECT|KEY_START)){
        ggQuest.talk=0;return;
    }
    if(down&KEY_UP)ggQuest.choice=(ggQuest.choice+choices-1)%choices;
    if(down&KEY_DOWN)ggQuest.choice=(ggQuest.choice+1)%choices;
    if(down&KEY_A){
        if(ggQuest.choice==0)ggQuestUseTown();
        else if(ggQuest.choice==1)ggQuestAboutTown();
        else if(choices==4&&ggQuest.choice==2)ggQuestOfferRelief();
        else ggQuest.talk=0;
    }
}
static int ggQuestWalkStep(int raw){
    int down=raw&~ggQuest.lastKeys;
    ggQuest.lastKeys=raw;
    if(!ggQuest.talk)return 0;
    ggQuestTalkStep(down,3);
    return 1;
}
static int ggQuestStep(int raw){
    int down=raw&~ggQuest.lastKeys;
    ggQuest.lastKeys=raw;
    if(ggQuest.talk){ggQuestTalkStep(down,4);return 1;}
    if(!ggChron.active||ggChron.won||ggChron.lost||ggReign.menu||
       ggChron.briefTime||ggChron.guardTicks||!(down&KEY_A)||ggQuestThreatNear())
        return 0;
    int town,person;
    if((raw&GG_3DS_ZL)||!ggQuestTalkTarget(&town,&person)||ggQuestAimedAtFoe())return 0;
    ggQuestOpenTown(town,person);
    return 1;
}
static const char *ggQuestHint(char *out,size_t size,const char *prefix){
    int v=ggQuestObjective();
    if(v<0)return NULL;
    char name[24];vilName(v,name);
    int dx=(int)vil[v].x-(ggExplore.px>>8),dy=(int)vil[v].y-(ggExplore.py>>8);
    const char *bearing=abs(dx)>abs(dy)?(dx>0?"E":"W"):(dy>0?"S":"N");
    const char *action=ggQuest.stage==GG_QUEST_FETCH?"COLLECT":"DELIVER";
    if(dx||dy)snprintf(out,size,"%s%s AT %s  %s %d",prefix,action,name,bearing,abs(dx)+abs(dy));
    else snprintf(out,size,"%s%s AT %s",prefix,action,name);
    return out;
}
static const char *ggQuestWalkHint(char *out,size_t size){
    const char *hint=ggQuestHint(out,size,"");
    if(hint||!ggQuest.pendingPay)return hint;
    snprintf(out,size,"SAVE WORLD FOR %d COURIER CROWNS",GG_QUEST_PAY*ggQuest.pendingPay);
    return out;
}
static const char *ggQuestFieldHint(char *out,size_t size){
    if(ggChron.briefTime||ggChron.guardTicks||ggQuestThreatNear())return NULL;
    int town,person;
    int canTalk=ggQuestTalkTarget(&town,&person)&&!ggQuestAimedAtFoe();
    const char *hint=ggQuestHint(out,size,
        canTalk&&town>=0&&town==ggQuestObjective()?"A WORK  ":"");
    return hint?hint:canTalk?"A TALK":ggQuestWalkHint(out,size);
}
static void ggQuestJournalDraw(u8 *out){
    char line[100];
    int v=ggQuestObjective();
    if(v<0){
        ggExpText(out,25,79,"NO ACTIVE CONTRACT",229,222,175);
        ggExpText(out,25,98,"PRESS A AT A SAFE TOWN HALL FOR WORK.",189,214,221);
    }else{
        ggQuestWalkHint(line,sizeof line);
        ggExpText(out,25,79,line,232,221,176);
        snprintf(line,sizeof line,"GO TO %d,%d  FROM CURRENT %d,%d",
                 vil[v].x,vil[v].y,ggExplore.px>>8,ggExplore.py>>8);
        ggExpText(out,25,98,line,200,217,229);
        ggExpText(out,25,116,"PRESS A AT A CIVIC BUILDING IN TOWN.",189,214,221);
    }
    snprintf(line,sizeof line,"COMPLETED DELIVERIES: %d",ggQuest.completed);
    ggExpText(out,25,132,line,221,199,153);
    if(ggQuest.pendingPay){
        snprintf(line,sizeof line,"SAVE WORLD FOR %d COURIER CROWNS",GG_QUEST_PAY*ggQuest.pendingPay);
        ggExpText(out,25,148,line,221,199,153);
    }
}
static void ggQuestDraw(u8 *out){
    if(!ggExplore.active||!ggQuest.talk||
       (ggChron.active&&(ggOrigin.active||ggReign.menu)))return;
    char line[92];
    int choices=ggChron.active?4:3;
    int v=ggQuest.talkVillage;
    ggExpRect(out,18,54,364,75+choices*16,15,28,42);
    ggExpRect(out,18,54,364,3,206,163,94);
    if(ggQuestTownLive(v)){
        char name[24];vilName(v,name);
        snprintf(line,sizeof line,"%s - TOWN WORK",name);
    }else if(ggHybridEligible(ggQuest.talkPerson)){
        char name[48];crName(&cr[ggQuest.talkPerson],name);
        snprintf(line,sizeof line,"TALKING TO %.37s",name);
    }else snprintf(line,sizeof line,"TRAVELER");
    ggExpText(out,29,64,line,244,218,168);
    ggExpText(out,29,84,ggQuest.response,214,230,224);
    for(int i=0;i<choices;i++){
        const char *label=i==0?"COURIER WORK":i==1?"ASK ABOUT THIS PLACE":
                          i==choices-1?"GOODBYE":"OFFER ONE RATION";
        int chosen=i==ggQuest.choice;
        snprintf(line,sizeof line,"%c %s",chosen?'>':' ',label);
        ggExpText(out,29,107+i*16,line,chosen?255:177,chosen?222:202,168);
    }
    ggExpText(out,29,113+choices*16,"UP/DOWN CHOOSE   A SPEAK   B EXIT",171,205,231);
}
#endif
