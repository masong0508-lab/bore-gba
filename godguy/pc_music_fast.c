// Soundtrack scene selection must be identical with the fast cell-mask
// prefilter and the original exhaustive creature scan. Exercises worlds,
// monster presence, camera movement and regular simulation ticks.
// gcc -O2 -DNDSSHIM -Icompat -Isource -include unistd.h pc_music_fast.c -lm \\
//     -o /tmp/gg_music_fast && /tmp/gg_music_fast
#define GG_LEGACY_GLOBE_TEST 1
#define main ds_main
#include "source/main.c"
#undef main
#include <time.h>
static int fails;
void shimInput(int f,int*h,int*tx,int*ty){(void)f;*h=*tx=*ty=0;}
void shimPresent(void){}
static int referenceDirector(void){
    if(paused) return GM_PAUSE;
    if((weather==WX_STORM||weather==WX_RAGE)&&!view3d) return GM_STORM;
    if(view3d) return GM_SPHERE;
    int cx=(camX>>3)+16, cy=(camY>>3)+12;
    for(int i=0;i<nCr;i++){
        const Cr*c=&cr[i];if(!c->alive)continue;
        int a=sp[c->sp].a;
        if(arch[a].diet!=5||c->hp<100)continue;
        int dx=c->x-cx,dy=c->y-cy;if(dx*dx+dy*dy>900)continue;
        if(a==ARCH_A+19)return GM_DRAGON;
        if(a==ARCH_A+17)return GM_DEMON;
        if(a==ARCH_A+18)return GM_LICH;
        if(a==ARCH_A+13||a==ARCH_A+12)return GM_KRAKEN;
        if(a==ARCH_A+16)return GM_DRAGON;
    }
    if(warHeat>=8||(gmCur==GM_WAR&&warHeat>=2))return GM_WAR;
    if(monCount()>0)return GM_MONSTER;
    {int t=world[cy<WH?cy:WH-1][cx<WW?cx:WW-1];
     if(t==T_DESERT)return GM_DESERT;
     if(t==T_SNOW)return GM_SNOW;
     if(t==T_DEEP||t==T_WATER)return GM_OCEAN;
     if(t==T_JUNGLE)return GM_JUNGLE;
     if(t==T_SWAMP)return GM_SWAMP;
     if(t==T_LAVA||t==T_MOUNT)return GM_VOLCANO;}
    {int lv=0;for(int v=0;v<VIL_N;v++)if(vil[v].on&&vil[v].lvl>lv)lv=vil[v].lvl;
     if(lv>=3)return GM_CITY;
     if(lv==2)return GM_MARKET;
     if(lv==1&&nVilOn>0&&(year&1))return GM_VILLAGE;}
    int p=(tick>>1)&2047;
    {int nf=0,op[GM_FAMN];
     for(int k=0;k<GM_FAMN;k++)if(famOpen(k))op[nf++]=k;
     if(nf&&((year+(p>>10))%3==0))return GM_FAM0+op[(year*5+(p>>9))%nf];}
    if(fullDone&&(year&1))return GM_REMIX0+(year*7+(p>>9))%GM_REMIXES;
    if(!storyOn&&(year%5==4))return GM_FREEPLAY;
    return(p<256||p>1792)?GM_NIGHT:p<640?GM_MORNING:p<1408?GM_NOON:GM_SUNSET;
}
int main(void){
    mapPtr=(u16*)bgGetMapPtr(0);
    rs=949343; planetSize=192;makeSpecies();makeItems();newWorld();monOn=1;
    paused=0;view3d=0;weather=WX_CLEAR;
    // Populate enough creatures for scene checks to matter and several
    // elite monster archetypes, with a mix of cities and wilderness.
    for(int i=0;i<4000;i++){
        int x=(int)(rnd()%WW),y=(int)(rnd()%WH);
        int species=(int)(rnd()%nSp);
        if(i%20==0)species=8+(ARCH_A+(i/20)%24-8)*16;
        addCr(species,x,y);
    }
    int checks=0,masks=0,misses=0,selected=0;
    for(int f=0;f<1800;f++){
        tick+=1;
        camX=(int)(hsh(f,1,42)%(WW*8-256));
        camY=(int)(hsh(f,2,42)%(WH*8-192));
        if((f%35)==0){year+=1;simulate();}
        weather=(f%23==0)?WX_STORM:WX_CLEAR;
        paused=(f%47==0); view3d=(f%71==0);
        int a=musDirector(),b=referenceDirector();
        if(a!=b){if(misses++<6)fprintf(stderr,"music mismatch frame %d: fast %d original %d\n",f,a,b);}
        if(musCellMonsterAnywhere()!=(monCount()>0)){if(masks++<6)fprintf(stderr,"monster mask disagrees frame %d\n",f);}
        if(a==GM_DRAGON||a==GM_MONSTER||a==GM_DEMON||a==GM_KRAKEN||a==GM_LICH)selected++;
        checks++;
    }
    printf("music scan: %d compared camera/time states, %d fast/reference differences, %d mask errors, %d monster themes\n",checks,misses,masks,selected);
    fails+=misses+masks;
    puts(fails?"MUSIC SPATIAL FAIL":"MUSIC SPATIAL OK");
    return fails?1:0;
}
