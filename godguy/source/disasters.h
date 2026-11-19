// disasters.h - the six missing world-scale events, built on the existing saved Hz slots.
// No new persistent structs: the existing GGH1 hazard records carry every event across saves.
#ifndef GGDISASTERS_H
#define GGDISASTERS_H

static int disasterSpecies(const char *name){
    for(int s=8;s<nSp;s++) if(arch[sp[s].a].diet==5&&!strcmp(arch[sp[s].a].nm,name)) return s;
    return -1;
}
static int disasterSpawn(const char *name,int x,int y,int rank){
    int s=disasterSpecies(name); if(s<0) return -1;
    for(int r=0;r<=5;r++) for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){
        if(abs(dx)+abs(dy)!=r) continue; int xx=x+dx, yy=y+dy;
        if(!hzFree(xx,yy,&sp[s])) continue;
        int i=addCr(s,xx,yy); if(i<0) return -1;
        if(rank>0){ cr[i].rk=(u8)rank; cr[i].hp=(u8)maxHp(&cr[i]); }
        cr[i].food=160; return i;
    }
    return -1;
}
static void disasterDrift(Hz*h){
    if((rnd()&3)==0){ int d=(int)(rnd()&7); h->dx=hzDx[d]; h->dy=hzDy[d]; }
    int nx=(int)h->x+h->dx, ny=(int)h->y+h->dy;
    if(nx<2||nx>=WW-2){ h->dx=(s8)-h->dx; nx=(int)h->x; }
    if(ny<2||ny>=WH-2){ h->dy=(s8)-h->dy; ny=(int)h->y; }
    h->x=(u16)nx; h->y=(u16)ny;
}
static int disasterIsMonster(int i){ return i>=0&&i<nCr&&cr[i].alive&&arch[sp[cr[i].sp].a].diet==5; }
static void disasterStep(Hz*h){
    int x=(int)h->x, y=(int)h->y; h->age++;
    switch(h->kind){
    case HZ_ALIEN:
        if(h->age==1){ disasterSpawn("ALIEN",x,y,1); disasterSpawn("ALIEN",x+1,y,0); disasterSpawn("UFO",x-1,y,1); logEv("ALIENS LAND AND HUNT THE LIVING"); fxReq(FX_MONSTER_SEEN); }
        disasterDrift(h);
        for(int dy=-2;dy<=2;dy++) for(int dx=-2;dx<=2;dx++){ int xx=x+dx, yy=y+dy; if(xx<0||yy<0||xx>=WW||yy>=WH) continue;
            if(occ[yy][xx]){ int i=occ[yy][xx]-1; if(cr[i].alive&&!disasterIsMonster(i)&&(rnd()%3==0)) hzHurt(i,2); }
            if(bld[yy][xx]&&rnd()%12==0) stormRaze(xx,yy,30);
        }
        if(h->age%16==0&&h->pad<4){ if(disasterSpawn("ALIEN",h->x,h->y,0)>=0) h->pad++; }
        break;
    case HZ_DRAGON:
        if(h->age==1){ if(disasterSpawn("ELDER DRAGON",x,y,2)<0) disasterSpawn("WYVERN",x,y,2); logEv("AN ELDER DRAGON ARRIVES FROM FAR LANDS"); fxReq(FX_SMITE); }
        disasterDrift(h);
        for(int dy=-2;dy<=2;dy++) for(int dx=-2;dx<=2;dx++){ int xx=(int)h->x+dx, yy=(int)h->y+dy; if(xx<0||yy<0||xx>=WW||yy>=WH||dx*dx+dy*dy>5) continue;
            burnAt(xx,yy,85); if((world[yy][xx]==T_FOREST||world[yy][xx]==T_JUNGLE)&&!LAWON(LW_GAIA)&&(rnd()&3)==0) setCell(xx,yy,T_DIRT);
            if(occ[yy][xx]){ int i=occ[yy][xx]-1; if(!disasterIsMonster(i)) hzHurt(i,4); }
        }
        break;
    case HZ_MOON:
        if(h->age==1){ disasterSpawn("WRAITH",x,y,1); disasterSpawn("WRAITH",x+1,y,0); disasterSpawn("LICH",x-1,y,1); logEv("MOON HORRORS HAUNT THE SETTLEMENTS"); fxReq(FX_MONSTER_SEEN); }
        disasterDrift(h);
        for(int v=0;v<VIL_N;v++) if(vil[v].on){ int dx=(int)vil[v].x-(int)h->x, dy=(int)vil[v].y-(int)h->y; if(dx*dx+dy*dy<=49&&(h->age&3)==0) vil[v].stock=(u16)(vil[v].stock>2?vil[v].stock-2:0); }
        for(int dy=-3;dy<=3;dy++) for(int dx=-3;dx<=3;dx++){ int xx=(int)h->x+dx, yy=(int)h->y+dy; if(xx<0||yy<0||xx>=WW||yy>=WH||dx*dx+dy*dy>10||!occ[yy][xx]) continue; int i=occ[yy][xx]-1; if(!disasterIsMonster(i)&&(h->age&1)==0) hzHurt(i,2); }
        break;
    case HZ_BIOMASS:
        if(h->age==1){ disasterSpawn("TUMOR MONSTER",x,y,1); disasterSpawn("BIOBLOB",x+1,y,0); logEv("A BIOMASS MONSTROSITY AWAKENS"); fxReq(FX_SMITE); }
        if(h->age%12==0&&h->r<5) h->r++;
        disasterDrift(h);
        for(int dy=-(int)h->r;dy<=(int)h->r;dy++) for(int dx=-(int)h->r;dx<=(int)h->r;dx++){ int xx=(int)h->x+dx, yy=(int)h->y+dy; if(xx<0||yy<0||xx>=WW||yy>=WH||dx*dx+dy*dy>(int)h->r*h->r+1) continue; u8 t=world[yy][xx];
            if(bld[yy][xx]&&rnd()%18==0) stormRaze(xx,yy,55);
            if(!LAWON(LW_GAIA)){ if(t==T_FOREST||t==T_JUNGLE){ if(rnd()%5==0) setCell(xx,yy,T_SWAMP); } else if(t==T_GRASS||t==T_DIRT){ if(rnd()%7==0) setCell(xx,yy,T_FOREST); } }
            if(occ[yy][xx]){ int i=occ[yy][xx]-1; if(!disasterIsMonster(i)&&(rnd()&3)==0) hzHurt(i,2); }
        }
        if(h->age%24==0&&h->pad<5){ if(disasterSpawn("TUMOR MONSTER",h->x,h->y,0)>=0) h->pad++; }
        break;
    case HZ_ICE:
        disasterDrift(h);
        for(int dy=-3;dy<=3;dy++) for(int dx=-3;dx<=3;dx++){ int xx=(int)h->x+dx, yy=(int)h->y+dy; if(xx<0||yy<0||xx>=WW||yy>=WH||dx*dx+dy*dy>10) continue; u8 t=world[yy][xx];
            if(!LAWON(LW_GAIA)&&(t==T_GRASS||t==T_FOREST||t==T_JUNGLE||t==T_SWAMP)) setCell(xx,yy,T_SNOW);
            if(occ[yy][xx]){ int i=occ[yy][xx]-1; Cr*c=&cr[i]; if(c->alive&&!HAS(c,TR_FROST)&&!HAS(c,TR_PERMA)&&!HAS(c,TR_CHILL)&&!HAS(c,TR_FIRE)&&arch[sp[c->sp].a].diet!=5) hzHurt(i,2); }
        }
        for(int i=0;i<HZ_N;i++) if(&hz[i]!=h&&hz[i].kind==HZ_FIRE){ int dx=(int)hz[i].x-(int)h->x, dy=(int)hz[i].y-(int)h->y; if(dx*dx+dy*dy<=16) hz[i].life=0; }
        if(h->age==1){ logEv("ICE FROM A DREAM FREEZES THE LAND"); fxReq(FX_RAIN); }
        break;
    case HZ_GARDEN:
        if(h->age==1){ logEv("A GARDEN SURPRISE BRINGS NEW LIFE"); fxReq(FX_BLESS); }
        if(h->r<4&&h->age%18==0) h->r++;
        disasterDrift(h);
        for(int dy=-(int)h->r;dy<=(int)h->r;dy++) for(int dx=-(int)h->r;dx<=(int)h->r;dx++){ int xx=(int)h->x+dx, yy=(int)h->y+dy; if(xx<0||yy<0||xx>=WW||yy>=WH||dx*dx+dy*dy>(int)h->r*h->r+1) continue; u8 t=world[yy][xx];
            if(!LAWON(LW_GAIA)){ if(t==T_DIRT||t==T_SAND||t==T_SWAMP) setCell(xx,yy,T_GRASS); else if((t==T_GRASS||t==T_JUNGLE)&&(rnd()&3)==0) setCell(xx,yy,T_FOREST); }
            if(occ[yy][xx]){ Cr*c=&cr[occ[yy][xx]-1]; if(c->alive){ int hp=c->hp+3; c->hp=(u8)(hp>maxHp(c)?maxHp(c):hp); if(c->food<250)c->food+=2; if(DISEASE(c)==1&&rnd()%8==0) SET_DISEASE(c,2); } }
            if((dx==0&&dy==0)&&(h->age&3)==0) for(int v=0;v<VIL_N;v++) if(vil[v].on){ int vx=(int)vil[v].x-(int)h->x, vy=(int)vil[v].y-(int)h->y; if(vx*vx+vy*vy<=25&&vil[v].stock<400) vil[v].stock++; }
        }
        break;
    }
}
#endif
