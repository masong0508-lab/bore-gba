// wild.h - Pack C (wild things) and Pack D (sapience), included from calamity.h's neighbour in main.c.
// C: the FIRE ELEMENTAL and GREY GOO (roaming hazards in the Hz table, so they are saved in the GGH1 trailer), GHOSTS of the notable dead (also Hz entries), the CRAB BOMB and CRABZILLA powers, BANDITS (a flag bit per creature, law BANDITS).
// D: a creature flag SAPIENT (kept in crFl, so it rides in the GGN1 trailer; the 32 trait slots and the u32 trSeen could not take a 33rd trait), the UPLIFT power, the EVOLUTION law, and DENS (a sapient group builds a hut that feeds it).
// Nothing here is in a raw-saved struct. Integer maths only.
#ifndef GGWILD_H
#define GGWILD_H
// ---- the fire elemental: wanders, sets fire to what it touches, dies in rain and storms
static void fireStep(Hz*h){
    int kind=h->pad%5; h->age++;
    if(weather==WX_RAIN||weather==WX_STORM){ if(rnd()%8==0){ h->life=0; return; } }
    if(kind==3&&h->age%4==0){ int tx=(int)(rnd()%WW),ty=(int)(rnd()%WH); if(tx>1&&ty>1&&tx<WW-1&&ty<WH-1&&world[ty][tx]!=T_WATER&&world[ty][tx]!=T_DEEP){ h->x=(u16)tx; h->y=(u16)ty; } }
    else if(rnd()%6==0){ int d=(int)(rnd()%8); h->dx=hzDx[d]; h->dy=hzDy[d]; }
    int nx=h->x+h->dx, ny=h->y+h->dy; if(nx<2||nx>=WW-2){ h->dx=(s8)-h->dx; nx=h->x; } if(ny<2||ny>=WH-2){ h->dy=(s8)-h->dy; ny=h->y; }
    if(world[ny][nx]==T_DEEP||world[ny][nx]==T_WATER){ h->dx=(s8)-h->dx; h->dy=(s8)-h->dy; nx=h->x; ny=h->y; }
    h->x=(u16)nx; h->y=(u16)ny;
    int radius=kind==2?2:1;
    for(int dy=-radius;dy<=radius;dy++) for(int dx=-radius;dx<=radius;dx++){ int x=h->x+dx, y=h->y+dy; if(x<0||y<0||x>=WW||y>=WH||dx*dx+dy*dy>radius*radius+1) continue;
        burnAt(x,y,kind==2?180:kind==4?55:100);
        if(!LAWON(LW_GAIA)&&(world[y][x]==T_FOREST||world[y][x]==T_GRASS||world[y][x]==T_JUNGLE)&&rnd()%(kind==0?4:kind==2?2:3)==0) setCell(x,y,T_DIRT);
        if(kind==1&&!LAWON(LW_GAIA)&&dx==0&&dy==0&&world[y][x]!=T_WATER&&world[y][x]!=T_DEEP&&world[y][x]!=T_MOUNT&&rnd()%8==0) setCell(x,y,T_LAVA);
        if(occ[y][x]&&rnd()%(kind==2?2:kind==4?6:4)==0){ int i=occ[y][x]-1; if(kind==4&&arch[sp[cr[i].sp].a].diet==5){ if(cr[i].hp<maxHp(&cr[i])) cr[i].hp++; } else hzHurt(i,kind==2?12:kind==1?10:kind==3?9:6); }
    }
    if(kind==3&&h->age%12==0){ atmoDust(2); fxReq(FX_LIGHTNING); }
    if(kind==4&&(h->age&7)==0){ for(int dy=-4;dy<=4;dy++) for(int dx=-4;dx<=4;dx++){ int x=(int)h->x+dx,y=(int)h->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; Cr*c=&cr[occ[y][x]-1]; if(c->alive&&arch[sp[c->sp].a].diet==5&&c->hp<maxHp(c)) c->hp++; } }
}
// ---- grey goo: a creeping blob that turns land to dirt, erases buildings and kills what it touches; it turns back at water, and dies after a while
static void gooStep(Hz*h){
    h->age++; int nx=h->x+h->dx, ny=h->y+h->dy; if(nx<3||nx>=WW-3||ny<3||ny>=WH-3||world[ny][nx]==T_DEEP||world[ny][nx]==T_WATER){ h->dx=(s8)-h->dx; h->dy=(s8)-h->dy; if(rnd()&1) h->dx=(s8)(h->dx?0:1); nx=h->x; ny=h->y; }
    h->x=(u16)nx; h->y=(u16)ny; int r=h->r;
    for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){ if(dx*dx+dy*dy>r*r+1) continue; int x=h->x+dx, y=h->y+dy; if(x<0||y<0||x>=WW||y>=WH) continue; u8 t=world[y][x]; if(t==T_DEEP||t==T_WATER||t==T_LAVA) continue;
        if(bld[y][x]){ razeAt(x,y,255); bld[y][x]=0; bldv[y][x]=0; } if(t!=T_DIRT&&t!=T_MOUNT&&!LAWON(LW_GAIA)) setCell(x,y,T_DIRT); if(occ[y][x]&&(dx*dx+dy*dy)>=(r-1)*(r-1)) killCr(occ[y][x]-1); } }
// ---- ghosts of the notable dead: they drift, lower the mood of the villages near them, and are driven off by a temple
static int ghostMood(int v){ int n=0; for(int i=0;i<HZ_N;i++) if(hz[i].kind==HZ_GHOST||hz[i].kind==HZ_GHOSTCIV){ int dx=hz[i].x-vil[v].x, dy=hz[i].y-vil[v].y; if(dx*dx+dy*dy<=100) n++; } return n>2?20:n*10; }
static void ghostStep(Hz*h){
    h->age++; if(rnd()%5==0){ int d=(int)(rnd()%8); h->dx=hzDx[d]; h->dy=hzDy[d]; } int nx=h->x+h->dx, ny=h->y+h->dy; if(nx<2||nx>=WW-2||ny<2||ny>=WH-2){ h->dx=(s8)-h->dx; h->dy=(s8)-h->dy; nx=h->x; ny=h->y; } h->x=(u16)nx; h->y=(u16)ny;
    for(int dy=-6;dy<=6;dy++) for(int dx=-6;dx<=6;dx++){ int x=h->x+dx, y=h->y+dy; if(x<0||y<0||x>=WW||y>=WH) continue; if(bld[y][x]==B_TEMPLE){ h->life=0; return; } } }
static int ghostFor(int i){       // a notable with kills or children dies: a ghost where it fell (if the table has room)
    if(!(crFl[i]&FL_NOTE)) return 0; int k=nbFind(crId[i]); if(k<0||(nb[k].kills<5&&nb[k].kids<2)) return 0; if(hzAdd(HZ_GHOST,cr[i].x,cr[i].y)<0) return 0; return 1; }
// ---- snowmen: harmless, they stand on snow while it snows and melt when it stops or the ground is no longer snow
static void snowStep(Hz*h){ h->age++; if(world[h->y][h->x]!=T_SNOW||(weather!=WX_SNOW&&rnd()%12==0)) h->life=0; }
static void snowmenTick(void){ if(weather!=WX_SNOW||(tick&255)!=0||hzCount(HZ_SNOWMAN)>=2) return; for(int k=0;k<30;k++){ int x=(int)(rnd()%WW), y=(int)(rnd()%WH); if(world[y][x]==T_SNOW&&!occ[y][x]&&!bld[y][x]){ hzAdd(HZ_SNOWMAN,x,y); break; } } }
// ---- the crab bomb and Crabzilla
static int crabT;
static void crabBomb(int x,int y){ for(int dy=-6;dy<=6;dy++) for(int dx=-6;dx<=6;dx++){ if(dx*dx+dy*dy>36) continue; int xx=x+dx, yy=y+dy; if(xx<0||yy<0||xx>=WW||yy>=WH||!occ[yy][xx]) continue; Cr*c=&cr[occ[yy][xx]-1]; if(arch[sp[c->sp].a].diet==5) continue; if(!HAS(c,TR_GIGGLY)&&c->tr2==0) giveTrait(c,TR_GIGGLY,0); c->food=200; }
    for(int v=0;v<VIL_N;v++) if(vil[v].on){ int dx=vil[v].x-x, dy=vil[v].y-y; if(dx*dx+dy*dy<=64) vil[v].stock=(u16)(vil[v].stock>80?vil[v].stock-80:0); }      // the party eats the stores
    flashT=6; fxReq(FX_BIRTH); logEv("A CRAB BOMB: EVERYONE RAVES"); achX[2]|=2; }
static void crabStomp(int x,int y){ if(tick<crabT){ siprintf(info,"CRABZILLA IS RESTING"); return; } crabT=tick+120;
    for(int dy=-3;dy<=3;dy++) for(int dx=-3;dx<=3;dx++){ if(dx*dx+dy*dy>10) continue; int xx=x+dx, yy=y+dy; if(xx<0||yy<0||xx>=WW||yy>=WH) continue; if(bld[yy][xx]) stormRaze(xx,yy,200); if(occ[yy][xx]){ int i=occ[yy][xx]-1; if(maxHp(&cr[i])<30) killCr(i); else hzHurt(i,20); } }
    flashT=4; fxReq(FX_SMITE); siprintf(info,"CRABZILLA STOMPS"); achX[2]|=4; }
// ---- sapient animals (UPLIFT, EVOLUTION) and their dens
static int isAnimal(const Cr*c){ int d=arch[sp[c->sp].a].diet; return d<4; }
static int sapientCount(void){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive&&(crFl[i]&FL_SAPIENT); return n; }
static int uplift(int i){ if(!cr[i].alive||!isAnimal(&cr[i])||(crFl[i]&FL_SAPIENT)) return 0; crFl[i]|=FL_SAPIENT; if(sapientCount()==1){ char nm[48], b[56]; crName(&cr[i],nm); siprintf(b,"%.24s IS THE FIRST UPLIFTED ANIMAL",nm); logEv(b); } return 1; }
static void denTick(void){      // every so often a sapient animal with three sapient friends near builds a den (a hut) and sapient animals near a den never starve
    if((tick&127)!=0||!LAWON(LW_EVOLUTION)&&sapientCount()==0) return; denCount=0; int sapN=sapientCount(); sapNow=sapN;      // (tribes grow slowly and stop at eighty sapient animals in the world)
    for(int i=0;i<nCr;i++){ Cr*c=&cr[i]; if(!c->alive||!(crFl[i]&FL_SAPIENT)) continue; int near=0, den=0;
        for(int dy=-5;dy<=5;dy++) for(int dx=-5;dx<=5;dx++){ int x=c->x+dx, y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH) continue; if(bld[y][x]==B_HUT) den=1; if(occ[y][x]&&occ[y][x]-1!=i&&(crFl[occ[y][x]-1]&FL_SAPIENT)) near++; }
        if(den){ denCount++; if(c->food<120) c->food=120;
            // life around a den: tools, a growing tribe, more huts
            if(!c->wep&&rnd()%6==0) giveTool(c,0);
            if(!c->arm&&rnd()%8==0) giveTool(c,1);
            if(near>=2&&near<12&&sapN<80&&c->food>=100&&nCr<MAXC-4&&rnd()%30==0){ int d=rnd()&3; int n=addCr(c->sp,c->x+dxs[d],c->y+dys[d]); if(n>=0){ cr[n].age=0; cr[n].food=60; born(&cr[n],c); crFl[n]|=FL_SAPIENT|(crFl[i]&FL_EVOLVED); sapN++; sapNow=sapN; } }
            if(near>=5&&rnd()%6==0){ int huts=0; for(int dy=-5;dy<=5;dy++) for(int dx=-5;dx<=5;dx++){ int x=c->x+dx, y=c->y+dy; if(x>=0&&y>=0&&x<WW&&y<WH&&bld[y][x]==B_HUT) huts++; }
                if(huts<4){ int d=rnd()&3, x=c->x+2*dxs[d], y=c->y+2*dys[d]; if(x>=0&&y>=0&&x<WW&&y<WH&&!bld[y][x]&&!occ[y][x]&&world[y][x]>T_WATER&&world[y][x]!=T_LAVA&&world[y][x]!=T_MOUNT){ bld[y][x]=B_HUT; bldv[y][x]=1; } } } }
        else if(near>=3&&!bld[c->y][c->x]&&walkable(&sp[c->sp],c->x,c->y)&&rnd()%4==0&&world[c->y][c->x]!=T_WATER&&world[c->y][c->x]!=T_DEEP){ bld[c->y][c->x]=B_HUT; bldv[c->y][c->x]=1; logEv("SAPIENT ANIMALS BUILD A DEN"); } } }
// ---- bandits: a fighter from a restless city turns outlaw, raids the nearest store, and is hunted by towers
static int banditCount(void){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive&&(crFl[i]&FL_DESERT); return n; }
static void banditTick(void){
    if(!LAWON(LW_BANDITS)||(tick&63)!=0) return;
    for(int v=0;v<VIL_N;v++){ Vil*V=&vil[v]; if(!V->on||V->pop<8||vilL[v]>-20||rnd()%6) continue; int have=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&(crFl[i]&FL_DESERT)){ int dx=cr[i].x-V->x, dy=cr[i].y-V->y; if(dx*dx+dy*dy<400) have++; } if(have>=V->pop/4) continue;
        for(int i=0;i<nCr;i++){ Cr*c=&cr[i]; if(!c->alive||arch[sp[c->sp].a].diet!=4||(crFl[i]&FL_DESERT)||c->sp>=nSp) continue; int dx=c->x-V->x, dy=c->y-V->y; if(dx*dx+dy*dy>36) continue; crFl[i]|=FL_DESERT; char n[24], b[48]; vilName(v,n); siprintf(b,"OUTLAWS RISE NEAR %.26s",n); logEv(b); break; } }
    for(int i=0;i<nCr;i++){ Cr*c=&cr[i]; if(!c->alive||!(crFl[i]&FL_DESERT)) continue;
        for(int u=0;u<VIL_N;u++){ Vil*V=&vil[u]; if(!V->on) continue; int dx=c->x-V->x, dy=c->y-V->y; if(dx*dx+dy*dy<=16&&V->stock>5){ V->stock-=5; if(c->food<110) c->food+=40; } }
        for(int dy=-6;dy<=6;dy++) for(int dx=-6;dx<=6;dx++){ int x=c->x+dx, y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH) continue; if(bld[y][x]==B_TOWER&&rnd()%3==0){ hzHurt(i,12); dy=7; break; } } } }
// ---- monoliths: a standing stone (a statue marked 255) that slowly uplifts the animals around it; found again by a scan after a load
static int monoX[8], monoY[8];
static void monoScan(void){ monoN=0; for(int y=0;y<WH&&monoN<8;y++) for(int x=0;x<WW&&monoN<8;x++) if(bld[y][x]==B_STATUE&&bldv[y][x]==255){ monoX[monoN]=x; monoY[monoN]=y; monoN++; } }
static void monolithTick(void){ if((tick&2047)==0) monoScan(); if(!monoN||(tick&63)!=0) return;
    for(int m=0;m<monoN;m++) for(int dy=-6;dy<=6;dy++) for(int dx=-6;dx<=6;dx++){ int x=monoX[m]+dx, y=monoY[m]+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; if(rnd()%10==0) uplift(occ[y][x]-1); } }
// ---- drowning (law): a creature that cannot walk on water and stands in it loses health until it dies
static void drownTick(void){ if(!LAWON(LW_DROWN)||(tick&7)!=0) return; for(int i=0;i<nCr;i++){ Cr*c=&cr[i]; if(!c->alive) continue; u8 t=world[c->y][c->x]; if((t!=T_WATER&&t!=T_DEEP)||walkable(&sp[c->sp],c->x,c->y)) continue; if(c->hp>10) c->hp-=10; else killNat(i); } }
static void wildTick(void){ drownTick(); denTick(); banditTick(); snowmenTick(); monolithTick(); }
#endif
