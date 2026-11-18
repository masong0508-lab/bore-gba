// Climate pack regression: special weather states run and persist without changing creature-record layouts.
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int fails;
#define CHECK(c,m) do{ if(!(c)){ printf("FAIL: %s\n",m); fails++; } }while(0)
int main(void){
    planetSize=128; rs=0xC11A7Eu; makeSpecies(); makeItems(); newWorld(); gx.wx=0; monOn=0;
    CHECK(!strcmp(weatherName(WX_ASH),"ASH CLOUD"),"ash cloud label");
    CHECK(!strcmp(weatherName(WX_MAGIC),"MAGIC CLOUD"),"magic cloud label");
    CHECK(!strcmp(weatherName(WX_RAGE),"RAGE CLOUD"),"rage cloud label");
    CHECK(!strcmp(weatherName(WX_HEAT),"HEATWAVE"),"heatwave label");
    int oldDust=atDust;
    for(int w=WX_ASH;w<=WX_HEAT;w++){
        weather=w; weatherT=1000;
        for(int k=0;k<256;k++){ tick++; weatherTick(); }
        CHECK(weather==w,"special weather remains active until its timer expires");
    }
    CHECK(atDust>oldDust,"ash and heat weather contribute atmospheric dust");
    weather=WX_HEAT; weatherT=1777;
    CHECK(saveGame("/tmp/gg_weather.sav"),"save special weather");
    weather=WX_CLEAR; weatherT=1;
    CHECK(loadGame("/tmp/gg_weather.sav"),"load special weather");
    CHECK(weather==WX_HEAT&&weatherT==1777,"special weather and timer survive save/load");
    printf(fails?"WEATHER FAIL\n":"WEATHER OK\n"); return fails?1:0;
}
