// ggwar.h -- GodGuy step 10c, second half: war groups.
//
// A village at war sends its soldiers against the enemy village (thinkBody: a soldier with a home village whose foe is set, old enough and healthy, marches on it). That is already a group order:
// the village is the group, the foe village is its target. This file makes the group visible as ONE record, so the later steps can think about an army without walking its soldiers:
//   wg[v]   for every village v that has marching soldiers: the target village, how many soldiers, their power sum, and their formation centre (the average tile).
// Power of one soldier = 1 + its level (0..10) + 3 for an elite or boss. Nothing here calls rnd() and nothing changes a creature: the groups are only counted from the creatures, once every 256 ticks
// (after compact) and whenever a test asks, one pass over the creature list. Cost: one pass, no stored state per creature (the groups are VIL_N records, 14 bytes each).
// A group's soldiers are not removed from the world when it is far away; they stay ordinary creatures (step 10d lets the far ones take a cheap step towards the centre and resolves a battle as one roll).
#ifndef GGWAR_H
#define GGWAR_H

typedef struct { u8 tgt; u8 pad; u16 cnt, power, cx, cy, tx, ty; } WarGrp;     // tgt: the target village +1 (0: no group). cnt, power: soldiers and their power sum. (cx,cy): formation centre. (tx,ty): the target's tile
typedef char wgSizeCheck[sizeof(WarGrp)==14?1:-1];
static WarGrp wg[VIL_N]; static int wgN;     // wgN: groups with soldiers in them

// 10e: the home lookup. vilNear walks every village slot for every soldier, so counting 12,000 soldiers into groups cost 12,000 x 64 steps in one tick (a hitch of several frames on a DS, and 40,000 x 128 in DSi mode).
// wvBuild lays the villages on 16 x 16 tile blocks once a count: a village is written into every block its 14-tile reach touches (in village order), so the villages that can be a soldier's home are the few in its own block.
// wvNear scans only those, in the same order and with the same test as vilNear, so the answer is the same tie for tie. A block that holds more than WV_K villages is marked full and falls back to vilNear.
// -DGG_SLOWHOME counts the old way (the tests compare the two).
#define WV_SH 4
#define WV_K 8
#define WV_B (MAXD>>WV_SH)
static u8 wvN[WV_B*WV_B], wvL[WV_B*WV_B][WV_K]; static int wvW, wvOn;
static void wvBuild(void){
    wvW=(gW+(1<<WV_SH)-1)>>WV_SH; int nb=wvW*((gH+(1<<WV_SH)-1)>>WV_SH); memset(wvN,0,(size_t)nb);
    for(int v=0;v<VIL_N;v++){ if(!vil[v].on) continue; int x0=(int)vil[v].x-14, x1=(int)vil[v].x+14, y0=(int)vil[v].y-14, y1=(int)vil[v].y+14; if(x0<0) x0=0; if(y0<0) y0=0; if(x1>=gW) x1=gW-1; if(y1>=gH) y1=gH-1;
        for(int by=y0>>WV_SH;by<=(y1>>WV_SH);by++) for(int bx=x0>>WV_SH;bx<=(x1>>WV_SH);bx++){ int b=by*wvW+bx; if(wvN[b]==255) continue; if(wvN[b]<WV_K) wvL[b][wvN[b]++]=(u8)v; else wvN[b]=255; } } }
static int wvNear(int race,int x,int y){
    int b=(y>>WV_SH)*wvW+(x>>WV_SH); int n=wvN[b]; if(n==255) return vilNear(race,x,y,14);
    int best=-1,bd=14*14+1; for(int k=0;k<n;k++){ int v=wvL[b][k]; if(!vil[v].on||vil[v].race!=race) continue; int dx=(int)vil[v].x-x, dy=(int)vil[v].y-y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=v; } } return best; }
static int wgSoldier(const Cr*c,int*home){      // the very test the soldiers' thinking makes before marching
    if(!c->alive||arch[sp[c->sp].a].diet!=4||c->age<=6||c->hp<=8) return 0;
    int h=wvOn?wvNear(sp[c->sp].a,c->x,c->y):vilNear(sp[c->sp].a,c->x,c->y,14); if(h<0||!vil[h].foe||!vil[vil[h].foe-1].on) return 0; *home=h; return 1; }
static int wgPower(const Cr*c){ return 1+c->dir+(c->rk?3:0); }
// 10d: what a battle needs to know about an army, worked out in the same pass: the average blow, armour, health and pace of its soldiers, and the list of who they are (wgMem, grouped by village).
// wgOf[i] says which group creature i belonged to when the groups were last counted (village +1, 0 for none); it is good only for creatures below wgNc (compact() and a new world clear it).
typedef struct { u16 atk, def, hp, spd; } WarAvg;
static WarAvg wga[VIL_N]; static occ_t wgMem[MAXC]; static u32 wgOff[VIL_N+1]; static u32 wgLen[VIL_N];
static u32 wgAcc[VIL_N][5]; static int wgCur;      // 10f: the averages being gathered by the slices (blow, armour, health, pace, soldiers seen) and where the slice has got to in the creature list
static int wgBlow(const Cr*c){ int a=arch[sp[c->sp].a].atk+gearAtk(c)+1+c->dir/2; if(c->rk==1) a=a*13/10; else if(c->rk==2) a*=2; return a; }      // one blow before armour, the random part at its average
static void wgBuild(void){
    static unsigned sx[VIL_N], sy[VIL_N], sa[VIL_N], sd[VIL_N], sh[VIL_N], ss[VIL_N], cc[VIL_N]; memset(wg,0,sizeof wg); memset(sx,0,sizeof sx); memset(sy,0,sizeof sy); memset(sa,0,sizeof sa); memset(sd,0,sizeof sd); memset(sh,0,sizeof sh); memset(ss,0,sizeof ss); memset(cc,0,sizeof cc); memset(wga,0,sizeof wga); wgN=0; wgNc=nCr; wgCur=0; memset(wgAcc,0,sizeof wgAcc);
#ifndef GG_SLOWHOME
    wvBuild(); wvOn=1;
#endif
    for(int i=0;i<nCr;i++){ const Cr*c=&cr[i]; int h; wgOf[i]=0; if(!wgSoldier(c,&h)) continue; wgOf[i]=(u8)(h+1); WarGrp*g=&wg[h]; if(g->cnt<65535) g->cnt++; cc[h]++; int p=g->power+wgPower(c); g->power=(u16)(p>65535?65535:p); sx[h]+=c->x; sy[h]+=c->y; sa[h]+=wgBlow(c); sd[h]+=armDef(c); sh[h]+=c->hp; ss[h]+=effSpd(c); }
    wgOff[0]=0; for(int v=0;v<VIL_N;v++){ wgOff[v+1]=wgOff[v]+cc[v]; wgLen[v]=0; }
    for(int i=0;i<nCr;i++) if(wgOf[i]){ int h=wgOf[i]-1; wgMem[wgOff[h]+wgLen[h]++]=(occ_t)i; }
    for(int v=0;v<VIL_N;v++){ WarGrp*g=&wg[v]; if(!cc[v]){ continue; } wgN++; g->tgt=(u8)vil[v].foe; g->cx=(u16)(sx[v]/cc[v]); g->cy=(u16)(sy[v]/cc[v]); g->tx=vil[vil[v].foe-1].x; g->ty=vil[vil[v].foe-1].y;
        wga[v].atk=(u16)(sa[v]/cc[v]); wga[v].def=(u16)(sd[v]/cc[v]); wga[v].hp=(u16)(sh[v]/cc[v]); wga[v].spd=(u16)(ss[v]/cc[v]); } wvOn=0; }
// 10f: the count, rolled. wgBuild above is one pass over every creature in one tick (0.3 ms of a PC for 12,000 soldiers, a hitch of a few frames on a DS). Here the expensive half of it, finding each soldier's home
// village and the averages of the army (blow, armour, health, pace), is done a few creatures a tick (wgSlice: the whole list once in about WG_PERIOD ticks) and kept in wgOf[] and wgAcc[]; what is left for
// every 256th tick (wgCommit, right after compact) is the cheap part: drop whoever is no longer a soldier at home (dead, wounded, a village gone or at peace), add up the soldiers, their power and their centre, and
// write the member lists. A soldier that has walked out of its home's reach is looked up again at once, so the marching armies leave their groups as they did; what can be late is a young adult joining
// (up to WG_PERIOD ticks) and a soldier whose nearest own village has changed while the old one is still in reach. wgBuild still counts everything exactly (a load, a save, small worlds, the tests).
#define WG_PERIOD 128
static void wgSlice(void){
    if(wgCur==0) wvBuild();      // the village blocks are laid again at the start of every sweep
    if(wgCur>=nCr){ for(int v=0;v<VIL_N;v++) if(wgAcc[v][4]){ unsigned n=wgAcc[v][4]; wga[v].atk=(u16)(wgAcc[v][0]/n); wga[v].def=(u16)(wgAcc[v][1]/n); wga[v].hp=(u16)(wgAcc[v][2]/n); wga[v].spd=(u16)(wgAcc[v][3]/n); } memset(wgAcc,0,sizeof wgAcc); wgCur=0; return; }
    int q=(nCr+WG_PERIOD-1)/WG_PERIOD; if(q<8) q=8; wvOn=1;
    for(int k=0;k<q&&wgCur<nCr;k++){ int i=wgCur++; const Cr*c=&cr[i]; int h; if(wgSoldier(c,&h)){ wgOf[i]=(u8)(h+1); wgAcc[h][0]+=(unsigned)wgBlow(c); wgAcc[h][1]+=(unsigned)armDef(c); wgAcc[h][2]+=c->hp; wgAcc[h][3]+=(unsigned)effSpd(c); wgAcc[h][4]++; } else wgOf[i]=0; if(i>=wgNc) wgNc=i+1; }
    wvOn=0; }
static void wgCommit(void){      // after compact(): wgKeep creatures at the front have a wgOf from the slices
    static unsigned sx[VIL_N], sy[VIL_N], cc[VIL_N], pw[VIL_N], na[VIL_N], nd[VIL_N], nh[VIL_N], ns[VIL_N];
    memset(wg,0,sizeof wg); memset(sx,0,sizeof sx); memset(sy,0,sizeof sy); memset(cc,0,sizeof cc); memset(pw,0,sizeof pw); memset(na,0,sizeof na); memset(nd,0,sizeof nd); memset(nh,0,sizeof nh); memset(ns,0,sizeof ns);
    wgN=0; wgNc=wgKeep; wgCur=0; memset(wgAcc,0,sizeof wgAcc); wvBuild(); wvOn=1;
    for(int i=0;i<wgNc;i++){ int h1=wgOf[i]; if(!h1) continue; const Cr*c=&cr[i]; int h=h1-1;
        if(!c->alive||c->age<=6||c->hp<=8||!vil[h].on||!vil[h].foe||!vil[vil[h].foe-1].on){ wgOf[i]=0; continue; }
        { int dx=(int)vil[h].x-(int)c->x, dy=(int)vil[h].y-(int)c->y; if(vil[h].race!=sp[c->sp].a||dx*dx+dy*dy>14*14){ int hh; if(!wgSoldier(c,&hh)){ wgOf[i]=0; continue; } h=hh; wgOf[i]=(u8)(h+1); } }
        cc[h]++; pw[h]+=(unsigned)wgPower(c); sx[h]+=c->x; sy[h]+=c->y;
        if(!wga[h].hp){ na[h]+=(unsigned)wgBlow(c); nd[h]+=(unsigned)armDef(c); nh[h]+=c->hp; ns[h]+=(unsigned)effSpd(c); } }      // (only a group that has no averages yet is worked out here)
    wvOn=0;
    wgOff[0]=0; for(int v=0;v<VIL_N;v++){ wgOff[v+1]=wgOff[v]+cc[v]; wgLen[v]=0; }
    for(int i=0;i<wgNc;i++) if(wgOf[i]){ int h=wgOf[i]-1; wgMem[wgOff[h]+wgLen[h]++]=(occ_t)i; }
    for(int v=0;v<VIL_N;v++){ WarGrp*g=&wg[v]; if(!cc[v]){ memset(&wga[v],0,sizeof wga[v]); continue; } wgN++; g->cnt=(u16)(cc[v]>65535?65535:cc[v]); g->power=(u16)(pw[v]>65535?65535:pw[v]); g->tgt=(u8)vil[v].foe; g->cx=(u16)(sx[v]/cc[v]); g->cy=(u16)(sy[v]/cc[v]); g->tx=vil[vil[v].foe-1].x; g->ty=vil[vil[v].foe-1].y;
        if(!wga[v].hp){ wga[v].atk=(u16)(na[v]/cc[v]); wga[v].def=(u16)(nd[v]/cc[v]); wga[v].hp=(u16)(nh[v]/cc[v]); wga[v].spd=(u16)(ns[v]/cc[v]); } } }
static int wgDist(const WarGrp*g){ int dx=(int)g->cx-(int)g->tx, dy=(int)g->cy-(int)g->ty; return (dx<0?-dx:dx)+(dy<0?-dy:dy); }      // how far the formation centre is from its target
static int wgOpposing(int v){ if(!wg[v].cnt) return -1; int f=wg[v].tgt-1; return wg[f].cnt?f:-1; }                                   // the enemy village's own army, if it has one marching
static int wgGap(int v){ int f=wgOpposing(v); if(f<0) return -1; int dx=(int)wg[v].cx-(int)wg[f].cx, dy=(int)wg[v].cy-(int)wg[f].cy; return (dx<0?-dx:dx)+(dy<0?-dy:dy); }   // how far apart two armies' centres are
#endif
