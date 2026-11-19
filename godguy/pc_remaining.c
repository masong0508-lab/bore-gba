// Remaining GODGUY mechanics: home recall, culture skins, preferred species colours, border brush and seeded culture traits.
#define PCTEST 1
#include <stdlib.h>
#include <string.h>
#include "source/main.c"
static int failures;
#define CHECK(c,m) do{ if(!(c)){ printf("FAIL: %s\n",m); failures++; } }while(0)
static int findPower(int kind){ for(int i=0;i<PW_N;i++) if(pw[i].kind==kind) return i; return -1; }
static int bitCount(unsigned v){ int n=0; while(v){ n+=(int)(v&1); v>>=1; } return n; }
static int borderCount(int v){ int n=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) if(bld[y][x]==B_BORDER&&bldv[y][x]==v+1) n++; return n; }
int main(void){
    planetSize=128; rs=0xC0A57u; makeSpecies(); makeItems(); newWorld(); gx.wx=0; monOn=0;
    CHECK(findPower(PK_HOME)>=0&&findPower(PK_BORDER)>=0,"home recall and border brush powers are in the catalogue");
    CHECK(PW_N<=80,"all powers fit the saved favourite capacity");
    CHECK(preferredFamily(8)>=0&&preferredFamily(8)<16&&preferredFamily(30)>=0&&preferredFamily(30)<16,"species preferred colours are valid palette families");
    CHECK(randomFamily(8)==preferredFamily(8)||randomFamily(8)>=0,"preferred colour selector returns a family");

    // A newly founded kingdom gets a culture identity and at least one non-xenophobic trait.
    nCr=0; memset(occ,0,sizeof occ); cellClear(); memset(vil,0,sizeof vil); memset(kd,0,sizeof kd); memset(kx,0,sizeof kx); memset(vilK,0,sizeof vilK); memset(vilL,0,sizeof vilL); nVilOn=0;
    for(int y=20;y<110;y++) for(int x=20;x<110;x++){ world[y][x]=T_GRASS; bld[y][x]=0; bldv[y][x]=0; occ[y][x]=0; }
    int v=vilFound(0,40,40); CHECK(v>=0,"found test city");
    if(v>=0){ int k=kdFound(v); CHECK(k>=0,"found test kingdom"); if(k>=0){ CHECK(kd[k].cul<5&&kd[k].rlg<4&&kd[k].lang<5&&kd[k].clan<8,"new culture, faith, language and clan are valid"); CHECK((kd[k].ctr&~63)==0,"culture trait flags fit their defined range");
            int fam[5]; for(int c=0;c<5;c++){ kd[k].cul=(u8)c; fam[c]=creatureFamily(&(Cr){.sp=0,.x=40,.y=40}); CHECK(fam[c]==((12+3*c)&15),"people receive the palette for their kingdom culture"); }
            for(int a=0;a<5;a++) for(int b=a+1;b<5;b++) CHECK(fam[a]!=fam[b],"five culture skins have distinct palette variants");
            int ci=addCr(0,80,80); CHECK(ci>=0,"place a citizen away from the city"); if(ci>=0){ CHECK(creatureFamily(&cr[ci])==creatureFamily(&cr[ci]),"culture skin selection is stable"); CHECK(sendCreatureHome(ci),"SEND HOME finds a free tile in the home city"); CHECK(abs((int)cr[ci].x-40)<=4&&abs((int)cr[ci].y-40)<=4,"citizen arrives beside the home city"); }
            int n=paintCityBorder(40,40); CHECK(n>0,"border brush paints a city boundary"); CHECK(borderCount(v)>0,"border tiles are owned by their city"); CHECK(paintCityBorder(40,40)==-1,"border brush toggles the boundary off"); CHECK(borderCount(v)==0,"cleared boundary leaves no border tiles");
            lw[LW_NOOSPHERE]=1; int v3=vilFound(0,100,40); if(v3>=0){ int k3=kdFound(v3); CHECK(k3>=0,"found a Glitched Noosphere kingdom"); if(k3>=0){ int culture=bitCount(kd[k3].ctr)+bitCount(kx[k3].ctr2&31); CHECK(culture>=3&&culture<=6,"Glitched Noosphere gives cultures 3-6 random traits"); CHECK(bitCount((kx[k3].ctr2>>5)&7)==3,"Glitched Noosphere gives languages three traits"); CHECK(bitCount((kx[k3].ctr2>>8)&7)==3,"Glitched Noosphere gives clans three traits"); CHECK(bitCount((kx[k3].ctr2>>11)&7)==3,"Glitched Noosphere gives religions three traits"); } } lw[LW_NOOSPHERE]=0;
            int v2=vilFound(0,80,40); CHECK(v2>=0,"found a second city of the same people"); if(v2>=0){ int k2=kdFound(v2); int a=addCr(0,42,40), b=addCr(0,78,40); CHECK(a>=0&&b>=0,"place citizens in both same-race kingdoms"); if(a>=0&&b>=0){ CHECK(!peopleKingdomsAtWar(a,b),"same-race citizens are loyal while their kingdoms are at peace"); kdWar(k,k2,1); CHECK(peopleKingdomsAtWar(a,b),"same-race citizens recognise their own kingdoms' war"); kdWar(k,k2,0); CHECK(!peopleKingdomsAtWar(a,b),"same-race citizens stop treating each other as enemies after peace"); } }
        } }
    // The real power dispatcher must perform both actions, not just expose menu entries.
    if(v>=0&&kd[0].on){ int ci=addCr(0,90,90); CHECK(ci>=0,"place a second citizen for power test"); if(ci>=0){ power=findPower(PK_HOME); usePower(90,90,1); { int hv=vilNear(0,90,90,WW>WH?WW:WH); CHECK(hv>=0&&abs((int)cr[ci].x-(int)vil[hv].x)<=4&&abs((int)cr[ci].y-(int)vil[hv].y)<=4,"SEND HOME works through usePower"); } }
        power=findPower(PK_BORDER); usePower(40,40,1); CHECK(borderCount(v)>0,"BORDER BRUSH works through usePower"); }
    // Drop of Thoughts: with the law enabled, a life-cloud can place a biome-appropriate sapient.
    nCr=0; memset(occ,0,sizeof occ); cellClear(); for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) world[y][x]=T_GRASS; lw[LW_THOUGHTS]=1; tick=77; int thoughtSpawn=0;
    for(u32 seed=1;seed<500&&!thoughtSpawn;seed++){ rs=seed; thoughtCloudTick(); thoughtSpawn=nCr>0; } CHECK(thoughtSpawn,"Drop of Thoughts spawns a sapient from a life cloud"); lw[LW_THOUGHTS]=0;
    // Rat King: a >20-mouse swarm has a 14% chance to infect nearby citizens.
    nCr=0; memset(occ,0,sizeof occ); cellClear(); for(int y=40;y<90;y++) for(int x=40;x<90;x++) world[y][x]=T_GRASS; int mouseA=-1; for(int a=8;a<ARCH_N;a++) if(!strcmp(arch[a].nm,"MOUSE")){mouseA=a;break;} int ratOk=0; if(mouseA>=0){ for(int q=0;q<25;q++) addCr(8+(mouseA-8)*16+(q&15),60+(q%5)-2,60+(q/5)-2); int h=addCr(0,64,64); if(h>=0){ lw[LW_RATKING]=1; for(u32 seed=1;seed<500&&!ratOk;seed++){ for(int i=0;i<nCr;i++) SET_DISEASE(&cr[i],0); rs=seed; ratOk=ratKingAt(60,60)>0; } CHECK(ratOk,"Rat King infects nearby citizens from a 25-mouse swarm"); lw[LW_RATKING]=0; } }
    // Rat carriers can also move infection between separated neighbours.
    nCr=0; memset(occ,0,sizeof occ); cellClear(); for(int y=40;y<90;y++) for(int x=40;x<90;x++) world[y][x]=T_GRASS; int mouse=mouseA>=0?8+(mouseA-8)*16:0; int mi=addCr(mouse,60,60), hi=addCr(0,63,60); int carrierSpread=0; if(mi>=0&&hi>=0){ SET_DISEASE(&cr[mi],1); gx.lPlague=0; for(u32 seed=1;seed<200&&!carrierSpread;seed++){ cr[mi].alive=1; cr[mi].hp=(u8)maxHp(&cr[mi]); cr[hi].alive=1; cr[hi].hp=(u8)maxHp(&cr[hi]); SET_DISEASE(&cr[mi],1); SET_DISEASE(&cr[hi],0); rs=seed; tick=11; plagueTick(); carrierSpread=DISEASE(&cr[hi])!=0; } CHECK(carrierSpread,"infected mice carry plague across a three-tile gap"); gx.lPlague=0; }

    // The remaining divine powers are real actions, not catalogue-only placeholders.
    nCr=0; memset(occ,0,sizeof occ); cellClear(); for(int y=40;y<90;y++) for(int x=40;x<90;x++){ world[y][x]=T_GRASS; bld[y][x]=0; bldv[y][x]=0; }
    { int ci=addCr(0,60,60); CHECK(ci>=0,"place a possession target"); if(ci>=0){ power=findPower(PK_POSSESS); usePower(60,60,1); CHECK(possessed==ci,"POSSESS attaches to the tapped creature"); usePower(60,60,1); CHECK(possessed<0,"POSSESS toggles back to normal control"); } }
    printerCopied=0; world[60][70]=T_FOREST; power=findPower(PK_PRINT_COPY); usePower(70,60,1); CHECK(printerCopied&&printerTile==T_FOREST,"PRINTER COPY captures the selected tile"); world[80][80]=T_DIRT; usePower(80,80,1); CHECK(world[80][80]==T_FOREST,"PRINTER COPY paints the captured tile");
    power=findPower(PK_PRINT_FOREST); usePower(50,50,1); CHECK(world[50][50]==T_FOREST,"PRINTER FOREST paints forest"); power=findPower(PK_PRINT_WATER); usePower(55,55,1); CHECK(world[55][55]==T_WATER,"PRINTER WATER paints water");
    { int a=addCr(0,65,65), b=addCr(0,66,65); if(a>=0&&b>=0){ int hp=cr[a].hp; power=findPower(PK_BOLT_SMALL); usePower(65,65,1); CHECK(cr[a].alive&&cr[a].hp<hp,"SMALL LIGHTNING damages without guaranteed instant death"); power=findPower(PK_BOLT_MEDIUM); usePower(65,65,1); CHECK(!cr[a].alive||!cr[b].alive,"MEDIUM LIGHTNING strikes its small area"); } }

    // Bee colonies elect one queen, workers follow her, and the queen lays a new worker.
    nCr=0; memset(occ,0,sizeof occ); cellClear(); for(int y=40;y<90;y++) for(int x=40;x<90;x++) world[y][x]=T_GRASS;
    { int beeSp=8+(42-8)*16+3; int q=addCr(beeSp,60,60), w=addCr(beeSp,68,60); CHECK(q>=0&&w>=0,"place two bees for a colony"); if(q>=0&&w>=0){ tick=64; beeMatriarchy(q,&cr[q]); beeMatriarchy(w,&cr[w]); CHECK((crFl[q]&FL_BEEQUEEN)!=0&&(crFl[w]&FL_BEEQUEEN)==0,"a bee colony has one queen and worker bees"); int before=nCr; tick=256; beeMatriarchy(q,&cr[q]); CHECK(nCr>before,"the bee queen lays a worker bee"); } }
    // Autosave is independently switchable; manual save is unaffected by this gate.
    { int oldSd=sdOk, oldYear=savedYear, oldY=year, oldTick=tick, oldSpeed=speedMul; sdOk=1; year=9; savedYear=8; tick=257; speedMul=1; autoSaveOn=1; CHECK(autoSaveDue(),"autosave is due at a new year when enabled"); autoSaveOn=0; CHECK(!autoSaveDue(),"autosave toggle suppresses automatic writes"); sdOk=0; autoSaveOn=1; CHECK(!autoSaveDue(),"autosave does not attempt writes without a card"); sdOk=oldSd; savedYear=oldYear; year=oldY; tick=oldTick; speedMul=oldSpeed; autoSaveOn=1; }
    // Save v14 carries the four rolling captain-rout zones and the monotonic ring index.
    memset(rz,0,sizeof rz); rzN=7; rz[3].x=40; rz[3].y=40; rz[3].a=2; rz[3].until=tick+500;
    CHECK(saveGame("/tmp/gg_remaining.sav"),"save captain-rout state"); memset(rz,0,sizeof rz); rzN=0;
    CHECK(loadGame("/tmp/gg_remaining.sav"),"load captain-rout state"); CHECK(rzN==7&&rz[3].x==40&&rz[3].a==2&&rz[3].until>tick,"save/load preserves the four-slot rout ring and its index");

    printf("%s\n",failures?"REMAINING FAIL":"REMAINING OK"); return failures?1:0;
}
