#ifndef GG_BUILDING_TYPES_H
#define GG_BUILDING_TYPES_H

// The original values are stored in world saves. Append new types here.
enum {
    B_NONE, B_HUT, B_HOUSE, B_HALL, B_FARM, B_TOWER, B_RUIN, B_DOCK,
    B_TEMPLE, B_STATUE, B_LIBRARY, B_BORDER, B_MANOR, B_WORKSHOP,
    B_INN, B_BARRACKS, B_MARKET, B_KEEP, B_WALL, B_GATE_NS, B_GATE_EW,
    B_HOUSE_EW, B_HOUSE_NS, B_ESTATE, B_WING_E, B_WING_S, B_WING_SE,
    B_PAVING, B_WELL, B_N
};

static inline int ggBuildingIsPaving(int type){
    return type==B_PAVING;
}

static inline int ggBuildingIsHome(int type){
    return type==B_HUT||type==B_HOUSE||type==B_MANOR||
           type==B_HOUSE_EW||type==B_HOUSE_NS||type==B_ESTATE;
}

static inline int ggBuildingIsWing(int type){
    return type>=B_WING_E&&type<=B_WING_SE;
}

static inline int ggBuildingIsGate(int type){
    return type==B_GATE_NS||type==B_GATE_EW;
}

static inline int ggBuildingStopsPawn(int type){
    return type==B_WALL||type==B_KEEP;
}

static inline int ggBuildingGateAllows(int type,int dx,int dy){
    if(type==B_GATE_NS)return dx==0;
    if(type==B_GATE_EW)return dy==0;
    return 1;
}

static inline const char *ggBuildingName(int type){
    static const char *const name[B_N]={
        "","HUT","HOUSE","TOWN HALL","FARM","WATCHTOWER","RUINS","DOCK",
        "TEMPLE","STATUE","LIBRARY","BORDER MARKER","MANOR","WORKSHOP",
        "INN","BARRACKS","MARKET","CASTLE KEEP","CURTAIN WALL",
        "NORTH-SOUTH GATE","EAST-WEST GATE","FAMILY HOUSE","FAMILY HOUSE",
        "MANOR ESTATE","HOUSE WING","HOUSE WING","MANOR WING",
        "PAVED WALK","STONE WELL"
    };
    return (unsigned)type<B_N?name[type]:name[B_NONE];
}

#endif
