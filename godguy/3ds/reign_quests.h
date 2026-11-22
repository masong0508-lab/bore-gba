// The Unwritten Reign: small, real-world RPG conversation / courier loop.
// A speaks to real nearby citizens or settlements only when no hostile is
// close enough to demand combat. The council's fifth page is a journal.
// No duplicate map, asset atlas, heap allocations or extra framebuffers.
#ifndef GG_REIGN_QUESTS_H
#define GG_REIGN_QUESTS_H
#ifndef GG_QUEST_SAVE_PATH
#define GG_QUEST_SAVE_PATH "sdmc:/godguy_quests.dat"
#endif
enum { GG_QUEST_IDLE, GG_QUEST_FETCH, GG_QUEST_RETURN,
       GG_QUEST_CARGO=12 };
typedef struct {
    int loaded,stage,home,target,completed;
    int talk,talkVillage,talkPerson,choice,lastKeys;
    char response[104];
} GgReignQuest;
static GgReignQuest ggQuest;
typedef struct {
    u32 magic,version,stage,home,target,completed,checksum;
} GgQuestDisk;
static u32 ggQuestChecksum(const GgQuestDisk *d){
    const u8 *p=(const u8*)d;
    u32 hash=2166136261u;
    for(unsigned i=0;i<sizeof(*d)-sizeof(d->checksum);i++)
        hash=(hash^p[i])*16777619u;
    return hash;
}
static void ggQuestBegin(void){
    if(!ggQuest.loaded){
        ggQuest.loaded=1;ggQuest.stage=GG_QUEST_IDLE;
        FILE *f=fopen(GG_QUEST_SAVE_PATH,"rb");
        if(f){
            GgQuestDisk d;
            int ok=fread(&d,sizeof d,1,f)==1;
            fclose(f);
            if(ok&&d.magic==0x47515154u&&d.version==1&&
               d.checksum==ggQuestChecksum(&d)&&d.stage<=GG_QUEST_RETURN&&
               d.home<VIL_N&&d.target<VIL_N&&d.completed<100000){
                ggQuest.stage=(int)d.stage;
                ggQuest.home=(int)d.home;ggQuest.target=(int)d.target;
                ggQuest.completed=(int)d.completed;
            }
        }
    }
    ggQuest.talk=0;ggQuest.choice=0;ggQuest.lastKeys=0;
    ggQuest.response[0]=0;
}
static void ggQuestSave(void){
    GgQuestDisk d;
    d.magic=0x47515154u;d.version=1;
    d.stage=(u32)ggQuest.stage;d.home=(u32)ggQuest.home;
    d.target=(u32)ggQuest.target;d.completed=(u32)ggQuest.completed;
    d.checksum=ggQuestChecksum(&d);
    FILE *f=fopen(GG_QUEST_SAVE_PATH,"wb");
    if(f){(void)fwrite(&d,sizeof d,1,f);fclose(f);}
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
// One flood fill at job acceptance, NOT every gameplay frame. This
// refuses impossible sea/mountain crossings and follows the real map.
// The 33x33 bound matches the expedition's existing navigation budget.
static int ggQuestSource(int home){
    const int px=ggExplore.px>>8,py=ggExplore.py>>8;
    const int originX=px-16,originY=py-16;
    unsigned char reached[GG_CHRON_CELLS]={0};
    unsigned short queue[GG_CHRON_CELLS];
    const int root=16*GG_CHRON_GRID+16;
    int head=0,tail=0;
    reached[root]=1;queue[tail++]=(unsigned short)root;
    while(head<tail){
        int p=queue[head++];
        int cx=p%GG_CHRON_GRID,cy=p/GG_CHRON_GRID;
        static const signed char dx[4]={1,0,-1,0},dy[4]={0,1,0,-1};
        for(int k=0;k<4;k++){
            int nx=cx+dx[k],ny=cy+dy[k];
            if((unsigned)nx>=GG_CHRON_GRID||
               (unsigned)ny>=GG_CHRON_GRID)continue;
            int i=ny*GG_CHRON_GRID+nx;
            int wx=originX+nx,wy=originY+ny;
            int fromX=originX+cx,fromY=originY+cy;
            if(reached[i]||!ggChronLand(wx,wy))continue;
            // Courier routes must obey the same step-up limit as the
            // player's first-person movement. Otherwise a mountain
            // wall can produce a "reachable" delivery that is actually
            // impossible to complete on foot.
            if(ggExpGround(world[wy][wx])-
               ggExpGround(world[fromY][fromX])>19)continue;
            reached[i]=1;queue[tail++]=(unsigned short)i;
        }
    }
    int nearest=-1,best=0x7fffffff;
    for(int v=0;v<VIL_N;v++)if(v!=home&&vil[v].on&&
                               vil[v].stock>=GG_QUEST_CARGO){
        int dx=(int)vil[v].x-px,dy=(int)vil[v].y-py;
        if(abs(dx)+abs(dy)<7||abs(dx)>17||abs(dy)>17)continue;
        int accessible=0;
        for(int yy=-4;yy<=4&&!accessible;yy++)
            for(int xx=-4;xx<=4;xx++){
                int rx=dx+xx+16,ry=dy+yy+16;
                if(abs(xx)+abs(yy)>4||
                   (unsigned)rx>=GG_CHRON_GRID||
                   (unsigned)ry>=GG_CHRON_GRID)continue;
                if(reached[ry*GG_CHRON_GRID+rx]){accessible=1;break;}
            }
        if(!accessible)continue;
        int cost=abs(dx)+abs(dy);
        if(cost<best){best=cost;nearest=v;}
    }
    return nearest;
}
static void ggQuestResponse(const char *line){
    snprintf(ggQuest.response,sizeof ggQuest.response,"%.90s",line);
}
static void ggQuestUseTown(void){
    int v=ggQuest.talkVillage;
    if(v<0){
        ggQuestResponse("A traveler has stories, but no town contract.");
        return;
    }
    if(ggQuest.stage==GG_QUEST_IDLE){
        int dest=ggQuestSource(v);
        if(dest<0){
            ggQuestResponse("No safe, stocked route nearby. Explore onward.");
            return;
        }
        ggQuest.home=v;ggQuest.target=dest;
        ggQuest.stage=GG_QUEST_FETCH;
        ggQuestResponse("Contract accepted. Visit the source settlement.");
        ggQuestSave();
    }else if(ggQuest.stage==GG_QUEST_FETCH){
        if(v!=ggQuest.target){
            ggQuestResponse("Find the source town shown in your journal.");
        }else if(vil[v].stock<GG_QUEST_CARGO){
            ggQuestResponse("This village lacks supplies. Return later.");
        }else{
            ggReignVillageStock(v,-GG_QUEST_CARGO);
            ggQuest.stage=GG_QUEST_RETURN;
            ggQuestResponse("Cargo collected. Bring it to the first town.");
            ggQuestSave();
        }
    }else if(v!=ggQuest.home){
        ggQuestResponse("Your delivery belongs to the original town.");
    }else{
        ggReignVillageStock(v,GG_QUEST_CARGO);
        ggReignVillageLoyalty(v,5);
        ggReign.credits=ggExpClamp(ggReign.credits+16,0,9999);
        ggReign.renown=ggExpClamp(ggReign.renown+6,0,9999);
        ggReign.influence=ggExpClamp(ggReign.influence+3,0,9999);
        ggQuest.completed++;
        ggQuest.stage=GG_QUEST_IDLE;
        ggQuestResponse("Delivered! Food, goodwill and pay are restored.");
        ggQuestSave();ggReignSave();
    }
}
static void ggQuestOfferRelief(void){
    int v=ggQuest.talkVillage;
    if(v<0){ggQuestResponse("Find a living village to share supplies.");return;}
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
    if(v>=0){
        if(ggQuest.stage==GG_QUEST_FETCH&&v==ggQuest.target)
            ggQuestResponse("Our caravan has the supplies you requested.");
        else if(ggQuest.stage==GG_QUEST_RETURN&&v==ggQuest.home)
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
static int ggQuestStep(int raw){
    int down=raw&~ggQuest.lastKeys;
    ggQuest.lastKeys=raw;
    if(ggQuest.talk){
        if(down&(KEY_B|KEY_SELECT|KEY_START)){
            ggQuest.talk=0;return 1;
        }
        if(down&KEY_UP)ggQuest.choice=(ggQuest.choice+3)%4;
        if(down&KEY_DOWN)ggQuest.choice=(ggQuest.choice+1)%4;
        if(down&KEY_A){
            if(ggQuest.choice==0)ggQuestUseTown();
            else if(ggQuest.choice==1)ggQuestAboutTown();
            else if(ggQuest.choice==2)ggQuestOfferRelief();
            else ggQuest.talk=0;
        }
        return 1; // gameplay/enemies freeze while talking
    }
    if(!ggChron.active||ggChron.won||ggChron.lost||ggReign.menu||
       ggChron.briefTime||!(down&KEY_A)||ggQuestThreatNear())
        return 0;
    if((raw&GG_3DS_ZL)||(ggChron.style!=GG_CHRON_BLADE&&
       ggChronCombatTarget((ggChron.style==GG_CHRON_BOW?8:6)*256,0,NULL)>=0))
        return 0;
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    int town=ggReignVillage(px,py,6);
    int person=ggQuestCitizenNear();
    if(town<0&&person<0)return 0;
    ggQuest.talk=1;ggQuest.choice=0;
    ggQuest.talkVillage=town;ggQuest.talkPerson=person;
    ggQuestResponse("A life is more than a job or a prophecy.");
    return 1;
}
static void ggQuestJournalDraw(u8 *out){
    char line[100];
    if(ggQuest.stage==GG_QUEST_IDLE){
        ggExpText(out,25,79,"NO ACTIVE CONTRACT",229,222,175);
        ggExpText(out,25,98,"VISIT A SAFE VILLAGE; PRESS A TO TALK.",189,214,221);
        ggExpText(out,25,116,"YOUR JOURNEY AND CAREER ARE YOUR OWN.",189,214,221);
    }else{
        int v=ggQuest.stage==GG_QUEST_FETCH?ggQuest.target:ggQuest.home;
        snprintf(line,sizeof line,"DELIVERY %s   SETTLEMENT %d",
                 ggQuest.stage==GG_QUEST_FETCH?"COLLECT":"RETURN",v+1);
        ggExpText(out,25,79,line,232,221,176);
        snprintf(line,sizeof line,"GO TO %d,%d  FROM CURRENT %d,%d",
                 vil[v].x,vil[v].y,ggExplore.px>>8,ggExplore.py>>8);
        ggExpText(out,25,98,line,200,217,229);
        ggExpText(out,25,116,"PRESS A IN THE DESTINATION TO TALK.",189,214,221);
    }
    snprintf(line,sizeof line,"COMPLETED DELIVERIES: %d",ggQuest.completed);
    ggExpText(out,25,132,line,221,199,153);
}
static void ggQuestDraw(u8 *out){
    if(!ggChron.active||ggOrigin.active||ggReign.menu)return;
    char line[92];
    if(ggQuest.talk){
        ggExpRect(out,18,54,364,139,15,28,42);
        ggExpRect(out,18,54,364,3,206,163,94);
        if(ggQuest.talkPerson>=0){
            char name[48];
            crName(&cr[ggQuest.talkPerson],name);
            snprintf(line,sizeof line,"TALKING TO %.37s",name);
        }else if(ggQuest.talkVillage>=0)
            snprintf(line,sizeof line,"SETTLEMENT %d - LOCAL VOICES",
                     ggQuest.talkVillage+1);
        else snprintf(line,sizeof line,"TRAVELER");
        ggExpText(out,29,64,line,244,218,168);
        ggExpText(out,29,84,ggQuest.response,214,230,224);
        ggExpText(out,29,107,ggQuest.choice==0?"> WORK / DELIVERY":"  WORK / DELIVERY",
                  ggQuest.choice==0?255:177,ggQuest.choice==0?222:202,168);
        ggExpText(out,29,123,ggQuest.choice==1?"> ASK ABOUT THIS PLACE":"  ASK ABOUT THIS PLACE",
                  ggQuest.choice==1?255:177,ggQuest.choice==1?222:202,168);
        ggExpText(out,29,139,ggQuest.choice==2?"> OFFER ONE RATION":"  OFFER ONE RATION",
                  ggQuest.choice==2?255:177,ggQuest.choice==2?222:202,168);
        ggExpText(out,29,155,ggQuest.choice==3?"> GOODBYE":"  GOODBYE",
                  ggQuest.choice==3?255:177,ggQuest.choice==3?222:202,168);
        ggExpText(out,29,177,"UP/DOWN CHOOSE   A SPEAK   B EXIT",171,205,231);
        return;
    }
    if(ggReign.noticeT)return; // no overlap with important game notice
    if(ggQuest.stage!=GG_QUEST_IDLE){
        int v=ggQuest.stage==GG_QUEST_FETCH?ggQuest.target:ggQuest.home;
        if(v>=VIL_N||!vil[v].on)return;
        int dx=(int)vil[v].x-(ggExplore.px>>8);
        int dy=(int)vil[v].y-(ggExplore.py>>8);
        int distance=abs(dx)+abs(dy);
        const char *bearing=abs(dx)>abs(dy)?(dx>=0?"EAST":"WEST"):
                                               (dy>=0?"SOUTH":"NORTH");
        snprintf(line,sizeof line,"DELIVERY: %s TO TOWN %d  %s %d TILES",
                 ggQuest.stage==GG_QUEST_FETCH?"COLLECT":"RETURN",
                 v+1,bearing,distance);
    }else if(ggReignVillage(ggExplore.px>>8,ggExplore.py>>8,6)>=0&&
             !ggQuestThreatNear())
        snprintf(line,sizeof line,"A: TALK TO LOCAL PEOPLE  -  SELECT: QUEST JOURNAL");
    else return;
    ggExpRect(out,8,157,384,15,18,34,47);
    ggExpText(out,14,161,line,233,210,158);
}
#endif
