// pc_city.c: a city recovers: after fire and war has left ruins the folk clear them and a fallen hall is rebuilt; SETTLE says why a village cannot be founded.
//   gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o pc_city pc_city.c -lm && ./pc_city
#define main ds_main
#include "source/main.c"
#undef main
void shimInput(int f,int*h,int*tx,int*ty){ *h=0; *tx=0; *ty=0; }
void shimPresent(void){}
static int fails;
#define CHECK(c,...) do{ if(!(c)){ printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); fails++; } }while(0)
static int cv, cx0, cy0; static int count(int type){ int n=0; for(int y=cy0-9;y<=cy0+9;y++) for(int x=cx0-9;x<=cx0+9;x++) if(x>=0&&y>=0&&x<WW&&y<WH) n+=bld[y][x]==type; return n; }      // (round the village only: the world has ruins of its own)
int main(void){
    mapPtr=(u16*)bgGetMapPtr(0); planetSize=192; rs=1234u; makeSpecies(); makeItems(); newWorld(); monOn=0;
    // a grown village
    int v=-1; for(int t=0;t<400&&v<0;t++){ int x=40+(int)(rnd()%100), y=40+(int)(rnd()%100); v=vilFound(1,x,y); } CHECK(v>=0,"no village could be founded");
    cx0=vil[v].x; cy0=vil[v].y; vil[v].stock=400; for(int k=0;k<8;k++) addCr(1,vil[v].x+(int)(rnd()%5)-2,vil[v].y+(int)(rnd()%5)-2);
    for(int t=1;t<=60;t++){ tick=t*16; vil[v].stock=400; vilTick(v); }
    int built=count(B_HUT)+count(B_HOUSE)+count(B_HALL); printf("village grown: %d buildings, level %d\n",built,vil[v].lvl); CHECK(built>=3,"village did not grow");
    // fire: every building falls to ruin
    for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) if(bld[y][x]&&bld[y][x]!=B_RUIN) razeAt(x,y,255);
    int ruins=count(B_RUIN); printf("after the fire: %d ruins, %d halls\n",ruins,count(B_HALL)); CHECK(ruins>0,"nothing fell");
    CHECK(bld[vil[v].y][vil[v].x]==B_RUIN,"the hall did not fall");
    for(int t=1;t<=400;t++){ tick=(60+t)*16; vil[v].stock=400; vilTick(v); }
    printf("village on=%d pop=%d stock=%d houses=%d\n",vil[v].on,vil[v].pop,vil[v].stock,vil[v].houses); printf("after recovery: %d ruins left, hall %s, %d buildings\n",count(B_RUIN),bld[vil[v].y][vil[v].x]==B_HALL?"rebuilt":"NOT rebuilt",count(B_HUT)+count(B_HOUSE)+count(B_HALL));
    CHECK(bld[vil[v].y][vil[v].x]==B_HALL,"the hall was not rebuilt"); CHECK(count(B_RUIN)<ruins/3,"ruins are not being cleared (%d of %d left)",count(B_RUIN),ruins);
    CHECK(count(B_HUT)+count(B_HOUSE)>=2,"the village did not rebuild its homes");
    // the reasons
    { int x=vil[v].x+3, y=vil[v].y; bld[y][x]=B_NONE; CHECK(!strcmp(vilWhyNot(vil[v].x,vil[v].y),"SOMETHING IS BUILT HERE"),"reason: built"); CHECK(!strcmp(vilWhyNot(1,1),"TOO NEAR THE MAP EDGE"),"reason: edge"); }
    printf(fails?"city: FAIL\n":"city: ok\n"); return fails?1:0; }
