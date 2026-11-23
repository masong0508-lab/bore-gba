#ifndef GG_CONSTRUCTION_PLANS_H
#define GG_CONSTRUCTION_PLANS_H

enum { GG_MARKET_SQUARE=B_N, GG_GUILD_YARD, GG_GARDEN_COURT };
enum { GG_SITE_CLEAR, GG_SITE_EDGE, GG_SITE_LAND, GG_SITE_BUSY,
       GG_SITE_ACCESS, GG_SITE_FORT };
typedef struct { s8 dx,dy; u8 kind; } GgPlanCell;
typedef struct { u8 count; GgPlanCell cell[32]; } GgBuildPlan;
_Static_assert(sizeof(((GgBuildPlan*)0)->cell)/sizeof(GgPlanCell)>=5*5+2,
               "Whole castle and both landings fit the construction preview");

static void ggPlanAdd(GgBuildPlan *plan,int dx,int dy,int kind,int turns){
    switch(turns&3){
    case 1: { int x=dx;dx=-dy;dy=x;break; }
    case 2: dx=-dx;dy=-dy;break;
    case 3: { int x=dx;dx=dy;dy=-x;break; }
    }
    plan->cell[plan->count++]=(GgPlanCell){(s8)dx,(s8)dy,(u8)kind};
}

static void ggCompoundPlan(GgBuildPlan *plan,int kind,int turns){
    // Each site has an irregular footprint and an entrance into a
    // walkable court. Dot cells remain available to the surrounding town.
    static const char blocks[3][5][6]={
        {".M.I.","=====",".=O=.",".===W","..=.."},
        {".W.L.",".===.","B=O=.",".==..",".=..."},
        {"..T..",".===.","F=O=F",".===.","..=.."}
    };
    static const u8 parts[256]={['=']=B_PAVING,['O']=B_WELL,['M']=B_MARKET,
        ['I']=B_INN,['W']=B_WORKSHOP,['L']=B_LIBRARY,['B']=B_BARRACKS,
        ['T']=B_TEMPLE,['F']=B_FARM};
    int style=kind-GG_MARKET_SQUARE;
    plan->count=0;
    for(int row=0;row<5;row++)for(int col=0;col<5;col++){
        int building=parts[(unsigned char)blocks[style][row][col]];
        if(building)ggPlanAdd(plan,col-2,row-2,building,turns);
    }
    // A grade-preserving paved landing past the street's outer end.
    ggPlanAdd(plan,style==1?-1:0,3,B_NONE,turns);
}

static int ggPlanCheck(const GgBuildPlan *plan,int x,int y){
    for(int i=0;i<plan->count;i++){
        const GgPlanCell *cell=&plan->cell[i];
        int wx=x+cell->dx,wy=y+cell->dy;
        if(wx<2||wy<2||wx>=WW-2||wy>=WH-2)return GG_SITE_EDGE;
        int terrain=world[wy][wx],old=bld[wy][wx];
        int paving=ggBuildingIsPaving(cell->kind);
        if(!cell->kind||(paving&&ggBuildingIsPaving(old))){
            if((old&&!ggBuildingIsPaving(old))||!((M_LAND>>terrain)&1)||
               abs(ggExpGround(terrain)-ggExpGround(T_DIRT))>GG_WORLD_WALK_RISE)
                return GG_SITE_ACCESS;
            if(!old&&occ[wy][wx])return GG_SITE_BUSY;
        }else{
            if(terrain!=T_GRASS&&terrain!=T_DIRT&&terrain!=T_FOREST)return GG_SITE_LAND;
            if(old||occ[wy][wx])return GG_SITE_BUSY;
            if(!paving&&ggFortProtectedPlot(wx,wy))return GG_SITE_FORT;
        }
    }
    return GG_SITE_CLEAR;
}

static void ggPlanCommit(const GgBuildPlan *plan,int owner,int x,int y){
    KD_DIRTY();
    for(int i=0;i<plan->count;i++){
        const GgPlanCell *cell=&plan->cell[i];
        int wx=x+cell->dx,wy=y+cell->dy,kind=cell->kind;
        if(!kind)kind=B_PAVING;
        if(ggBuildingIsPaving(kind)&&ggBuildingIsPaving(bld[wy][wx]))continue;
        if(cell->kind&&(ggBuildingIsPaving(kind)||kind==B_WELL||ggBuildingIsGate(kind))&&world[wy][wx]!=T_DIRT)
            setCell(wx,wy,T_DIRT);
        putBld(owner,wx,wy,kind);
        if(owner>=0){
            Vil *v=&vil[owner];
            if(ggBuildingIsHome(kind)&&v->houses<255)v->houses++;
            if(kind==B_FARM&&v->farms<255)v->farms++;
            vilBf[owner]|=(u8)ggBuildingFlag(kind);
        }
    }
}

#endif
