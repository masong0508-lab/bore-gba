// Disaster pack regression: the six world events and six new special events share the existing saved hazard record.
#define PCTEST 1
#include <stdlib.h>
#include <string.h>
#include "source/main.c"
static int failures;
#define CHECK(c,m) do{ if(!(c)){ printf("FAIL: %s\n",m); failures++; } }while(0)
static int findPower(int kind){ for(int i=0;i<PW_N;i++) if(pw[i].kind==kind) return i; return -1; }
static int countArch(const char *name){ int n=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&!strcmp(arch[sp[cr[i].sp].a].nm,name)) n++; return n; }
static int findHuman(void){ for(int s=0;s<nSp;s++) if(arch[sp[s].a].diet==4) return s; return -1; }
static void saveVersion12WithoutPowerTail(const char *src,const char *dst){
    FILE *in=fopen(src,"rb"), *out=fopen(dst,"wb"); if(!in||!out){ if(in)fclose(in); if(out)fclose(out); return; }
    fseek(in,0,SEEK_END); long n=ftell(in); rewind(in); long keep=n>=20?n-20:n;
    for(long k=0;k<keep;k++){ int ch=fgetc(in); if(ch==EOF) break; fputc(ch,out); } fclose(in); fclose(out);
    out=fopen(dst,"r+b"); if(out){ SaveHdr h; if(fread(&h,sizeof h,1,out)==1){ h.ver=12; rewind(out); fwrite(&h,sizeof h,1,out); } fclose(out); }
}
static void setup(void){
    planetSize=128; rs=0xD15A57u; makeSpecies(); makeItems(); newWorld(); gx.wx=0; monOn=0;
    nCr=0; memset(occ,0,sizeof occ); cellClear(); memset(hz,0,sizeof hz);
    for(int y=24;y<104;y++) for(int x=24;x<104;x++){ world[y][x]=T_GRASS; bld[y][x]=0; bldv[y][x]=0; }
}
int main(void){
    setup();
    CHECK(sizeof(Hz)==14,"disasters do not enlarge the saved hazard record");
    CHECK(sizeof(Cr)==(sizeof(coord_t)==1?20:22),"creature record size remains target-compatible");
    CHECK(findPower(PK_ALIEN)>=0&&findPower(PK_DRAGON)>=0&&findPower(PK_MOON)>=0&&findPower(PK_BIOMASS)>=0&&findPower(PK_ICE)>=0&&findPower(PK_GARDEN)>=0,"all six disaster powers are listed");
    CHECK(findPower(PK_SPECIAL)>=0&&findPower(PK_FIREEL)>=0&&PW_N<=(int)sizeof favPow,"special-event power and five cycling fire variants are available without exceeding the 80 favorite slots");

    int h=hzAdd(HZ_ALIEN,40,40); CHECK(h>=0,"create alien invasion hazard"); if(h>=0){ disasterStep(&hz[h]); CHECK(countArch("ALIEN")>=1&&countArch("UFO")>=1,"alien invasion deploys aliens and a UFO"); }
    memset(hz,0,sizeof hz); nCr=0; memset(occ,0,sizeof occ); cellClear();
    h=hzAdd(HZ_DRAGON,48,48); if(h>=0){ disasterStep(&hz[h]); CHECK(countArch("ELDER DRAGON")+countArch("WYVERN")>=1,"far-land dragon enters as a real monster"); }
    memset(hz,0,sizeof hz); nCr=0; memset(occ,0,sizeof occ); cellClear();
    h=hzAdd(HZ_MOON,52,52); if(h>=0){ disasterStep(&hz[h]); CHECK(countArch("WRAITH")>=1&&countArch("LICH")>=1,"moon horrors summon wraiths and a lich"); }
    memset(hz,0,sizeof hz); nCr=0; memset(occ,0,sizeof occ); cellClear();
    h=hzAdd(HZ_BIOMASS,56,56); if(h>=0){ disasterStep(&hz[h]); CHECK(countArch("TUMOR MONSTER")>=1&&countArch("BIOBLOB")>=1,"biomass event spawns its tumor creatures"); }
    memset(hz,0,sizeof hz); nCr=0; memset(occ,0,sizeof occ); cellClear();
    world[64][64]=T_FOREST; h=hzAdd(HZ_ICE,64,64); if(h>=0){ disasterStep(&hz[h]); CHECK(world[64][64]==T_SNOW,"dream ice freezes vegetation into snow"); }
    memset(hz,0,sizeof hz); nCr=0; memset(occ,0,sizeof occ); cellClear();
    { int s=findHuman(); int i=addCr(s,72,72); CHECK(i>=0,"place a garden test creature"); if(i>=0) cr[i].hp=2;
      h=hzAdd(HZ_GARDEN,72,72); if(h>=0){ disasterStep(&hz[h]); CHECK(i>=0&&cr[i].alive&&cr[i].hp>2,"garden surprise heals nearby creatures"); } }

    // The special-event power cycles through the six remaining named creature/world-event families.
    memset(hz,0,sizeof hz); nCr=0; memset(occ,0,sizeof occ); cellClear(); power=findPower(PK_SPECIAL); brush=0;
    for(int k=0;k<6;k++) usePower(80,80,1);
    CHECK(hzCount(HZ_NAVY)==1&&hzCount(HZ_PLANES)==1&&hzCount(HZ_HYPERCUBE)==1&&hzCount(HZ_GHOSTCIV)==1&&hzCount(HZ_CYBERCORE)==1&&hzCount(HZ_PUMPKIN)==1,"SPECIAL EVENT cycles through all six named event types");
    for(int i=0;i<HZ_N;i++) if(hz[i].kind>=HZ_NAVY&&hz[i].kind<=HZ_PUMPKIN) disasterStep(&hz[i]);
    CHECK(countArch("SEAL")>=1,"Navy Seal deployment creates armed seal units");
    CHECK(countArch("EAGLE")>=1,"plane squadron deploys flying units");
    CHECK(countArch("WRAITH")>=1,"ghost civilization deploys wraith units");
    CHECK(countArch("VR HEADSET")>=1&&countArch("GOLEM")>=1,"cybercore deploys machine defenders");
    CHECK(countArch("LIL PUMPKIN")>=1,"super pumpkin deploys pumpkin monsters");
    memset(hz,0,sizeof hz);
    for(int k=0;k<5;k++){ h=hzAdd(HZ_FIRE,80,80); CHECK(h>=0,"create one of five fire-elemental forms"); if(h>=0){ hz[h].pad=(u8)k; fireStep(&hz[h]); CHECK(hz[h].age==1,"fire-elemental variant executes its distinct pulse"); hz[h].kind=HZ_NONE; } }

    // Exercise the real brush power path and save/load of an added hazard kind.
    memset(hz,0,sizeof hz); power=findPower(PK_ICE); brush=0; usePower(80,80,1);
    CHECK(hzCount(HZ_ICE)==1,"DREAM ICE power creates its hazard");
    favPow[64]=1; favPow[66]=1;
    CHECK(saveGame("/tmp/gg_disasters.sav"),"save a new disaster and high-index power favorites");
    memset(hz,0,sizeof hz); memset(favPow,0,sizeof favPow); CHECK(loadGame("/tmp/gg_disasters.sav"),"load save with new disaster kind");
    CHECK(hzCount(HZ_ICE)==1,"new hazard kind survives save/load");
    h=hzAdd(HZ_GHOSTCIV,84,84); CHECK(h>=0,"create a newly added ghost-civilization hazard");
    CHECK(saveGame("/tmp/gg_disasters_special.sav"),"save newly added special-event kind");
    memset(hz,0,sizeof hz); CHECK(loadGame("/tmp/gg_disasters_special.sav"),"load save containing newly added special-event kind");
    CHECK(hzCount(HZ_GHOSTCIV)==1,"new special-event kind is not discarded by the save loader");
    CHECK(favPow[64]&&favPow[66],"favorites for powers 64 and 66 survive v13 save/load");
    saveVersion12WithoutPowerTail("/tmp/gg_disasters.sav","/tmp/gg_disasters_v12.sav");
    memset(favPow,0,sizeof favPow); CHECK(loadGame("/tmp/gg_disasters_v12.sav"),"older v12 save still loads without the GGF1 trailer");
    CHECK(!favPow[64]&&!favPow[66],"new favorite slots default off in older saves");

    printf("%s\n",failures?"DISASTERS FAIL":"DISASTERS OK"); return failures?1:0;
}
