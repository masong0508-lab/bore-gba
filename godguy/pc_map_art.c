// Source-rendered DS map art and palette/VRAM regressions. Native 256x192
// captures use the actual shared terrain, building, sprite and zoom paths.
// This harness is not Nintendo DS emulator footage.
// gcc -O2 -DNDSSHIM -Icompat -Isource -include unistd.h \
//     pc_map_art.c -lm -o /tmp/gg-map-art
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
void shimInput(int f,int*k,int*x,int*y){(void)f;*k=0;*x=*y=-1;}
void shimPresent(void){}
static int artErrors;
#define ART_CHECK(c,m) do{if(!(c)){fprintf(stderr,"MAP ART FAIL: %s\n",m);artErrors++;}}while(0)
static int artSave(const char*dir,const char*name){
    static u8 rgb[320*240*3];char path[512];
    memset(rgb,0,sizeof rgb);shimLut(shimLutM,BG_PALETTE);
    shimLut(shimLutP,SPRITE_PALETTE);shimDrawMain(rgb,320,32);
    if(snprintf(path,sizeof path,"%s/%s.ppm",dir,name)>=(int)sizeof path)return 0;
    FILE*f=fopen(path,"wb");if(!f)return 0;
    fprintf(f,"P6\n256 192\n255\n");
    int ok=1;for(int y=24;y<216;y++)if(fwrite(rgb+(y*320+32)*3,1,256*3,f)!=256*3)ok=0;
    if(fclose(f))ok=0;return ok;
}
static void artScene(void){
    gW=gH=128;rs=777;makeSpecies();makeItems();
    memset(bld,0,sizeof bld);memset(bldv,0,sizeof bldv);memset(occ,0,sizeof occ);
    memset(world,T_DEEP,sizeof world);bioClear();nCr=0;
    for(int y=0;y<24;y++)for(int x=0;x<32;x++){
        int coast=5+(y/5)+(y>14?((y-14)/2):0);
        int t=x<coast-2?T_DEEP:x<coast?T_WATER:x==coast?T_SAND:T_GRASS;
        if(x>coast+1&&y<5)t=x>24?T_SNOW:x>20?T_MOUNT:x>17?T_HILL:T_FOREST;
        if(x>27&&y>4&&y<18)t=y<12?T_FOREST:T_JUNGLE;
        if(x>22&&y>18)t=T_SWAMP;
        if(x>coast+1&&x<17&&y>17)t=T_DESERT;
        world[y+48][x+48]=(u8)t;
    }
    static const u8 buildings[][3]={{15,8,B_HALL},{12,10,B_HOUSE},{15,10,B_HOUSE},
        {20,8,B_LIBRARY},{23,7,B_TEMPLE},{22,11,B_STATUE},{25,12,B_TOWER},
        {19,14,B_FARM},{20,14,B_FARM},{19,15,B_FARM},{20,15,B_FARM},
        {14,14,B_HUT},{8,12,B_DOCK},{25,18,B_RUIN},{21,18,B_BORDER},
        {17,8,B_MARKET},{19,8,B_INN},{20,11,B_WORKSHOP},
        {16,9,B_PAVING},{17,9,B_PAVING},{18,9,B_PAVING},{19,9,B_PAVING},{20,9,B_PAVING},
        {17,10,B_PAVING},{18,10,B_WELL},{19,10,B_PAVING},
        {17,11,B_PAVING},{18,11,B_PAVING},{19,11,B_PAVING},{18,12,B_PAVING}};
    for(unsigned i=0;i<sizeof buildings/sizeof *buildings;i++){
        int x=buildings[i][0]+48,y=buildings[i][1]+48;
        bld[y][x]=buildings[i][2];
    }
    int human=0;for(int s=0;s<nSp;s++)if(arch[sp[s].a].diet==4){human=s;break;}
    for(int i=0;i<18;i++){
        Cr*c=&cr[i];memset(c,0,sizeof *c);c->sp=(u16)human;c->alive=1;
        c->x=(coord_t)(48+11+i%8*2);c->y=(coord_t)(48+6+i/8*5);
        occ[c->y][c->x]=(occ_t)(i+1);crId[i]=(u32)(i+1);
    }
    nCr=18;camX=48*8;camY=48*8;winX=winY=-99;mapPtr=(u16*)bgGetMapPtr(0);
    mapPtr1=(u16*)bgGetMapPtr(1);
    makeTiles();makeSprites();uiPalette();tbInit();saveBasePal();
    memset(shimMap1,0,sizeof shimMap1); // capture the map without unrelated toolbar state
    for(int i=0;i<1024;i++)shimMap1[i]=TB_BLANK;
    memset(shimTiles+TB_BLANK*64,0,64);
    tick=2048;weather=WX_CLEAR;applyLight();refillWindow();
    bgSetScroll(0,camX,camY);drawSprites();
}
static void artPlazaInvariants(void){
    const int x=16,y=17;
    const u8 *paving=shimTiles+BTILE(B_PAVING,0)*64;
    int stone=0,shades=0;
    for(int i=0;i<64;i++)if(paving[i]/8==T_MOUNT){stone++;shades|=1<<(paving[i]&7);}
    ART_CHECK(stone==64&&__builtin_popcount(shades)>=3,"paving lacks solid, shaded stonework");
    int paveFar=bZoomCol(B_PAVING,0),wellFar=bZoomCol(B_WELL,0);
    ART_CHECK(paveFar/8==T_MOUNT&&wellFar/8==T_MOUNT&&wellFar>paveFar,"far zoom loses plaza and well colors");

    zSet(Z_NORMAL);
    world[y][x]=T_GRASS;bld[y][x]=B_WELL;bldv[y][x]=1;
    for(int r=0;r<8;r++){
        const u8 *well=shimTiles+BTILE(B_WELL,r)*64;
        const u8 *racePaving=shimTiles+BTILE(B_PAVING,r)*64;
        int rim=0,mouth=0,timber=0,roof=0;
        for(int i=0;i<64;i++){
            rim+=well[i]==SH(T_MOUNT,7);
            mouth+=well[i]==SH(T_MOUNT,0);
            timber+=well[i]==137||well[i]==SH(T_DIRT,5);
            roof+=well[i]==PX_ROOFL+r;
        }
        ART_CHECK(!memcmp(paving,racePaving,64),"paving changes color with the village race");
        ART_CHECK(rim>=3&&mouth>=2&&timber>=2&&roof>=2,"well lacks a stone rim, opening or roof supports");
        ART_CHECK(bZoomCol(B_PAVING,r)==paveFar&&bZoomCol(B_WELL,r)==wellFar,"far zoom uses race colors for plaza stone");

        vil[0].race=(u8)r;
        putCell(x,y);
        ART_CHECK(mapPtr[y*32+x]==BTILE(B_WELL,r),"normal map selects the wrong well tile");
        ART_CHECK(!memcmp(zArtOf(x,y),well,64),"close zoom selects the wrong well art");
    }
    // An odd race and the high bit of the building type used to overlap.
    bld[y][x]=B_MANOR;
    ART_CHECK(!memcmp(zArtOf(x,y),shimTiles+BTILE(B_MANOR,7)*64,64),"close zoom keeps the art of the previous building");
    bld[y][x]=B_PAVING;
    putCell(x,y);
    ART_CHECK(mapPtr[y*32+x]==BTILE(B_PAVING,7),"normal map selects the wrong paving tile");
    ART_CHECK(!memcmp(zArtOf(x,y),paving,64),"close zoom selects the wrong paving art");
}
static void artConstructionPreview(void){
    zSet(Z_CLOSE);
    zcx=66*8+4;zcy=58*8+4;zClamp();
    constructionGhostX=66;constructionGhostY=58;
    constructionGhost.count=1;
    constructionGhost.cell[0]=(GgPlanCell){0,0,B_WELL};
    constructionGhostValid=1;
    int x=66*zS_-zOx(),y=58*zS_-zOy();
    zForce=1;zDraw();
    u8 ground=gbuf[y*256+x];
    constructionGhostShown=1;
    ART_CHECK(zDraw()&&gbuf[y*256+x]==UIP(U_GREEN),
              "close zoom does not show a valid construction footprint");
    constructionCancelAim();
    ART_CHECK(zDraw()&&gbuf[y*256+x]==ground,
              "canceling a zoomed footprint leaves a stale map outline");
    zSet(Z_NORMAL);
}
static void artInvariants(void){
    // A roof's normal palette must remain intact across zoom and lighting.
    // The old zoom pool reused roof entries and collapsed families 9..15.
    makeTiles();uiPalette();saveBasePal();tick=2048;applyLight();
    u16 roofs[16];for(int i=0;i<16;i++)roofs[i]=BG_PALETTE[144+i];
    zPalette();for(int i=0;i<16;i++)ART_CHECK(BG_PALETTE[144+i]==roofs[i],"zoom recolors building roofs");
    for(int a=0;a<16;a++)for(int b=a+1;b<16;b++)
        ART_CHECK(zFamPal(a)!=zFamPal(b),"creature families alias in the close view");
    u16 families[16][3];for(int f=0;f<16;f++)for(int k=0;k<3;k++)families[f][k]=BG_PALETTE[zFamPal(f)+k];
    uiShadePal();for(int f=0;f<16;f++)for(int k=0;k<3;k++)
        ART_CHECK(BG_PALETTE[zFamPal(f)+k]==families[f][k],"toolbar swatches overwrite creature colors");
    // Every tile must stay within the world palette and off toolbar slots.
    for(int t=0;t<T_N;t++)for(int v=0;v<4;v++)for(int y=0;y<8;y++)for(int x=0;x<8;x++){
        int p=tilePix(t,v,x,y);ART_CHECK(p>=0&&p<GFX_PAL_N&&!UI_RESV(p),"ground writes a reserved palette index");
    }
    for(int b=1;b<B_N;b++)for(int r=0;r<8;r++)for(int y=0;y<8;y++)for(int x=0;x<8;x++){
        int p=bldPix(b,r,x,y);ART_CHECK(p>=0&&p<GFX_PAL_N&&!UI_RESV(p),"building writes a reserved palette index");
        ART_CHECK(shimTiles[BTILE(b,r)*64+y*8+x]==p,"building tile upload differs from map art");
    }
    artPlazaInvariants();
    artConstructionPreview();
    // Magic-biome keys exceed16 bits. Repeated redraws must reuse their
    // transition tiles rather than consuming the entire484-slot cache.
    memset(trKey,0,sizeof trKey);trUsed=0;
    for(int pass=0;pass<2;pass++){
        int before=trUsed;
        for(int b=1;b<=3;b++)for(int t=T_SAND;t<T_N;t++)for(int v=0;v<4;v++){
            int slot=trTile(t,T_WATER,5,v,0,b);
            ART_CHECK(slot>=TR0&&slot<TR0+TR_SLOTS,"transition cache exceeds VRAM tile area");
        }
        if(pass)ART_CHECK(trUsed==before,"special biomes leak transition cache slots");
    }
    memset(trKey,0,sizeof trKey);trUsed=0;
    u8 toolbar[64];memcpy(toolbar,shimTiles+(TR0+TR_SLOTS)*64,64);
    for(int q=0;q<TR_SLOTS+16;q++){
        int slot=trTile(T_SAND+q/64,T_WATER,q&15,(q>>4)&3,0,3);
        ART_CHECK(slot==(q<TR_SLOTS?TR0+q:-1),"distinct transition keys collide or overfill the cache");
    }
    ART_CHECK(!memcmp(toolbar,shimTiles+(TR0+TR_SLOTS)*64,64),"transition cache overwrites toolbar tiles");
    for(int q=0;q<TR_SLOTS;q++)
        ART_CHECK(trTile(T_SAND+q/64,T_WATER,q&15,(q>>4)&3,0,3)==TR0+q,
                  "full transition cache loses an earlier tile");
    // Enlarging terrain must preserve the normal view's chosen meadow and
    // variation, including freshly changed coasts and special biomes.
    memset(zart,0,sizeof zart);
    for(int y=48;y<69;y++)for(int x=48;x<80;x++){
        if(bld[y][x])continue;
        int t=world[y][x],v=tileVar(t,x,y),mask,foam,n=cellEdge(x,y,&mask,&foam);
        const u8*a=zArtOf(x,y);
        for(int py=0;py<8;py++)for(int px=0;px<8;px++){
            int p=n>=0?edgePix(t,n,mask,v,foam,px,py):
                (t==T_GRASS&&v==2?pxMeadow((int)(hsh(x,y,5)&15),px,py):tilePix(t,v,px,py));
            ART_CHECK(a[py*8+px]==p,"zoom changes terrain motifs or meadow locations");
        }
    }
}
int main(int argc,char**argv){
    if(argc<2)return 2;mkdir(argv[1],0777);artScene();
    if(!artSave(argv[1],"map_day"))return 3;
    tick=0;applyLight();if(!artSave(argv[1],"map_night"))return 3;
    tick=2048;applyLight();zSet(Z_CLOSE);zcx=66*8;zcy=59*8;zClamp();
    zForce=1;zDraw();zFlush();if(!artSave(argv[1],"map_close"))return 3;
    zSet(Z_MICRO);zcx=66*8+4;zcy=58*8+4;zClamp();
    zForce=1;zDraw();zFlush();if(!artSave(argv[1],"map_micro"))return 3;
    if(argc<3){artInvariants();if(artErrors)return 1;}
    printf("MAP ART OK: native captures; 13 terrains, %d buildings, 16 creature families\n",B_N-1);return 0;
}
