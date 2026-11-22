// Riven March uses a 1024-unit survey at every planet size. Continental features
// stay in the same places in the preview and the world. Seeded noise sets the
// local contours, rock and vegetation. Only the finished terrain tiles persist.
enum { MARCH_SPAN=1024, MARCH_UPPER_FORD=362, MARCH_LOWER_FORD=666 };

static const u16 marchSites[][2]={
    {511,520}, // central meadows
    {308,424}, // western woods
    {632,304}, // high pass
    {745,572}, // eastern foothills
    {500,710}, // south bank
    {180,566}, // western headland
    {649,628}, // east bank
    {750,748}, // above the bay
};

static void marchSite(int i,int *x,int *y){
    *x=marchSites[i][0]*WW/MARCH_SPAN;
    *y=marchSites[i][1]*WH/MARCH_SPAN;
}

static int marchDist(int dx,int dy){
    dx=abs(dx); dy=abs(dy);
    return dx>dy?dx+dy/2:dy+dx/2;
}

// Samples along north/south or east/west features, spaced 128 units apart.
static int marchProfile(const u16 *samples,int at){
    int i=at/128, t=at%128;
    int a=samples[i?i-1:0], b=samples[i], c=samples[i+1], d=samples[i<7?i+2:8];
    // Cubic interpolation keeps the river's direction continuous at the survey points.
    int bend=2*a-5*b+4*c-d+(-a+3*b-3*c+d)*t/128;
    return b+((c-a+bend*t/128)*t/128)/2;
}

static int marchShore(int x,int y,int broad,int detail){
    int shore=421-marchDist(x-488,(y-485)*9/10);
    int headland=123-marchDist(x-162,y-477);
    if(headland>shore) shore=headland;
    int cape=134-marchDist(x-762,y-674);
    if(cape>shore) shore=cape;
    int bay=marchDist(x-631,y-924)-174;
    if(bay<shore) shore=bay;
    int inlet=marchDist(x-153,y-151)-122;
    if(inlet<shore) shore=inlet;
    int sound=marchDist(x-881,y-694)-84;
    if(sound<shore) shore=sound;
    int skerry=31-marchDist(x-827,y-838);
    if(skerry>shore) shore=skerry;
    return shore+(broad-128)*(cCoast+4)/12+(detail-128)*(cCoast+2)/21-cSea*9;
}

static void genMarch(u32 seed){
    static const u16 riverX[]={373,391,472,521,565,530,618,735,827};
    static const u16 ridgeY[]={220,230,246,177,200,230,176,215,218};
    int s=(int)(seed&0xffff);
    int xScale=((MARCH_SPAN<<16)+WW-1)/WW, yScale=((MARCH_SPAN<<16)+WH-1)/WH;
    int springX=marchProfile(riverX,218)+(vnoise(0,218,64,s+3)-128)/7;
    for(int ty=0;ty<WH;ty++){
        int y=ty*yScale>>16;
        int river=marchProfile(riverX,y)+(vnoise(0,y,64,s+3)-128)/7;
        int channel=4+y/112+(vnoise(0,y,64,s+11)-128)/56;
        if(channel<4) channel=4;
        int ford=abs(y-MARCH_UPPER_FORD)<=5||abs(y-MARCH_LOWER_FORD)<=5;
        int temperature=76+y/9+cClim*24;
        for(int tx=0;tx<WW;tx++){
            int x=tx*xScale>>16;
            int broad=vnoise(x,y,128,s), detail=vnoise(x,y,32,s+1);
            int shore=marchShore(x,y,broad,detail);
            if(x<14||x>=MARCH_SPAN-14||y<14||y>=MARCH_SPAN-14||shore<-18){ world[ty][tx]=T_DEEP; continue; }
            if(shore<0){ world[ty][tx]=T_WATER; continue; }

            int bank=abs(x-river);
            if(y>=216&&bank<=channel){ world[ty][tx]=ford?T_SAND:T_WATER; continue; }
            int spring=marchDist(x-springX,y-219)+(detail-128)/18;
            if(spring<25){ world[ty][tx]=T_WATER; continue; }
            if(shore<5+(detail-128)/29||spring<30||(y>=216&&bank<=channel+3)){
                world[ty][tx]=T_SAND; continue;
            }

            int ridge=marchProfile(ridgeY,x)+(broad-128)/4;
            int alpine=93-abs(y-ridge)+(detail-128)/2;
            int end=x<510?x-176:844-x;
            // Two saddles interrupt the northern ridge and open the far coast.
            int pass=abs(x-366-(y-210)/5-(detail-128)/10);
            int eastPass=abs(x-632+(y-210)/6+(detail-128)/12);
            if(eastPass<pass) pass=eastPass;
            if(alpine>end) alpine=end;
            if(alpine>15+pass) alpine=15+pass;
            int escarpmentX=750+(y-420)/7+(broad-128)/3;
            int escarpment=64-abs(x-escarpmentX)-abs(y-500)/3+(detail-128)/3;
            int rise=alpine>escarpment?alpine:escarpment;
            int meadowRise=48-marchDist(x-523,y-532);
            if(meadowRise>rise) rise=meadowRise;
            if(y>=215&&bank<56&&rise>bank/2) rise=bank/2;
            int wet=118+(broad-128)/2+(detail-128)/3+cWet*26;
            wet+=x<472?(472-x)/5:x>565?-(x-565)/5:0;
            int copse=15-marchDist(x-490,y-533);
            if(copse>0) wet+=copse*4;
            int floodplain=y+(broad-128)/4+(detail-128)/3-660;
            if(floodplain>0&&bank<95){
                int floodWet=(95-bank)/2+10;
                wet+=floodplain<floodWet?floodplain:floodWet;
            }

            u8 terrain;
            if(rise>66-cMtn*12) terrain=temperature<114?T_SNOW:T_MOUNT;
            else if(rise>35-cMtn*8) terrain=T_HILL;
            else if(temperature+(detail-128)/3<64) terrain=T_SNOW;
            else if(floodplain>18&&bank<95+(detail-128)/7&&wet>132) terrain=T_SWAMP;
            else if(y>=250&&y<=680&&bank<47+(detail-128)/8) terrain=T_GRASS;
            else if(wet<100&&temperature+(detail-128)/4>118) terrain=T_DESERT;
            else if(wet>164&&temperature>174) terrain=T_JUNGLE;
            else if(wet>143) terrain=T_FOREST;
            else terrain=T_GRASS;
            world[ty][tx]=terrain;
        }
    }
    // Clear small building sites above the floodplain. This also keeps the
    // opening view on workable land in every climate.
    int radius=2+WW/192;
    for(int i=0;i<(int)(sizeof marchSites/sizeof marchSites[0]);i++){
        int x,y; marchSite(i,&x,&y);
        for(int dy=-radius;dy<=radius;dy++) for(int dx=-radius;dx<=radius;dx++){
            if(dx*dx+dy*dy>radius*radius) continue;
            if(world[y+dy][x+dx]>T_WATER) world[y+dy][x+dx]=T_GRASS;
        }
    }
    world[WH/2][WW/2]=T_GRASS;
}
