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

static int wgSoldier(const Cr*c,int*home){      // the very test the soldiers' thinking makes before marching
    if(!c->alive||arch[sp[c->sp].a].diet!=4||c->age<=6||c->hp<=8) return 0;
    int h=vilNear(sp[c->sp].a,c->x,c->y,14); if(h<0||!vil[h].foe||!vil[vil[h].foe-1].on) return 0; *home=h; return 1; }
static int wgPower(const Cr*c){ return 1+c->dir+(c->rk?3:0); }
static void wgBuild(void){
    static unsigned sx[VIL_N], sy[VIL_N]; memset(wg,0,sizeof wg); memset(sx,0,sizeof sx); memset(sy,0,sizeof sy); wgN=0;
    for(int i=0;i<nCr;i++){ const Cr*c=&cr[i]; int h; if(!wgSoldier(c,&h)) continue; WarGrp*g=&wg[h]; if(g->cnt<65535) g->cnt++; int p=g->power+wgPower(c); g->power=(u16)(p>65535?65535:p); sx[h]+=c->x; sy[h]+=c->y; }
    for(int v=0;v<VIL_N;v++){ WarGrp*g=&wg[v]; if(!g->cnt){ continue; } wgN++; g->tgt=(u8)vil[v].foe; g->cx=(u16)(sx[v]/g->cnt); g->cy=(u16)(sy[v]/g->cnt); g->tx=vil[vil[v].foe-1].x; g->ty=vil[vil[v].foe-1].y; } }
static int wgDist(const WarGrp*g){ int dx=(int)g->cx-(int)g->tx, dy=(int)g->cy-(int)g->ty; return (dx<0?-dx:dx)+(dy<0?-dy:dy); }      // how far the formation centre is from its target
static int wgOpposing(int v){ if(!wg[v].cnt) return -1; int f=wg[v].tgt-1; return wg[f].cnt?f:-1; }                                   // the enemy village's own army, if it has one marching
static int wgGap(int v){ int f=wgOpposing(v); if(f<0) return -1; int dx=(int)wg[v].cx-(int)wg[f].cx, dy=(int)wg[v].cy-(int)wg[f].cy; return (dx<0?-dx:dx)+(dy<0?-dy:dy); }   // how far apart two armies' centres are
#endif
