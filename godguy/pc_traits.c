// Regression coverage for the creature-life batch: traits, statuses, damage, settlement mood and save compatibility.
#define PCTEST 1
#include <stdlib.h>
#include <string.h>
#include "source/main.c"

static int failures;
#define CHECK(ok,msg) do { if(!(ok)){ printf("  FAIL: %s\n",msg); failures++; } } while(0)
static void setup(void){
    planetSize=128; rs=0x71A17u; makeSpecies(); makeItems(); newWorld();
    nCr=0; memset(occ,0,sizeof occ); cellClear(); trSeen=trSeenHi=stMut=0;
}
static int findHuman(void){ for(int s=0;s<nSp;s++) if(arch[sp[s].a].diet==4) return s; return -1; }
static int addHumanAt(int sx,int sy){
    int h=findHuman(); if(h<0) return -1;
    for(int r=0;r<WW;r++) for(int y=1;y<WH-1;y++) for(int x=1;x<WW-1;x++){
        if(abs(x-sx)+abs(y-sy)!=r || occ[y][x] || bld[y][x] || !walkable(&sp[h],x,y)) continue;
        int i=addCr(h,x,y); if(i>=0) return i;
    }
    return -1;
}
static int findPower(int kind){ for(int i=0;i<PW_N;i++) if(pw[i].kind==kind) return i; return -1; }
static void saveVersion11(const char *src,const char *dst){
    FILE *in=fopen(src,"rb"), *out=fopen(dst,"wb");
    if(!in||!out){ if(in) fclose(in); if(out) fclose(out); return; }
    fseek(in,0,SEEK_END); long n=ftell(in); rewind(in);
    if(n>8){ long keep=n-8; for(long k=0;k<keep;k++){ int ch=fgetc(in); if(ch==EOF) break; fputc(ch,out); } }
    fclose(in); fclose(out);
    out=fopen(dst,"r+b"); if(out){ SaveHdr h; if(fread(&h,sizeof h,1,out)==1){ h.ver=11; rewind(out); fwrite(&h,sizeof h,1,out); } fclose(out); }
}
int main(void){
    setup();
    CHECK(sizeof(Cr)==(sizeof(coord_t)==1?20:22),"creature record layout remains target-sized");
    int a=addHumanAt(4,4), b=addHumanAt(7,4);
    CHECK(a>=0&&b>=0,"place two test creatures");
    if(a<0||b<0){ printf("TRAITS FAIL\n"); return 1; }

    // Disease must coexist with status flags in the same saved byte.
    cr[a].sick=(u8)(ST_COUGH|1); SET_DISEASE(&cr[a],2);
    CHECK(DISEASE(&cr[a])==2&&HAS_STATUS(&cr[a],ST_COUGH),"disease changes preserve status bits");
    CHECK(!infect(a),"immune creature does not catch plague");
    SET_DISEASE(&cr[a],0); CHECK(infect(a)&&DISEASE(&cr[a])==1&&HAS_STATUS(&cr[a],ST_COUGH),"plague infection preserves independent status");

    // Exercise the actual divine status picker/tap path.
    { int p=findPower(PK_STATUS); CHECK(p>=0,"status power exists"); if(p>=0){ power=p; brush=0; statusSel=3; usePower(cr[b].x,cr[b].y,1); CHECK(HAS_STATUS(&cr[b],ST_MOTIVATED),"status power applies selected status"); CHECK(DISEASE(&cr[b])==0,"status power does not corrupt disease state"); usePower(cr[b].x,cr[b].y,1); CHECK(!HAS_STATUS(&cr[b],ST_MOTIVATED),"status power toggles status off"); } }

    // Defensive traits: bubble halves damage, thorns reflect it, invincible blocks it.
    cr[a].hp=100; cr[b].hp=100; cr[b].tr=TR_BUBBLE; cr[b].tr2=0;
    damageCreature(b,8,a); CHECK(cr[b].hp==96&&cr[a].hp==100,"bubble skin halves incoming damage");
    cr[b].tr=TR_THORNS; cr[b].hp=100; cr[a].hp=100;
    damageCreature(b,8,a); CHECK(cr[b].hp==92&&cr[a].hp==98,"thorns reflect one quarter damage");
    cr[b].tr=0; cr[b].sick=(u8)(DISEASE(&cr[b])|ST_INVINCIBLE); cr[b].hp=100;
    damageCreature(b,200,a); killCr(b); CHECK(cr[b].alive&&cr[b].hp==100,"invincible status blocks lethal damage and direct kill");

    // Adaptation should remove the biome speed penalty; permafrost should confer cold terrain access.
    { Cr test=cr[a]; int fast=0; for(int q=1;q<nSp;q++) if(arch[sp[q].a].spd>arch[sp[fast].a].spd) fast=q; test.sp=(u16)fast; test.x=4; test.y=4; world[test.y][test.x]=T_DESERT; test.tr=test.tr2=0; int slow=effSpd(&test); test.tr=TR_DESERT; int adapted=effSpd(&test); CHECK(adapted<slow,"desert adaptation removes desert movement penalty"); test.tr=TR_PERMA; CHECK((trMask(&test)&(1<<T_SNOW))!=0,"permafrost adaptation permits snow terrain"); }

    // Grin Mark directly improves nearby people’s settlement mood.
    { int v=0; memset(&vil[v],0,sizeof vil[v]); vil[v].on=1; vil[v].race=sp[cr[a].sp].a; vil[v].x=cr[a].x; vil[v].y=cr[a].y; vil[v].stock=100; vil[v].houses=10; vil[v].pop=10; cr[a].tr=0; int before=vilMood(v); cr[a].tr=TR_GRIN; int after=vilMood(v); CHECK(after>=before+4,"Grin Mark raises nearby settlement happiness"); vil[v].on=0; }

    // Save/load new trait discoveries and status bits in the v12 trailer.
    traitMark(TR_DESERT); traitMark(TR_LAMP); trSeenHi&=0x3FFFu;
    cr[a].sick=(u8)(ST_MINDLESS|1); CHECK(saveGame("/tmp/gg_traits_v12.sav"),"write version 12 save");
    trSeenHi=0; cr[a].sick=0; CHECK(loadGame("/tmp/gg_traits_v12.sav"),"load version 12 save");
    CHECK(traitMet(TR_DESERT)&&traitMet(TR_LAMP),"high trait discovery bits survive save/load");
    { int found=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].sick==(ST_MINDLESS|1)) found=1; CHECK(found,"combined disease/status byte survives save/load"); }

    // Version 11 saves have the same body but no GGU1 trailer and must remain loadable.
    saveVersion11("/tmp/gg_traits_v12.sav","/tmp/gg_traits_v11.sav");
    trSeenHi=0; CHECK(loadGame("/tmp/gg_traits_v11.sav"),"load legacy version 11 save without GGU1");
    CHECK(!traitMet(TR_DESERT)&&!traitMet(TR_LAMP),"version 11 has no high trait discoveries");

    printf("%s\n",failures?"TRAITS FAIL":"TRAITS OK");
    return failures?1:0;
}
