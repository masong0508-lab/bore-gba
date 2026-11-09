// worldgen.h - GODGUY world generator 2 (included from main.c in place of the old genWorld).
// Land is a set of plates: each is a disc (centre, radius) whose coast is wobbled by noise; height falls from the middle of a plate to the sea, so
// where plates are apart there is deep ocean between them. Plates are placed with a gap (continents), in clusters (archipelagos) or small and many (islands).
// All noise is tileable (period = the planet), so with EDGE = WRAP there is no seam. Integer maths only.
static int cEdge;      // what happens at the edge of the map: 0 walls, 1 vanish, 2 wrap round
static int wnoise(int x,int y,int cell,int s){   // value noise 0..255 that repeats every planet width
    int px=WW/cell, py=WH/cell; if(px<1) px=1; if(py<1) py=1;
    int gx=x/cell, gy=y/cell, fx=x%cell, fy=y%cell; int gx1=(gx+1)%px, gy1=(gy+1)%py; gx%=px; gy%=py;
    int a=(int)(hsh(gx,gy,s)&255), b=(int)(hsh(gx1,gy,s)&255), c=(int)(hsh(gx,gy1,s)&255), d=(int)(hsh(gx1,gy1,s)&255);
    int t=a+(b-a)*fx/cell, u=c+(d-c)*fx/cell; return t+(u-t)*fy/cell;
}
static int wfbm(int x,int y,int s){ return (wnoise(x,y,32,s)*8+wnoise(x,y,16,s+1)*4+wnoise(x,y,8,s+2)*2+wnoise(x,y,4,s+3))/15; }
#define GW_P 40
static int gpX[GW_P], gpY[GW_P], gpR[GW_P], gpN;
static int gwDist(int ax,int ay,int bx,int by){   // a cheap round-ish distance, the short way round when the planet wraps
    int dx=ax-bx, dy=ay-by; if(dx<0) dx=-dx; if(dy<0) dy=-dy;
    if(cEdge==2){ if(dx>WW/2) dx=WW-dx; if(dy>WH/2) dy=WH-dy; }
    return dx>dy?dx+(dy*3>>3):dy+(dx*3>>3);
}
// put one plate near (cx,cy) within +-spread (anywhere when spread<0); gap = the least distance to other plates in percent of the two radii (100 = just not touching)
static int gwPlate(int cx,int cy,int R,int spread,int gap,int s,int k){
    if(gpN>=GW_P||R<2) return 0;
    for(int t=0;t<60;t++){
        int x,y;
        if(spread<0){ x=(int)(hsh(k,t,s+11)%(u32)WW); y=(int)(hsh(k,t,s+12)%(u32)WH); }
        else { x=cx+(int)(hsh(k,t,s+13)%(u32)(2*spread+1))-spread; y=cy+(int)(hsh(k,t,s+14)%(u32)(2*spread+1))-spread; }
        if(cEdge==2){ x=(x%WW+WW)%WW; y=(y%WH+WH)%WH; }
        else { int m=R/2+4; if(x<m||y<m||x>WW-1-m||y>WH-1-m) continue; }
        int ok=1; for(int i=0;i<gpN;i++) if(gwDist(x,y,gpX[i],gpY[i])*100<(R+gpR[i])*gap){ ok=0; break; }
        if(!ok) continue;
        gpX[gpN]=x; gpY[gpN]=y; gpR[gpN]=R; gpN++; return 1; }
    return 0;
}
static void gwIsles(int n,int s,int*k,int gap){ int u=WW; for(int i=0;i<n;i++){ gwPlate(0,0,u/45+(int)(hsh(i,2,s)%(u/40+1)),-1,gap,s,(*k)++); } }
static void gwSetup(int s){
    gpN=0; int u=WW, k=0;
    switch(cShape){
    case 1:   // one supercontinent and a few isles
        gwPlate(u/2,u/2,u*30/100,u/10,0,s,k++); gwIsles(3,s,&k,105); break;
    case 2: { // many separate islands
        int n=10+u/10; for(int i=0;i<n;i++) gwPlate(0,0,u/36+(int)(hsh(i,5,s)%(u/30+1)),-1,120,s,k++); break; }
    case 3: { // archipelagos: clusters of small plates that touch into chains, with open sea between the clusters
        int nc=3+(u>=192)+(u>=256), cxs[6], cys[6], got=0;
        for(int t=0;t<200&&got<nc;t++){ int x=(int)(hsh(t,1,s+21)%(u32)u), y=(int)(hsh(t,2,s+22)%(u32)u);
            if(cEdge!=2&&(x<u/6||y<u/6||x>u-u/6||y>u-u/6)) continue;
            int ok=1; for(int i=0;i<got;i++) if(gwDist(x,y,cxs[i],cys[i])<u*28/100) ok=0;
            if(ok){ cxs[got]=x; cys[got]=y; got++; } }
        for(int c=0;c<got;c++) for(int i=0;i<7;i++) gwPlate(cxs[c],cys[c],u/30+(int)(hsh(c*9+i,3,s)%(u/36+1)),u/8,70,s,k++);
        break; }
    case 4:   // two continents, one on each side of the planet
        gwPlate(u/4,u/2,u*24/100,u/10,0,s,k++); gwPlate(u*3/4,u/2,u*24/100,u/10,85,s,k++); gwIsles(4+u/64,s,&k,110); break;
    case 5:   // one great land with inland seas
        gwPlate(u/2,u/2,u*36/100,u/12,0,s,k++); gwIsles(3,s,&k,105); break;
    default: { // continents far apart, with islands in the oceans between
        int n=3+(u>=192)+(u>=256), big=u*17/100;
        for(int i=0;i<n;i++){ int R=big-(int)(hsh(i,1,s)%(u32)(u/16+1)); if(!gwPlate(0,0,R,-1,125,s,k++)) gwPlate(0,0,R,-1,100,s,k++); }
        int ni=5+u/32; for(int i=0;i<ni;i++) gwPlate(0,0,u/40+(int)(hsh(i,2,s)%(u32)(u/32+1)),-1,115,s,k++);
        break; }
    }
}
static void genWorld(u32 seed){
    int s=(int)(seed&0xFFFF); gwSetup(s); int lakes=cShape==5;
    for(int y=0;y<WH;y++) for(int x=0;x<WW;x++){
        int hn=wfbm(x,y,s), m=wfbm(x,y,s+9), lat=(y<WH/2?y:WH-1-y)*128/WH, temp=40+lat*3+(wfbm(x,y,s+20)-128)/3+cClim*28; m+=cWet*30;
        int warp=wfbm(x,y,s+30)-128, best=-256;   // the plate that holds this cell: how far inside it (256 = middle, 0 = coast, negative = open sea)
        for(int k=0;k<gpN;k++){ int R=gpR[k]; int d=gwDist(x,y,gpX[k],gpY[k]); if(d>2*R) continue; d+=warp*R/220; int n=(R-d)*256/R; if(n>best) best=n; }
        int h=92+best*105/256+(hn-128)/3+(wnoise(x,y,6,s+41)-128)/10;
        if(lakes){ int l=wfbm(x,y,s+50); if(l>145&&best>20) h-=(l-145)*3; }
        if(cEdge!=2){ int e=x<y?x:y, e2=(WW-1-x)<(WH-1-y)?(WW-1-x):(WH-1-y); if(e2<e) e=e2; if(e<10) h-=(10-e)*7; }   // walls: an ocean border
        h+=cSea*9;
        u8 t;
        if(h<62) t=T_DEEP; else if(h<84) t=T_WATER; else if(h<94) t=(temp<70)?T_SNOW:T_SAND;
        else if(h>205) t=(temp<150)?T_SNOW:T_MOUNT; else if(h>170) t=T_HILL;
        else if(temp<70) t=T_SNOW; else if(temp>200&&m<110) t=T_DESERT; else if(temp>190&&m>150) t=T_JUNGLE; else if(m>175&&h<115) t=T_SWAMP; else if(m>135) t=T_FOREST; else t=T_GRASS;
        world[y][x]=t; }
    // a few lava pools in the mountains
    for(int i=0;i<40;i++){ int x=rnd()%WW, y=rnd()%WH; if(world[y][x]==T_MOUNT){ world[y][x]=T_LAVA; if(x+1<WW&&world[y][x+1]==T_MOUNT) world[y][x+1]=T_LAVA; } }
}
