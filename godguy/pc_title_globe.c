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
    static u8 still[256*192];memcpy(still,uiBuf,sizeof still);
    tick=9000;uiWorldPreview(128,96,32);
    CHECK(memcmp(still,uiBuf,sizeof still)!=0,
          "title planet stopped rotating with the world");
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
    // The WORLD menu's narrow gutter is deliberately outside every
    // row and button; its centre and interior must depict live terrain.
    fieldLedgerOpen=0;wTab=wTabPrev=wSel=0;uiWorld();
    int worldGlobePixels=0;
    for(int dy=-8;dy<=8;dy++)for(int dx=-8;dx<=8;dx++){
        if(dx*dx+dy*dy>64)continue;
        int col=uiBuf[(46+dy)*256+243+dx];
        worldGlobePixels+=col>=100&&col<100+T_N*3;
    }
    CHECK(worldGlobePixels>=50,
          "WORLD settings globe is absent or covered by another panel");
    CHECK(GG_TITLE_GLOBE_DIAMETER<=109,
          "DS/DSi sphere cache exceeds native 3DS resource budget");
#ifdef GG_HAS_GLOBE
    zoom3=30;
#ifndef __3DS__
    CHECK(globeR()<=88,"DS/DSi planet exceeded ARM9 pixel budget");
    CHECK(GR_MAX==88&&sizeof gnz==2u*(88u*88u+1u)&&
          sizeof gxn==2u*(2u*88u+1u),
          "DS/DSi still reserves normals beyond its maximum rendered radius");
#endif
    CHECK(globePick(128,96,&camX,&camY),
          "full planetary view no longer picks the centre");
    globePalette();globeDraw(1);
    CHECK(gbuf[96*256+128]!=1,
          "restored interactive planet still renders only empty space");
#endif
    printf(failed?"TITLE GLOBE FAIL (%d)\n":
           "TITLE GLOBE OK: rotating lit sphere, menus, creator, loading, slots, playable planet zoom\n",
           failed);
    return failed?1:0;
}
