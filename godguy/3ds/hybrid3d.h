// GodGuy 3DS hybrid creature camera.
// Maps the existing Inspect/Possess selection onto a persistent creature ID
// and the live voxel viewport. Sentience is the game's own people/FL_SAPIENT
// classification; MINDLESS and infected zombies cannot become a POV.
// No additional save fields, world buffers, creature copies or frame scans.
#ifndef GG_HYBRID3D_H
#define GG_HYBRID3D_H

static int ggHybridEligible(int index){
    if(index<0||index>=nCr||!cr[index].alive)return 0;
    const Cr *c=&cr[index];
    if((unsigned)c->sp>=(unsigned)nSp||
       (unsigned)c->x>=WW||(unsigned)c->y>=WH)return 0;
    int race=sp[c->sp].a;
    if(race<0||race>=ARCH_N||!crId[index])return 0;
    if(HAS_STATUS(c,ST_MINDLESS)||DISEASE(c)==3)return 0;
    return arch[race].diet==4||(crFl[index]&FL_SAPIENT)!=0;
}
static int ggHybridResolve(void){
    if(!ggExplore.actorId)return -1;
    int i=ggExplore.actorIndex;
    if(i>=0&&i<nCr&&crId[i]==ggExplore.actorId)
        return ggHybridEligible(i)?i:-1;
    // saveGameRaw compacts living creatures and shifts array slots.
    // Only a mismatch causes a linear ID lookup; NOT every render frame.
    for(i=0;i<nCr;i++)
        if(crId[i]==ggExplore.actorId){
            ggExplore.actorIndex=i;
            return ggHybridEligible(i)?i:-1;
        }
    return -1;
}
static int ggHybridPick(int x,int y){
    if((unsigned)x>=WW||(unsigned)y>=WH)return -1;
    int index=occ[y][x]?(int)occ[y][x]-1:-1;
    if(!ggHybridEligible(index))return -1;
    // Inspect is a deliberate selection as in normal god-game play;
    // Possession lets the D-pad control this same mind in the 2D map.
    inspected=index;
    inspKind=1;
    inspIdx=index;
    inspT=420;
    possessed=index;
    return index;
}
static int ggHybridBegin(int index,int oldPause){
    if(!ggHybridEligible(index))return 0;
    Cr *c=&cr[index];
    // An intentional resident can occupy a home or a water habitat:
    // the generic land-spawn check does not override its own location.
    (void)ggExpBegin(c->x,c->y,oldPause);
    ggExplore.active=1;
    // The selected creature may be standing IN its house, on a water
    // habitat, or on a unique creature-passable tile. Do not teleport
    // its consciousness to an arbitrary nearby open grassy cell.
    ggExplore.px=(int)c->x*256+128;
    ggExplore.py=(int)c->y*256+128;
    ggExplore.actorId=crId[index];
    ggExplore.actorIndex=index;
    crName(c,ggExplore.actorName);
    // A genuine selection may originate in a house interior; the live
    // occupancy grid is authoritative but may be transient mid-move.
    inspected=index;inspKind=1;inspIdx=index;inspT=420;
    possessed=index;
    return 1;
}
static int ggHybridMove(int oldPX,int oldPY){
    if(!ggExplore.actorId)return 1; // existing free-camera behaviour
    int i=ggHybridResolve();
    if(i<0)return 0; // subject died or lost sentience: return to map
    Cr *c=&cr[i];
    int nx=ggExplore.px>>8,ny=ggExplore.py>>8;
    if(nx!=c->x||ny!=c->y){
        int fromX=c->x,fromY=c->y;
        // Never move a different actor, walk through another person,
        // or teleport across the map using first-person controls.
        if((nx-fromX>1||nx-fromX< -1||
            ny-fromY>1||ny-fromY< -1)||
           (unsigned)nx>=WW||(unsigned)ny>=WH||
           (bld[ny][nx]&&!ggBuildingIsGate(bld[ny][nx]))||occ[ny][nx]||
           !ggHybridWalkable(c,nx,ny)){
            ggExplore.px=oldPX;
            ggExplore.py=oldPY;
            ggExplore.stepPhase=0;
        }else{
            moveTo(c,i,nx,ny);
            // The existing 2D sprite and bottom-map occupant position
            // now agree with the first-person camera. No duplicate actor.
            possessed=i;
            inspected=i;
            inspIdx=i;
        }
    }
    return 1;
}
static void ggHybridAfterSave(void){
    if(!ggExplore.actorId)return;
    int i=ggHybridResolve();
    if(i>=0){
        possessed=i;
        inspected=i;
        inspIdx=i;
        inspKind=1;
    }
}
#endif
