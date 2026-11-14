// Biomes and atmosphere on the real program (through compat/ndsshim.h): painting CORRUPT / INFERNAL / ENCHANT, spreading, the air powers, climate, and a save / load round trip.
// gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o pc_bio pc_bio.c -lm && ./pc_bio
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
static int fails=0; static void chk(int c,const char*m){ printf("%s %s\n",c?"ok  ":"FAIL",m); if(!c) fails++; }
static int findPw(const char*nm){ for(int i=0;i<PW_N;i++) if(!strcmp(pw[i].nm,nm)) return i; return -1; }
void shimInput(int frame,int*held,int*tx,int*ty){
    *held=0; if(frame!=5) return;
    titleOn=0; menuOn=0; extrasView=0; sceneOn=0; dlgOn=0; dlgPend=-1; page=0; paused=0; planetSize=128; rs=4242u*2654435761u+7; newWorld(); monOn=0;
    for(int t=1;t<=3000;t++){ tick=t; year=t/1200; simulate(); }
    chk(nBio[1]+nBio[2]+nBio[3]==0,"a new world has no biomes");
    int x=-1,y=-1; for(int k=0;k<4000&&x<0;k++){ int a=rnd()%WW,b=rnd()%WH; if(world[b][a]==T_GRASS){ x=a; y=b; } } chk(x>=0,"found grass");
    for(int b=1;b<=3;b++){ char* n=b==1?"CORRUPT":b==2?"INFERNAL":"ENCHANT"; int p=findPw(n); chk(p>=0,n); power=p; usePower(x+b*3,y,1); }
    chk(nBio[1]+nBio[2]+nBio[3]>=1,"painting a biome sets tiles");
    int before=nBio[1]+nBio[2]+nBio[3]; for(int t=3001;t<=13000;t++){ tick=t; simulate(); }
    int after=nBio[1]+nBio[2]+nBio[3]; printf("biome tiles %d -> %d\n",before,after); chk(after>before,"biomes spread");
    bioCount(); chk(nBio[1]+nBio[2]+nBio[3]==after,"biome counts are consistent");
    atmoReset(); int o=atO2,c=atCO2; power=findPw("GREENHOUSE"); usePower(x,y,1); chk(atCO2>c,"GREENHOUSE raises CO2"); power=findPw("OXYGEN"); usePower(x,y,1); chk(atO2>o,"OXYGEN raises O2");
    power=findPw("DUST CLOUD"); usePower(x,y,1); chk(atDust>0,"DUST CLOUD adds dust");
    for(int t=13001;t<=16000;t++){ tick=t; simulate(); } chk(ATV(atO2)>0&&ATV(atO2)<=60&&ATV(atCO2)>=0&&ATV(atTemp)>-45&&ATV(atTemp)<45,"the atmosphere stays in range");
    printf("O2 %d CO2 %d dust %d warmth %d\n",ATV(atO2),ATV(atCO2),ATV(atDust),ATV(atTemp));
    static u8 b2[sizeof biome2]; memcpy(b2,biome2,sizeof b2); int sO=atO2,sC=atCO2,sT=atTemp;
    chk(saveGameRaw("/tmp/gg_bio_out.sav"),"saved"); bioClear(); atmoReset(); chk(nBio[1]+nBio[2]+nBio[3]==0&&atO2==21*16,"cleared");
    chk(loadGameRaw("/tmp/gg_bio_out.sav"),"loaded"); int same=1; for(int yy=0;yy<gH;yy++) for(int xx=0;xx<gW;xx++) if(bioAt(xx,yy)!=((b2[(yy*MAXD+xx)>>2]>>((xx&3)*2))&3)) same=0;
    chk(same,"biomes survive a save and load"); chk(atO2==sO&&atCO2==sC&&atTemp==sT,"atmosphere survives a save and load");
    printf(fails?"BIO FAIL\n":"BIO OK\n"); exit(fails?1:0); }
void shimPresent(void){}
int main(int argc,char**argv){ mkdir("/tmp/gg_bio_out",0777); if(chdir("/tmp/gg_bio_out")) return 1; return ds_main(); }
