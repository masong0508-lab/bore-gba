#ifndef GG_SETTLEMENT_BUILDINGS_H
#define GG_SETTLEMENT_BUILDINGS_H

enum {
    GG_BF_INN=8, GG_BF_WORKSHOP=16, GG_BF_MARKET=32,
    GG_BF_BARRACKS=64, GG_BF_KEEP=128, GG_FORT_RADIUS=2
};

static int ggBuildingFlag(int building){
    switch(building){
    case B_TEMPLE:return 1;
    case B_STATUE:return 2;
    case B_LIBRARY:return 4;
    case B_INN:return GG_BF_INN;
    case B_WORKSHOP:return GG_BF_WORKSHOP;
    case B_MARKET:return GG_BF_MARKET;
    case B_BARRACKS:return GG_BF_BARRACKS;
    case B_KEEP:return GG_BF_KEEP;
    default:return 0;
    }
}

static int ggFortSite(int x,int y,int axis){
    for(int dy=-GG_FORT_RADIUS;dy<=GG_FORT_RADIUS;dy++)
        for(int dx=-GG_FORT_RADIUS;dx<=GG_FORT_RADIUS;dx++)
            if(!ggOpenBuildingPlot(x+dx,y+dy))return 0;
    // Both gates need a clear landing outside the perimeter.
    int reach=GG_FORT_RADIUS+1;
    int dx=axis==B_GATE_EW?reach:0,dy=axis==B_GATE_NS?reach:0;
    return ggOpenBuildingPlot(x-dx,y-dy)&&ggOpenBuildingPlot(x+dx,y+dy);
}

static int ggPlaceFort(int owner,int x,int y,int axis){
    if(!ggBuildingIsGate(axis)||!ggFortSite(x,y,axis))return 0;
    KD_DIRTY();
    for(int dy=-GG_FORT_RADIUS;dy<=GG_FORT_RADIUS;dy++)
        for(int dx=-GG_FORT_RADIUS;dx<=GG_FORT_RADIUS;dx++){
            int wx=x+dx,wy=y+dy;
            if(!dx&&!dy)putBld(owner,wx,wy,B_KEEP);
            else if(abs(dx)==GG_FORT_RADIUS||abs(dy)==GG_FORT_RADIUS){
                int gate=axis==B_GATE_EW?!dy:!dx;
                if(gate)setCell(wx,wy,T_DIRT);
                putBld(owner,wx,wy,gate?axis:B_WALL);
            }else setCell(wx,wy,T_DIRT);
        }
    if(owner>=0)vilBf[owner]|=GG_BF_KEEP;
    return 1;
}

static void ggSettlementGrow(int v,int population){
    Vil*V=&vil[v];
    if(V->lvl<2||population<8)return;
    static const struct { u8 building,flag,folk,cost; } civic[]={
        {B_WORKSHOP,GG_BF_WORKSHOP,8,26},{B_INN,GG_BF_INN,10,30},
        {B_MARKET,GG_BF_MARKET,14,38},{B_BARRACKS,GG_BF_BARRACKS,18,42}
    };
    for(unsigned i=0;i<sizeof civic/sizeof *civic;i++){
        if((vilBf[v]&civic[i].flag)||population<civic[i].folk||V->stock<civic[i].cost)continue;
        if(placeBld(v,civic[i].building,civic[i].cost)){
            vilBf[v]|=civic[i].flag;
            char message[40];
            siprintf(message,"A %.16s OPENS",ggBuildingName(civic[i].building));
            logEv(message);
        }
        return;
    }
    if(V->lvl<3||population<18||V->stock<96||(vilBf[v]&GG_BF_KEEP))return;
    static const s8 site[][2]={{8,0},{0,8},{-8,0},{0,-8},
                                {7,7},{-7,7},{-7,-7},{7,-7}};
    unsigned rotation=((unsigned)tick>>9)&7u;
    for(unsigned i=0;i<sizeof site/sizeof *site;i++){
        const s8 *offset=site[(i+rotation)&7u];
        int x=V->x+offset[0],y=V->y+offset[1];
        int axis=abs(offset[0])>=abs(offset[1])?B_GATE_EW:B_GATE_NS;
        if(vilAny(x,y,4)>=0||!ggPlaceFort(v,x,y,axis))continue;
        V->stock-=96;
        logEv("A CASTLE GUARDS THE CITY");
        return;
    }
}

static void ggSettlementExpandHomes(int v,int population){
    Vil*V=&vil[v];
    if(V->lvl<2||population<10||V->stock<12)return;
    // Upgrade legacy single-plot homes when adjacent land is available.
    // A few fixed hash probes per city turn keep large worlds bounded.
    for(unsigned i=0;i<5;i++){
        unsigned sample=hsh(V->x,V->y,((unsigned)tick>>4)+i);
        int x=V->x+(int)(sample%13)-6,y=V->y+(int)((sample>>8)%13)-6;
        if(x<2||y<2||x>=WW-2||y>=WH-2||bldv[y][x]!=v+1||occ[y][x])continue;
        int type=bld[y][x];
        if(type!=B_HOUSE&&type!=B_MANOR)continue;
        int plan=ggResidenceWingPlan(x,y,type);
        if(!plan)continue;
        ggWriteResidence(v,x,y,plan);
        V->stock-=12;
        return;
    }
}

#endif
