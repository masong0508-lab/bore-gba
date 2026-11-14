// calamity.h - GODGUY v0.14, the CALAMITY PACK: two roaming disasters (TORNADO, BLACK HOLE), ACID RAIN, the ATOMIC BOMB, a PLAGUE that spreads from creature to creature, a CURSE,
// and the rules behind WORLD > LAWS (gx.lAge, lPlague, lFire, lLava, lStorm: see ggx.h).
// Included from main.c just before eventTick: it uses setCell, killCr, moveTo, razeAt, logEv and friends, which are all defined above that point. Integer maths only.
// What is saved: the plague is one byte in each creature (Cr.sick: 0 healthy, 1 plagued, 2 recovered and immune), which rides along in the creature list.
// The roaming disasters are a small table (hz) that goes to the save file in a second trailer ("GGH1") after the GGX1 one, so an old build never reads it and an old save has none.
#define HZ_N 6
enum { HZ_NONE, HZ_TORNADO, HZ_HOLE };
typedef struct { u8 kind, x, y; s8 dx, dy; u8 r, t, pad; u16 life, age; } Hz;   // a roaming disaster: what, where, which way it drifts, its size, a step timer, steps left, steps lived
static Hz hz[HZ_N];
static const s8 hzDx[8]={1,1,0,-1,-1,-1,0,1}, hzDy[8]={0,1,1,1,0,-1,-1,-1};
static int hzAdd(int kind,int x,int y){
    for(int i=0;i<HZ_N;i++) if(hz[i].kind==HZ_NONE){ Hz*h=&hz[i]; memset(h,0,sizeof *h); int d=(int)(rnd()%8);
        h->kind=(u8)kind; h->x=(u8)x; h->y=(u8)y; h->dx=hzDx[d]; h->dy=hzDy[d]; h->r=1; h->life=(u16)(kind==HZ_TORNADO?220:110); return i; }
    return -1; }
static int hzCount(int kind){ int n=0; for(int i=0;i<HZ_N;i++) n+=hz[i].kind==kind; return n; }
// ---------------------------------------------------------------- helpers
static int hzFree(int x,int y,const Sp*s){ return x>=0&&y>=0&&x<WW&&y<WH&&!occ[y][x]&&!bld[y][x]&&walkable(s,x,y); }
static int hzHurt(int i,int dmg){ Cr*c=&cr[i]; if(c->hp>dmg){ c->hp=(u8)(c->hp-dmg); return 1; } killCr(i); return 0; }   // 1 if it lives
static void hzFling(int i){   // thrown up to six tiles away, onto any free tile it could stand on (if there is none it just stays)
    Cr*c=&cr[i]; for(int k=0;k<8;k++){ int nx=c->x+(int)(rnd()%13)-6, ny=c->y+(int)(rnd()%13)-6; if(!hzFree(nx,ny,&sp[c->sp])) continue;
        occ[c->y][c->x]=0; c->x=(u8)nx; c->y=(u8)ny; c->ox=0; c->oy=0; occ[ny][nx]=(u16)(i+1); return; } }
static void stormRaze(int x,int y,int chance){ if(!gx.lStorm) razeAt(x,y,chance); }   // tornado, acid and black hole leave homes alone when WORLD > LAWS says so
// ---------------------------------------------------------------- the plague
static int infect(int i){ Cr*c=&cr[i]; if(!c->alive||c->sick||gx.lPlague==1) return 0; c->sick=1; return 1; }   // only the healthy and the never-ill can fall sick
static void plagueTick(void){   // every 32 ticks: the sick suffer, pass it to whoever stands next to them, and now and then get better (and stay immune)
    if((tick&31)!=11) return; int mode=gx.lPlague;   // 0 normal, 1 off (everyone is cured at once), 2 deadly
    for(int i=0;i<nCr;i++){ Cr*c=&cr[i]; if(!c->alive||c->sick!=1) continue;
        if(mode==1){ c->sick=2; continue; }
        int dmg=mode==2?4:2; if(c->hp>dmg) c->hp=(u8)(c->hp-dmg); else { killNat(i); continue; }
        for(int k=0;k<4;k++){ int x=c->x+dxs[k], y=c->y+dys[k]; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; Cr*q=&cr[occ[y][x]-1]; if(q->alive&&!q->sick&&(int)(rnd()%100)<(mode==2?45:25)) q->sick=1; }
        if((int)(rnd()%100)<(mode==2?4:10)) c->sick=2; } }
static int plagueCount(int st){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive&&cr[i].sick==st; return n; }
// ---------------------------------------------------------------- the tornado: wanders, uproots forest, flattens homes, throws whatever it meets
static void tornadoStep(Hz*h){
    if(rnd()%5==0){ h->dx=(s8)((int)(rnd()%3)-1); h->dy=(s8)((int)(rnd()%3)-1); if(!h->dx&&!h->dy) h->dx=1; }
    int nx=h->x+h->dx, ny=h->y+h->dy; if(nx<2||nx>=WW-2){ h->dx=(s8)-h->dx; nx=h->x; } if(ny<2||ny>=WH-2){ h->dy=(s8)-h->dy; ny=h->y; } h->x=(u8)nx; h->y=(u8)ny;
    for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){ int x=h->x+dx, y=h->y+dy; if(x<0||y<0||x>=WW||y>=WH) continue; u8 t=world[y][x];
        if((t==T_FOREST||t==T_JUNGLE)&&rnd()%3==0) setCell(x,y,T_GRASS);
        if(bld[y][x]) stormRaze(x,y,150);
        if(occ[y][x]){ int i=occ[y][x]-1; if(hzHurt(i,6+(int)(rnd()%8))) hzFling(i); } } }
// ---------------------------------------------------------------- the black hole: grows, drags creatures in, kills what reaches it and leaves deep sea where it was
static void holeStep(Hz*h){
    h->age++; if(h->age%24==0&&h->r<5) h->r++; int r=h->r, pr=r+5;
    for(int dy=-pr;dy<=pr;dy++) for(int dx=-pr;dx<=pr;dx++){ int x=h->x+dx, y=h->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int d2=dx*dx+dy*dy; if(d2>pr*pr) continue; int i=occ[y][x]-1;
        if(d2<=r*r){ killNat(i); continue; }
        int sx=dx>0?-1:dx<0?1:0, sy=dy>0?-1:dy<0?1:0; Cr*c=&cr[i]; if(hzFree(x+sx,y+sy,&sp[c->sp])) moveTo(c,i,x+sx,y+sy); }   // one tile towards the middle
    for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){ if(dx*dx+dy*dy>r*r) continue; int x=h->x+dx, y=h->y+dy; if(x<0||y<0||x>=WW||y>=WH) continue;
        if(bld[y][x]){ if(gx.lStorm) continue; razeAt(x,y,255); bld[y][x]=0; bldv[y][x]=0; }
        if(world[y][x]!=T_DEEP) setCell(x,y,T_DEEP); } }
// ---------------------------------------------------------------- acid rain: a brush; plants die, stone crumbles, homes corrode, flesh burns
static void acidAt(int x,int y){ u8 t=world[y][x];
    if((t==T_GRASS||t==T_FOREST||t==T_JUNGLE)&&rnd()%3==0) setCell(x,y,T_DIRT);
    else if((t==T_HILL||t==T_MOUNT)&&rnd()%20==0) setCell(x,y,t==T_MOUNT?T_HILL:T_DIRT);
    if(bld[y][x]) stormRaze(x,y,24);
    if(occ[y][x]&&(rnd()&3)==0) hzHurt(occ[y][x]-1,5); }
// ---------------------------------------------------------------- the atomic bomb: a lava core, a scorched crater, everything dead inside, fallout (plague) outside
static void nukeAt(int cx,int cy,int R){
    int core=R*R/9, mid=R*R*4/9, outer=(R+5)*(R+5);
    for(int dy=-R-5;dy<=R+5;dy++) for(int dx=-R-5;dx<=R+5;dx++){ int x=cx+dx, y=cy+dy; if(x<0||y<0||x>=WW||y>=WH) continue; int d2=dx*dx+dy*dy; if(d2>outer) continue; u8 t=world[y][x]; int water=(t==T_WATER||t==T_DEEP);
        if(d2<=R*R){
            if(bld[y][x]) razeAt(x,y,255);
            if(occ[y][x]) killCr(occ[y][x]-1);
            if(d2<=core){ if(bld[y][x]){ bld[y][x]=0; bldv[y][x]=0; } setCell(x,y,water?T_SAND:T_LAVA); }
            else if(d2<=mid){ if(!water){ if(bld[y][x]){ bld[y][x]=0; bldv[y][x]=0; } setCell(x,y,T_DIRT); } }
            else if((t==T_FOREST||t==T_JUNGLE||t==T_GRASS||t==T_SWAMP)&&rnd()%10<7) setCell(x,y,T_DIRT); }
        else if(occ[y][x]){ int i=occ[y][x]-1; if((int)(rnd()%100)<60) infect(i); if(rnd()&1) hzHurt(i,10); } }   // the fallout ring: sickness and burns
    flashT=14; fxReq(FX_METEOR); logEv("AN ATOMIC BOMB FALLS"); }
// ---------------------------------------------------------------- the world has calamities of its own now and then (only when DISASTERS is not OFF)
static void calamityNatural(void){
    if(!gx.dis||tick<2400||(tick%1800)!=900||rnd()%(gx.dis==1?6:3)) return;
    int k=(int)(rnd()%6), x=(int)(rnd()%WW), y=(int)(rnd()%WH);
    if(k<3){ if(hzAdd(HZ_TORNADO,x,y)>=0){ logEv("A TORNADO TOUCHES DOWN"); fxReq(FX_LIGHTNING); } }
    else if(k==3){ if(hzAdd(HZ_HOLE,x,y)>=0){ logEv("A BLACK HOLE OPENS"); fxReq(FX_SMITE); flashT=4; } }
    else if(gx.lPlague!=1){ for(int tries=0;tries<60;tries++){ int i=(int)(rnd()%(unsigned)(nCr?nCr:1)); if(i<nCr&&cr[i].alive&&arch[sp[cr[i].sp].a].diet!=5&&infect(i)){ logEv("A PLAGUE BREAKS OUT"); fxReq(FX_MONSTER_SEEN); break; } } } }
static void calamityTick(void){
    for(int i=0;i<HZ_N;i++){ Hz*h=&hz[i]; if(!h->kind) continue; if(++h->t<(h->kind==HZ_TORNADO?4:6)) continue; h->t=0;
        if(h->kind==HZ_TORNADO) tornadoStep(h); else holeStep(h);
        if(h->life>0) h->life--; if(!h->life){ if(h->kind==HZ_TORNADO) siprintf(info,"THE TORNADO DIES OUT"); else { siprintf(info,"THE BLACK HOLE COLLAPSES"); flashT=6; } h->kind=HZ_NONE; } }
    plagueTick(); calamityNatural(); }
