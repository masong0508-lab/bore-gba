// The same spherical planet must appear in title, settings, creation,
// save slots and load UI on DS, DSi enhanced and New3DS. SDK-free regression.
// Build e.g. gcc -O1 -std=c11 -DNDSSHIM -DGG_PORTABLE_GLOBE_TEST
//   -Icompat -Isource -include unistd.h pc_title_globe.c -lm -o /tmp/gg-title
#define main ggTitleLegacyMain
#include "source/main.c"
#undef main
static int failed;
#define CHECK(e,msg) do{if(!(e)){fprintf(stderr,"TITLE GLOBE FAIL: %s\n",msg);failed++;}}while(0)
void shimInput(int frame,int *held,int *tx,int *ty){
    (void)frame;*held=*tx=*ty=0;
}
void shimPresent(void){}

static void sphereVerify(int cx,int cy,int R,const char*name){
    int center=uiBuf[cy*256+cx],inside=0,halo=0;
    for(int y=-R;y<=R;y++)for(int x=-R;x<=R;x++){
        if(x*x+y*y>R*R)continue;
        int px=cx+x,py=cy+y;
        if((unsigned)px>=256||(unsigned)py>=192)continue;
        int color=uiBuf[py*256+px];
        if(color>=100&&color<100+T_N*3)inside++;
    }
    for(int y=-R-2;y<=R+2;y++)for(int x=-R-2;x<=R+2;x++){
        int d=x*x+y*y,px=cx+x,py=cy+y;
        if(d<R*R||d>(R+3)*(R+3)||
           (unsigned)px>=256||(unsigned)py>=192)continue;
        if(uiBuf[py*256+px]==U_CYAN)halo++;
    }
    if(center<100||center>=100+T_N*3||inside<R*R/2||halo<R){
        fprintf(stderr,"TITLE GLOBE FAIL: %s center %d shaded %d rim %d\n",
                name,center,inside,halo);
        failed++;
    }
}
// Compare the optimized title's actual pixels against the original
// coordinate formulas; ignore the outer halo where the rim overlaps.
static void compareTitlePixels(int R){
    const int B=GG_TITLE_GLOBE_RADIUS,D=GG_TITLE_GLOBE_DIAMETER;
    int shift=(int)(((unsigned)tick>>5)*(unsigned)WW/600u);
    for(int y=-R+4;y<=R-4;y++)for(int x=-R+4;x<=R-4;x++){
        if(x*x+y*y>(R-4)*(R-4))continue;
        int k=((y*B/R)+B)*D+((x*B/R)+B);
        if(plJ[k]==255)continue;
        int wx=((plLon[k]*WW>>12)+shift)%WW;
        int wy=plWy[k]*WH>>12;
        if(wy<0)wy=0;if(wy>=WH)wy=WH-1;
        if(uiBuf[(96+y)*256+128+x]!=100+world[wy][wx]*3+plJ[k]){
            CHECK(0,"optimized title changed interior sphere pixels");return;
        }
    }
}
static void sharedTuningVerify(void){
    const int widths[]={64,128,192,256,512,768,1024};
    const int ticks[]={0,31,32,9000,1234567,2147483647};
    for(unsigned w=0;w<sizeof(widths)/sizeof(widths[0]);w++){
        for(unsigned t=0;t<sizeof(ticks)/sizeof(ticks[0]);t++){
            int width=widths[w];
            int shift=(int)(((unsigned)ticks[t]>>5)*(unsigned)width/600u);
            int phase=ggTitlePhase(ticks[t],width);
            for(int lon=0;lon<4096;lon++){
                if(ggTitleLongitude(lon,width,phase)!=((lon*width>>12)+shift)%width){
                    CHECK(0,"title rotation diverged from reference");return;
                }
            }
        }
    }
#ifdef GG_HAS_GLOBE
    int savedW=WW,savedH=WH;
    for(int size=128;size<=MAXD;size*=2){
        gW=size;gH=size/2;globeCloudTables();
        for(int x=0;x<WW;x++)CHECK(gCloudX[x]==x*64/WW,"cloud longitude cache");
        for(int y=0;y<WH;y++)CHECK(gCloudY[y]==y*64/WH,"cloud latitude cache");
    }
    gW=savedW;gH=savedH;globeCloudTables();
    CHECK(gCloudW==WW&&gCloudH==WH,"cloud cache invalidated on restored dimensions");
#endif
    // Drive the real world event after visual tests, with a richer hostile
    // donor and a richer unrelated donor to prove eligibility restrictions.
    memset(vil,0,sizeof vil);memset(vilK,0,sizeof vilK);
    for(int v=0;v<5;v++){vil[v].on=1;vil[v].pop=3;}
    vil[0].stock=30;vil[1].stock=250;
    vil[2].stock=390;vil[2].foe=1;
    vil[3].stock=390;vil[3].race=1;
    vil[4].stock=50;
    jrN=0;
    eventDo(EV_RELIEF);
    CHECK(vil[0].stock==90&&vil[1].stock==190,"relief conserves stocks and prioritizes the poorest");
    CHECK(vil[2].stock==390&&vil[3].stock==390,"relief skips hostile or unrelated donors");
    CHECK(jrN==1&&strstr(jr[0],"RECEIVES RELIEF")!=NULL,"relief enters journal");
    CHECK(reliefCaravan()==10&&vil[1].stock==180&&vil[4].stock==60,"donor keeps its reserve");
    CHECK(reliefCaravan()==0&&jrN==2,"no phantom relief without available surplus");
    CHECK(ggReliefAmount(300,79)==21&&ggReliefAmount(400,80)==0,"aid threshold and recipient cap");
}
// These tests exercise the shared implementation used by all native ports.
static void livingWorldVerify(void){
    CHECK(ggBestiaryMove(319,1,MON_N)==320,"bestiary can pass the old 320-entry cap");
    CHECK(ggBestiaryMove(MON_N-5,10,MON_N)==MON_N-1,"bestiary clamps final page");
    CHECK(ggBestiaryMove(2,-10,MON_N)==0,"bestiary clamps first page");
    for(int dy=-10;dy<=10;dy++)for(int dx=-10;dx<=10;dx++)
        CHECK((dx>=-ggCircleHalfWidth10(dy)&&dx<=ggCircleHalfWidth10(dy))==
              (dx*dx+dy*dy<=100),"optimized village circle equals original");
    memset(vil,0,sizeof vil);memset(vilK,0,sizeof vilK);memset(kd,0,sizeof kd);
    memset(achX,0,sizeof achX);achBits=0;
    for(int v=0;v<12;v++){vil[v].on=1;vil[v].pop=3;vil[v].lvl=v==7?3:1;}
    kd[0].on=1;kd[0].empire=1;kd[0].tech=9;
    kd[KN_N-1].on=1;kd[KN_N-1].lord=1;
    achCheck();
    CHECK(ACH_HAS(9)&&ACH_HAS(10)&&ACH_HAS(11)&&ACH_HAS(14)&&ACH_HAS(15)&&ACH_HAS(16),
          "single achievement snapshot retains city and kingdom milestones");
    memset(kd,0,sizeof kd);achCheck();
    CHECK(ACH_HAS(34),"history remembers the last kingdom even after it falls");
    // The most desperate city is isolated. The next eligible city must
    // still receive aid instead of the whole event being blocked.
    memset(vil,0,sizeof vil);memset(vilK,0,sizeof vilK);
    for(int v=0;v<3;v++){vil[v].on=1;vil[v].pop=3;}
    vil[0].stock=1;vil[0].race=1;vil[1].stock=40;vil[2].stock=250;
    CHECK(reliefCaravan()==60&&vil[0].stock==1&&vil[1].stock==100&&vil[2].stock==190,
          "isolated poorest city no longer blocks a feasible relief delivery");
    // Reconstruction uses the real terrain, ownership and stock arrays.
    memset(vil,0,sizeof vil);memset(vilK,0,sizeof vilK);
    vil[0].on=1;vil[0].pop=8;vil[0].stock=100;vil[0].x=32;vil[0].y=32;
    for(int y=28;y<=36;y++)for(int x=28;x<=36;x++){
        world[y][x]=T_GRASS;bld[y][x]=B_NONE;bldv[y][x]=0;occ[y][x]=0;
    }
    world[32][31]=T_DIRT;bld[32][31]=B_RUIN;
    bld[32][33]=B_RUIN;bldv[32][33]=2; // another town's property
    bld[31][32]=B_HOUSE;bldv[31][32]=1; // intact home
    jrN=0;
    eventDo(EV_REBUILD);
    CHECK(bld[32][31]==B_NONE&&world[32][31]==T_GRASS&&vil[0].stock==92,
          "reconstruction reclaims a ruin using real town supplies");
    CHECK(bld[32][33]==B_RUIN&&bld[31][32]==B_HOUSE,
          "reconstruction respects foreign plots and existing homes");
    CHECK(jrN==1&&strstr(jr[0],"REBUILDS")!=NULL,"reconstruction event is recorded");
    CHECK(rebuildCaravan()==0&&jrN==1,"no fake reconstruction on an intact city");
    // With no creatures, the simulation must still advance weather.
    memset(vil,0,sizeof vil);memset(vilK,0,sizeof vilK);memset(occ,0,sizeof occ);
    nCr=0;nVilOn=0;cellClear();gxDefaults();
    if(!LAWON(LW_WEATHER))lw[LW_WEATHER]^=1;
    tick=1;paused=0;weather=WX_CLEAR;weatherT=15;
    simulate();
    CHECK(weatherT==14,"lifeless planet still advances weather");
}
int main(void){
    mapPtr=(u16*)bgGetMapPtr(0);
    rs=773;planetSize=128;
    gxDefaults();makeSpecies();makeItems();newWorld();
    uiPalette();uiShadePal();
    // A deterministic globe with different continents at different
    // longitudes makes rotation observable without depending on the
    // world generator's random seed.
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++)
        world[y][x]=(u8)(((x/13)+(y/17))%T_N);
    tick=0;
    memset(uiBuf,U_BG,sizeof uiBuf);
    uiWorldPreview(128,96,32);
    CHECK(uiBuf[0]==U_BG,"3D title preview writes outside the sphere");
    sphereVerify(128,96,32,"portable sphere");
    compareTitlePixels(32);
    static u8 still[256*192];memcpy(still,uiBuf,sizeof still);
    tick=9000;uiWorldPreview(128,96,32);
    CHECK(memcmp(still,uiBuf,sizeof still)!=0,
          "title planet stopped rotating with the world");
    compareTitlePixels(32);
    // The live title and all settings submenus draw the same sphere.
    mScr=0;mSel=0;uiTitle();
    sphereVerify(206,128,34,"main title");
    mScr=1;uiTitle();
    sphereVerify(215,108,30,"options");
    mScr=2;uiTitle();
    sphereVerify(215,108,30,"extras");
    cSel=0;cLand=45;mScr=3;uiCreate();
    sphereVerify(208,63,41,"world creator");
    uiLoadHook("READY",100);
    sphereVerify(128,76,GG_TITLE_GLOBE_RADIUS,"loading");
    slSel=0;uiSlots();
    if(NSLOT<=4)sphereVerify(208,144,25,"save slots");
    CHECK(GG_TITLE_GLOBE_DIAMETER<=109,
          "DS/DSi sphere cache exceeds native 3DS resource budget");
#ifdef GG_HAS_GLOBE
    zoom3=30;
#ifndef __3DS__
    CHECK(globeR()<=88,"DS/DSi planet exceeded ARM9 pixel budget");
#endif
    CHECK(globePick(128,96,&camX,&camY),
          "full planetary view no longer picks the centre");
    globePalette();globeDraw(1);
    CHECK(gbuf[96*256+128]!=1,
          "restored interactive planet still renders only empty space");
#endif
    sharedTuningVerify();
    livingWorldVerify();
    printf(failed?"TITLE GLOBE FAIL (%d)\n":
           "TITLE GLOBE OK: rotating lit sphere, menus, planet zoom, pixel parity, cloud cache, relief event\n",
           failed);
    return failed?1:0;
}
