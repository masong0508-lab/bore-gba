// GodGuy: The Unwritten Reign (3DS only).
// Profession, elected office, and godborn abilities are deliberately
// independent. This playable side campaign uses the REAL generated villages,
// village stores and loyalties, and the 3D expedition's existing foe positions.
// The 2D story and DS/DSi builds have no additional data or rendering cost.
#ifndef GG_UNWRITTEN_REIGN_H
#define GG_UNWRITTEN_REIGN_H
#ifndef GG_REIGN_SAVE_PATH
#define GG_REIGN_SAVE_PATH "sdmc:/godguy_unwritten_reign.dat"
#endif

enum { GG_REIGN_ROLES=13, GG_REIGN_PAGES=6, GG_REIGN_ORDERS=3,
       GG_REIGN_SQUAD_MAX=6, GG_REIGN_OFFICES=5 };
enum { GG_ROLE_WAYFINDER,GG_ROLE_SHADE,GG_ROLE_TRADER,
       GG_ROLE_SOLDIER,GG_ROLE_ENVOY,GG_ROLE_MYSTIC,
       GG_ROLE_FARMER,GG_ROLE_ARTISAN,GG_ROLE_HEALER,
       GG_ROLE_SCHOLAR,GG_ROLE_SAILOR,GG_ROLE_WATCH,
       GG_ROLE_UNBOUND };
enum { GG_REIGN_FOLLOW,GG_REIGN_HOLD,GG_REIGN_ASSAULT };
typedef struct {short x,y; signed char hp; } GgReignGuard;
typedef struct {
    int loaded,role,choice,menu,page,action,order,selectedOrder;
    int credits,renown,influence,infamy,legitimacy,divine,office;
    int endorsements,lastKeys,frame,noticeT,combatTick;
    int squadN,initialChoice,nextCivicAt;
    unsigned char support[(VIL_N+7)/8];
    GgReignGuard squad[GG_REIGN_SQUAD_MAX];
    char notice[74];
} GgReign3D;
static GgReign3D ggReign;
static const char *const ggReignRoles[GG_REIGN_ROLES]={
    "WAYFINDER","SHADE","TRADER","SOLDIER","ENVOY","MYSTIC",
    "FARMER","ARTISAN","HEALER","SCHOLAR","SAILOR","WATCHMAN",
    "UNAFFILIATED"
};
static const char *const ggReignOffices[GG_REIGN_OFFICES]={
    "CITIZEN","COUNCILLOR","MAYOR","GOVERNOR","PRESIDENT"
};
static const char *const ggReignOrders[GG_REIGN_ORDERS]={
    "FOLLOW","HOLD","ASSAULT"
};
static const char *const ggReignPages[GG_REIGN_PAGES]={
    "LOCAL WORK","WAR COUNCIL","CIVIC HALL","DIVINE SPARK",
    "QUEST JOURNAL","FATE AND ENDINGS"
};
static const char *const ggReignCivicActions[3]={
    "CLASS DEED","GAIN ENDORSEMENT","SEEK OFFICE"
};
static const char *const ggReignPowers[3]={
    "GODBOLT","MENDING","BATTLE AWE"
};
typedef struct {
    u32 magic,version,role,chosen,credits,renown,influence,infamy;
    u32 legitimacy,divine,office,endorsements;
    unsigned char support[(VIL_N+7)/8];
    u32 checksum;
} GgReignDisk;
static u32 ggReignChecksum(const GgReignDisk *d){
    const unsigned char *p=(const unsigned char *)d;
    u32 h=2166136261u;
    // Only meaningful fields are hashed: potential struct padding does not
    // affect persistence and is never read as gameplay state.
    for(unsigned i=0;i<sizeof(*d)-sizeof(d->checksum);i++)
        h=(h^p[i])*16777619u;
    return h;
}
static void ggReignNotice(const char *m){
    snprintf(ggReign.notice,sizeof ggReign.notice,"%.70s",m);
    ggReign.noticeT=160;
}
static void ggReignLoad(void){
    if(ggReign.loaded)return;
    ggReign.loaded=1;
    // The new 22-year-old character is an ordinary civilian until
    // they seek a profession in a real generated settlement.
    ggReign.role=GG_ROLE_UNBOUND;
    ggReign.choice=GG_ROLE_UNBOUND;
    ggReign.credits=35;
    ggReign.divine=100;
    ggReign.legitimacy=10;
    FILE *f=fopen(GG_REIGN_SAVE_PATH,"rb");
    if(!f)return;
    GgReignDisk d;
    memset(&d,0,sizeof d);
    int valid=fread(&d,sizeof d,1,f)==1;
    fclose(f);
    if(!valid||d.magic!=0x47475245u||d.version!=1||
       d.checksum!=ggReignChecksum(&d)||d.role>=GG_REIGN_ROLES||
       d.office>=GG_REIGN_OFFICES||d.endorsements>VIL_N)return;
    ggReign.role=(int)d.role;
    ggReign.choice=ggReign.role;
    ggReign.initialChoice=(int)(d.chosen!=0);
    ggReign.credits=ggExpClamp((int)d.credits,0,9999);
    ggReign.renown=ggExpClamp((int)d.renown,0,9999);
    ggReign.influence=ggExpClamp((int)d.influence,0,9999);
    ggReign.infamy=ggExpClamp((int)d.infamy,0,9999);
    ggReign.legitimacy=ggExpClamp((int)d.legitimacy,0,9999);
    ggReign.divine=ggExpClamp((int)d.divine,0,100);
    ggReign.office=(int)d.office;
    ggReign.endorsements=(int)d.endorsements;
    memcpy(ggReign.support,d.support,sizeof ggReign.support);
}
static void ggReignSave(void){
    if(!ggReign.loaded)return;
    GgReignDisk d;
    memset(&d,0,sizeof d);
    d.magic=0x47475245u;d.version=1;
    d.role=(u32)ggReign.role;d.chosen=(u32)ggReign.initialChoice;
    d.credits=(u32)ggReign.credits;d.renown=(u32)ggReign.renown;
    d.influence=(u32)ggReign.influence;d.infamy=(u32)ggReign.infamy;
    d.legitimacy=(u32)ggReign.legitimacy;d.divine=(u32)ggReign.divine;
    d.office=(u32)ggReign.office;d.endorsements=(u32)ggReign.endorsements;
    memcpy(d.support,ggReign.support,sizeof ggReign.support);
    d.checksum=ggReignChecksum(&d);
    FILE *f=fopen(GG_REIGN_SAVE_PATH,"wb");
    if(!f)return;
    (void)fwrite(&d,sizeof d,1,f);
    fclose(f);
}
static int ggReignVillage(int x,int y,int radius){
    int chosen=-1,best=radius*radius+1;
    // Only on explicit civic interaction, never on a rendered frame.
    for(int i=0;i<VIL_N;i++)if(vil[i].on){
        int dx=(int)vil[i].x-x,dy=(int)vil[i].y-y;
        int distance=dx*dx+dy*dy;
        if(distance<best){best=distance;chosen=i;}
    }
    return chosen;
}
static void ggReignVillageStock(int v,int amount){
    int stock=ggExpClamp((int)vil[v].stock+amount,0,65535);
    vil[v].stock=(u16)stock;
    KD_DIRTY();
}
static void ggReignVillageLoyalty(int v,int amount){
    vilL[v]=(s8)ggExpClamp((int)vilL[v]+amount,-100,100);
    KD_DIRTY();
}
static int ggReignEnroll(int role){
    if(role<0||role>=GG_REIGN_ROLES||role==GG_ROLE_UNBOUND){
        ggReignNotice("CHOOSE A PROFESSION, OR REMAIN UNAFFILIATED");return 0;
    }
    if(ggReignVillage(ggExplore.px>>8,ggExplore.py>>8,12)<0){
        ggReignNotice("FIND A LIVING TOWN TO SEEK A JOB");return 0;
    }
    if(role==ggReign.role){
        ggReignNotice("YOU ALREADY WORK IN THIS PROFESSION");return 1;
    }
    if(ggReign.initialChoice){
        if(ggReign.credits<15){
            ggReignNotice("RETRAINING NEEDS 15 CROWNS");return 0;
        }
        ggReign.credits-=15;
    }
    ggReign.role=role;
    ggReign.initialChoice=1;
    ggReign.renown=ggExpClamp(ggReign.renown+2,0,9999);
    ggReignNotice("NEW JOB CONTRACT - YOUR FUTURE REMAINS OPEN");
    ggReignSave();
    return 1;
}
static int ggReignRecruit(void){
    int v=ggReignVillage(ggExplore.px>>8,ggExplore.py>>8,12);
    if(v<0){ggReignNotice("RECRUIT AT A REAL SETTLEMENT");return 0;}
    if(ggReign.squadN>=GG_REIGN_SQUAD_MAX){
        ggReignNotice("YOUR FIELD SQUAD IS AT CAPACITY");return 0;
    }
    int price=ggReign.role==GG_ROLE_SOLDIER?8:15;
    if(ggReign.credits<price||vil[v].stock<5){
        ggReignNotice("NEED FUNDS AND VILLAGE PROVISIONS");return 0;
    }
    const int x=ggExplore.px>>8,y=ggExplore.py>>8;
    static const signed char ox[8]={1,-1,0,0,2,-2,1,-1};
    static const signed char oy[8]={0,0,1,-1,1,-1,-2,2};
    for(int i=0;i<8;i++){
        int nx=x+ox[i],ny=y+oy[i];
        if(!ggChronLand(nx,ny))continue;
        int occupied=0;
        for(int k=0;k<ggReign.squadN;k++)
            if(ggReign.squad[k].x==nx&&ggReign.squad[k].y==ny)
                occupied=1;
        if(occupied)continue;
        ggReign.credits-=price;
        ggReignVillageStock(v,-5);
        ggReign.squad[ggReign.squadN++]=(GgReignGuard){
            (short)nx,(short)ny,45};
        ggReignNotice("RECRUITED A FOLLOWER FROM THE REAL TOWN");
        ggReignSave();
        return 1;
    }
    ggReignNotice("NO SAFE PLACE FOR A RECRUIT");
    return 0;
}
static int ggReignDeed(void){
    if(ggReign.frame<ggReign.nextCivicAt){
        ggReignNotice("WORK AND POLITICS REQUIRE PASSING WORLD TIME");return 0;
    }
    int v=ggReignVillage(ggExplore.px>>8,ggExplore.py>>8,12);
    if(v<0){ggReignNotice("FIND A SETTLEMENT TO CHANGE ITS FATE");return 0;}
    switch(ggReign.role){
    case GG_ROLE_WAYFINDER:
        ggReign.credits+=5;ggReign.renown+=4;
        ggReignNotice("GUIDED A CARAVAN THROUGH REAL TERRAIN");break;
    case GG_ROLE_SHADE:
        if(vil[v].stock<8){ggReignNotice("THIS TOWN HAS NOTHING LEFT TO STEAL");return 0;}
        ggReignVillageStock(v,-8);ggReignVillageLoyalty(v,-5);
        ggReign.credits+=19;ggReign.infamy+=9;
        ggReign.legitimacy=ggExpClamp(ggReign.legitimacy-6,0,9999);
        ggReignNotice("HEIST: WEALTH GAINED, PUBLIC TRUST LOST");break;
    case GG_ROLE_TRADER:
        if(ggReign.credits<8){ggReignNotice("TRADE REQUIRES 8 CROWNS");return 0;}
        ggReign.credits+=4;ggReign.influence+=4;
        ggReignVillageStock(v,12);ggReignVillageLoyalty(v,2);
        ggReignNotice("MERCHANT CONVOY RESTOCKED THE SETTLEMENT");break;
    case GG_ROLE_SOLDIER:return ggReignRecruit();
    case GG_ROLE_ENVOY:
        if(ggReign.credits<8){ggReignNotice("CIVIC PROJECT REQUIRES 8 CROWNS");return 0;}
        ggReign.credits-=8;ggReign.influence+=9;
        ggReign.legitimacy+=6;
        ggReignVillageStock(v,8);ggReignVillageLoyalty(v,4);
        ggReignNotice("NEGOTIATED CIVIC SUPPORT AND RELIEF");break;
    case GG_ROLE_MYSTIC:
        if(ggReign.divine<20){ggReignNotice("DIVINE SPARK TOO WEAK");return 0;}
        ggReign.divine-=20;ggReign.influence+=5;
        ggReign.legitimacy+=6;
        ggReignVillageStock(v,14);ggReignVillageLoyalty(v,3);
        ggReignNotice("A GODBORN BLESSING FED REAL HOUSEHOLDS");break;
    case GG_ROLE_FARMER:
        if(world[ggExplore.py>>8][ggExplore.px>>8]!=T_GRASS&&
           world[ggExplore.py>>8][ggExplore.px>>8]!=T_DIRT&&
           world[ggExplore.py>>8][ggExplore.px>>8]!=T_FOREST){
            ggReignNotice("FARMING NEEDS ARABLE LAND");return 0;
        }
        ggReign.credits+=7;ggReign.renown+=2;
        ggReignVillageStock(v,18);ggReignVillageLoyalty(v,2);
        ggReignNotice("HARVEST SOLD - REAL LOCAL FOOD STOCK ROSE");break;
    case GG_ROLE_ARTISAN:
        if(vil[v].stock<6){ggReignNotice("WORKSHOP NEEDS RAW MATERIALS");return 0;}
        ggReignVillageStock(v,-6);
        ggReign.credits+=13;ggReign.renown+=3;
        ggReignNotice("WORKSHOP CRAFTED TOOLS FOR THE MARKET");break;
    case GG_ROLE_HEALER:
        if(ggReign.credits<3){ggReignNotice("MEDICINES NEED 3 CROWNS");return 0;}
        ggReign.credits-=3;ggReign.legitimacy+=5;
        ggReignVillageLoyalty(v,5);
        ggReign.renown+=4;ggChron.hp=ggExpClamp(ggChron.hp+16,0,100);
        ggReignNotice("HEALER'S CLINIC AIDED YOUR COMMUNITY");break;
    case GG_ROLE_SCHOLAR:
        if(vil[v].stock<2){ggReignNotice("A SCHOLAR NEEDS TOWN SUPPLIES");return 0;}
        ggReignVillageStock(v,-2);
        ggReign.influence+=6;ggReign.renown+=3;
        ggReignNotice("SCHOLAR DOCUMENTED LOCAL HISTORY AND LAWS");break;
    case GG_ROLE_SAILOR: {
        int coast=0;
        for(int dy=-4;dy<=4;dy++)
            for(int dx=-4;dx<=4;dx++){
                int x=(ggExplore.px>>8)+dx,y=(ggExplore.py>>8)+dy;
                if((unsigned)x<WW&&(unsigned)y<WH&&
                   world[y][x]<=T_WATER)coast=1;
            }
        if(!coast){ggReignNotice("SAILING NEEDS A REAL COAST OR RIVER");return 0;}
        ggReign.credits+=11;ggReign.renown+=3;
        ggReignVillageStock(v,7);
        ggReignNotice("BOAT WORK DELIVERED FOOD TO THE PORT");break;
    }
    case GG_ROLE_WATCH:
        ggReign.credits+=8;ggReign.renown+=4;
        ggReignVillageLoyalty(v,3);
        ggReignNotice("WATCH DUTY KEPT THE HOMESTEAD SAFE");break;
    default:
        ggReign.credits+=3;
        ggReignNotice("HONEST ODD JOBS: NO CLASS REQUIRED");break;
    }
    ggReign.nextCivicAt=ggReign.frame+95;
    ggReignSave();
    return 1;
}
static int ggReignPetition(void){
    int v=ggReignVillage(ggExplore.px>>8,ggExplore.py>>8,12);
    if(v<0){ggReignNotice("NEED A LIVING VILLAGE TO SEEK TRUST");return 0;}
    if(ggReign.support[v>>3]&(1u<<(v&7))){
        ggReignNotice("THIS SETTLEMENT ALREADY ENDORSED YOU");return 0;
    }
    if(ggReign.credits<10||ggReign.influence<12||
       ggReign.legitimacy<10||ggReign.infamy>ggReign.legitimacy+30){
        ggReignNotice("EARN INFLUENCE, TRUST, AND 10 CROWNS");return 0;
    }
    ggReign.credits-=10;
    ggReign.support[v>>3]|=(unsigned char)(1u<<(v&7));
    ggReign.endorsements++;
    ggReignVillageStock(v,8);
    ggReignVillageLoyalty(v,8);
    ggReign.influence+=9;
    ggReign.legitimacy+=6;
    ggReignNotice("THIS GENERATED SETTLEMENT HAS ENDORSED YOU");
    ggReignSave();
    return 1;
}
static int ggReignElection(void){
    static const int neededInfluence[GG_REIGN_OFFICES]={0,12,35,70,105};
    static const int neededTrust[GG_REIGN_OFFICES]={0,8,25,55,80};
    static const int neededTowns[GG_REIGN_OFFICES]={0,1,2,3,4};
    if(ggReign.office>=GG_REIGN_OFFICES-1){
        ggReignNotice("PRESIDENT: HIGHEST ELECTED OFFICE HELD");return 0;
    }
    if(ggReignVillage(ggExplore.px>>8,ggExplore.py>>8,12)<0){
        ggReignNotice("STAND FOR OFFICE IN A REAL SETTLEMENT");return 0;
    }
    int next=ggReign.office+1;
    if(ggReign.influence<neededInfluence[next]||
       ggReign.legitimacy<neededTrust[next]||
       ggReign.endorsements<neededTowns[next]||
       (next==4&&ggChron.cleared<2)){
        ggReignNotice("ELECTION NEEDS MORE TRUST, TOWNS AND REPUTATION");
        return 0;
    }
    ggReign.office=next;
    ggReign.renown+=15*next;
    ggReignNotice(ggReignOffices[next]);
    ggReignSave();
    return 1;
}
static int ggReignDivine(int spell){
    int cost=spell==0?20:spell==1?30:40;
    if(ggReign.divine<cost){ggReignNotice("DIVINE ENERGY MUST RECHARGE");return 0;}
    if(spell==0){
        int best=-1,dist=99999;
        int a=ggExplore.angle&255;
        for(int i=0;i<ggChron.foeN;i++){
            GgChronFoe *f=&ggChron.foe[i];
            if(f->hp<=0)continue;
            int vx=((int)f->x<<8)+128-ggExplore.px;
            int vy=((int)f->y<<8)+128-ggExplore.py;
            int ahead=(vx*ggExpCos[a]+vy*ggExpSin[a])/4096;
            int side=(-vx*ggExpSin[a]+vy*ggExpCos[a])/4096;
            if(ahead<15||ahead>8*256||abs(side)>ahead/2+60||
               !ggExpLineClear(ggExplore.px>>8,ggExplore.py>>8,
                               f->x,f->y))continue;
            if(ahead<dist){best=i;dist=ahead;}
        }
        if(best<0){ggReignNotice("NO VISIBLE ENEMY IN DIVINE RANGE");return 0;}
        ggChron.foe[best].hp-=40;
        ggChron.score+=20;ggChron.dirty=1;
        ggReignNotice("GODBOLT STRUCK AN ENEMY WITHOUT HARMING CIVILIANS");
    }else if(spell==1){
        if(ggChron.hp>=100){ggReignNotice("YOUR BODY IS ALREADY MENDED");return 0;}
        ggChron.hp=ggExpClamp(ggChron.hp+38,0,100);
        ggReignNotice("GODBORN MENDING RESTORED YOUR STRENGTH");
    }else{
        ggChron.morale=ggExpClamp(ggChron.morale+24,0,100);
        for(int i=0;i<ggChron.foeN;i++)if(ggChron.foe[i].hp>0&&
           ggChronNear(ggExplore.px>>8,ggExplore.py>>8,
                       ggChron.foe[i].x,ggChron.foe[i].y)<8)
            ggChron.foe[i].wait=70;
        ggReignNotice("BATTLE AWE RALLIED YOUR ALLIES AND HALTED FOES");
    }
    ggReign.divine-=cost;
    ggReign.renown+=2;
    ggReignSave();
    return 1;
}
static void ggReignBegin(void){
    ggReignLoad();
    ggReign.menu=0;ggReign.page=0;ggReign.action=0;
    ggReign.choice=ggReign.role;ggReign.selectedOrder=ggReign.order;
    ggReign.frame=0;ggReign.lastKeys=0;ggReign.squadN=0;
    ggReign.nextCivicAt=0;
    ggReignNotice("YOU ARE GODBORN. YOUR SOCIAL DESTINY IS NOT FIXED.");
}
static int ggReignStep(int raw){
    int down=raw&~ggReign.lastKeys;
    ggReign.lastKeys=raw;
    if(ggReign.noticeT)ggReign.noticeT--;
    if(!ggReign.menu){
        if(down&KEY_SELECT){
            ggReign.menu=1;
            ggReign.page=0;ggReign.action=0;
            ggReign.choice=ggReign.role;
            ggReign.selectedOrder=ggReign.order;
            return 1;
        }
        return 0;
    }
    if(down&KEY_START)return 0; // honour emergency retreat everywhere
    if(down&(KEY_B|KEY_SELECT)){ggReign.menu=0;return 1;}
    if(down&KEY_LEFT){
        ggReign.page=(ggReign.page+GG_REIGN_PAGES-1)%GG_REIGN_PAGES;
        ggReign.action=0;
    }
    if(down&KEY_RIGHT){
        ggReign.page=(ggReign.page+1)%GG_REIGN_PAGES;
        ggReign.action=0;
    }
    if(down&KEY_X){
        if(ggReign.page==0)
            ggReign.choice=(ggReign.choice+1)%GG_REIGN_ROLES;
        else if(ggReign.page==1)
            ggReign.selectedOrder=(ggReign.selectedOrder+1)%GG_REIGN_ORDERS;
        else if(ggReign.page<4)ggReign.action=(ggReign.action+1)%3;
    }
    if(down&KEY_Y&&ggReign.page==1)ggReignRecruit();
    if(down&KEY_A){
        switch(ggReign.page){
        case 0:ggReignEnroll(ggReign.choice);break;
        case 1:
            ggReign.order=ggReign.selectedOrder;
            ggReignNotice("FIELD SQUAD ORDER ISSUED");
            break;
        case 2:
            if(ggReign.action==0)ggReignDeed();
            else if(ggReign.action==1)ggReignPetition();
            else ggReignElection();
            break;
        case 3:ggReignDivine(ggReign.action);break;
        default:break; // journal is read-only, never casts divine spells
        }
    }
    return 1; // 3D world is paused while the command board is open
}
static void ggReignSquadTick(void){
    if(!ggChron.active||ggChron.won||ggChron.lost||ggReign.menu)return;
    ggReign.frame++;
    if(ggReign.divine<100&&ggReign.frame%28==0)ggReign.divine++;
    if(ggReign.frame%12)return;
    for(int i=0;i<ggReign.squadN;i++){
        GgReignGuard *g=&ggReign.squad[i];
        if(g->hp<=0)continue;
        int enemy=-1,distance=999;
        for(int j=0;j<ggChron.foeN;j++){
            if(ggChron.foe[j].hp<=0)continue;
            int d=ggChronNear(g->x,g->y,ggChron.foe[j].x,ggChron.foe[j].y);
            if(d<distance){enemy=j;distance=d;}
        }
        if(enemy>=0&&distance<=2){
            // Soldiers contribute bounded damage, creating a real
            // tactical advantage without spawning fake 2D world citizens.
            ggChron.foe[enemy].hp-=4+(ggReign.order==GG_REIGN_ASSAULT?3:0);
            ggChron.dirty=1;
            continue;
        }
        int tx=ggExplore.px>>8,ty=ggExplore.py>>8;
        if(ggReign.order==GG_REIGN_HOLD)continue;
        if(ggReign.order==GG_REIGN_ASSAULT&&enemy>=0&&distance<8){
            tx=ggChron.foe[enemy].x;ty=ggChron.foe[enemy].y;
        }
        int stepX=tx>g->x?1:tx<g->x?-1:0;
        int stepY=ty>g->y?1:ty<g->y?-1:0;
        int nx=g->x,ny=g->y;
        if(abs(tx-g->x)>abs(ty-g->y)&&ggChronLand(nx+stepX,ny))
            nx+=stepX;
        else if(ggChronLand(nx,ny+stepY))ny+=stepY;
        else if(ggChronLand(nx+stepX,ny))nx+=stepX;
        if(nx==tx&&ny==ty)continue;
        int blocked=0;
        for(int k=0;k<ggReign.squadN;k++)if(i!=k&&
            ggReign.squad[k].hp>0&&ggReign.squad[k].x==nx&&
            ggReign.squad[k].y==ny)blocked=1;
        if(!blocked){g->x=(short)nx;g->y=(short)ny;}
    }
}
// The journal is supplied by the lightweight quest module included after
// this header. It shares the existing modal, without adding a screen buffer.
static void ggQuestJournalDraw(u8 *out);
static void ggSagaFateDraw(u8 *out);
static void ggReignDraw(u8 *out){
    if(!ggChron.active)return;
    // Officers and companions use tiny native-game silhouettes with
    // physically positioned banners; there is no block-built army.
    for(int i=0;i<ggReign.squadN;i++){
        const GgReignGuard *g=&ggReign.squad[i];
        if(g->hp<=0)continue;
        int x,y,size;
        if(!ggChronProject(g->x,g->y,&x,&y,&size))continue;
        int scale=ggExpClamp(size/8,1,4);
        int frame=(ggReign.frame/14+i)&1;
        for(int py=0;py<8;py++){
            const char *cel=shpRow(5,frame,py);
            for(int px=0;px<8;px++){
                int color=cel[px]=='b'?0:cel[px]=='a'?1:
                          cel[px]=='d'?2:cel[px]=='e'?3:-1;
                if(color<0)continue;
                const int colors[4][3]={{160,185,177},{218,162,86},
                                        {59,76,84},{237,216,180}};
                ggExpRect(out,x+(px-4)*scale,y-(8-py)*scale,
                          scale,scale,colors[color][0],colors[color][1],
                          colors[color][2]);
            }
        }
        ggExpRect(out,x+4*scale,y-10*scale,2,10*scale,170,150,94);
        ggExpRect(out,x+4*scale+2,y-10*scale,5*scale,3*scale,42,131,137);
    }
    // Text is contained in a slim band and a dedicated modal board.
    if(ggReign.menu){
        ggExpRect(out,14,45,372,144,13,24,38);
        ggExpRect(out,14,45,372,3,210,164,92);
        char line[110];
        snprintf(line,sizeof line,"THE UNWRITTEN REIGN   <%s>",
                 ggReignPages[ggReign.page]);
        ggExpText(out,25,56,line,243,220,166);
        if(ggReign.page==0){
            snprintf(line,sizeof line,"WORK: %s    OFFICE: %s",
                     ggReignRoles[ggReign.choice],ggReignOffices[ggReign.office]);
            ggExpText(out,25,79,line,225,233,216);
            ggExpText(out,25,96,"X BROWSE JOB  A APPLY IN TOWN",190,214,218);
            ggExpText(out,25,112,"FARM / CRAFT / HEAL / TRADE / SERVE / MORE",190,214,218);
        }else if(ggReign.page==1){
            snprintf(line,sizeof line,"SQUAD %d/%d   ORDER: %s",
                     ggReign.squadN,GG_REIGN_SQUAD_MAX,
                     ggReignOrders[ggReign.selectedOrder]);
            ggExpText(out,25,79,line,225,233,216);
            ggExpText(out,25,98,"X CHANGE  A COMMAND  Y RECRUIT IN TOWN",190,214,218);
            ggExpText(out,25,116,"FOLLOW / HOLD / ASSAULT; FIGHT WITH YOUR MEN",190,214,218);
        }else if(ggReign.page==2){
            snprintf(line,sizeof line,"CIVIC ACTION: %s",
                     ggReignCivicActions[ggReign.action]);
            ggExpText(out,25,79,line,225,233,216);
            snprintf(line,sizeof line,"TRUST %d TOWNS  INFLUENCE %d  LEGITIMACY %d",
                     ggReign.endorsements,ggReign.influence,ggReign.legitimacy);
            ggExpText(out,25,98,line,190,214,218);
            ggExpText(out,25,116,"X CHANGE  A EXECUTE AT A REAL SETTLEMENT",190,214,218);
        }else if(ggReign.page==3){
            snprintf(line,sizeof line,"GODBORN POWER: %s    ENERGY %d/100",
                     ggReignPowers[ggReign.action],ggReign.divine);
            ggExpText(out,25,79,line,225,233,216);
            ggExpText(out,25,98,"BOLT 20  /  MEND 30  /  BATTLE AWE 40",190,214,218);
            ggExpText(out,25,116,"X CHANGE  A CAST  GODHOOD IS INNATE",190,214,218);
        }else if(ggReign.page==4)ggQuestJournalDraw(out);
        else ggSagaFateDraw(out);
        ggExpText(out,25,146,"LEFT/RIGHT PAGES   B/SELECT CLOSE  START RETREAT",
                  230,195,130);
    }else{
        char line[112];
        ggExpRect(out,0,42,400,14,15,31,43);
        snprintf(line,sizeof line,"%s  %s  CASH %d  DIVINE %d  SQUAD %d/%d  SELECT: COUNCIL",
                 ggReignRoles[ggReign.role],ggReignOffices[ggReign.office],
                 ggReign.credits,ggReign.divine,ggReign.squadN,GG_REIGN_SQUAD_MAX);
        ggExpText(out,7,44,line,219,217,175);
    }
    if(ggReign.noticeT>0){
        ggExpRect(out,22,173,356,15,20,32,44);
        ggExpText(out,26,177,ggReign.notice,239,212,155);
    }
}
#endif
