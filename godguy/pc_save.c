// PC test of saving: play 3 years, save, wipe, load, and check that every byte of the world, the creatures, the villages and the story came back -- and that the game then runs on exactly as if it had never been saved.
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static u32 hashAll(void){ u32 h=2166136261u; const u8*p; 
#define H(ptr,n) p=(const u8*)(ptr); for(size_t i=0;i<(size_t)(n);i++) h=(h^p[i])*16777619u;
    for(int y=0;y<gH;y++){ H(world[y],gW); H(bld[y],gW); H(bldv[y],gW); } H(cr,nCr*sizeof(Cr)); H(vil,sizeof vil); H(&nCr,sizeof nCr); H(&year,sizeof year); H(&chapter,sizeof chapter); H(&tierOpen,sizeof tierOpen); H(&stKill,sizeof stKill); H(&rs,sizeof rs); H(seenB,sizeof seenB); H(slainB,sizeof slainB); H(&trSeen,sizeof trSeen); H(&stMut,sizeof stMut); H(&diff,sizeof diff); H(&worldSeed,sizeof worldSeed); H(&weather,sizeof weather); H(&weatherT,sizeof weatherT); return h; }
int main(int argc,char**argv){
    int bad=0; for(int size=128;size<=256;size+=64){
        planetSize=size; rs=99u*2654435761u+size; makeSpecies(); makeItems(); newWorld(); chapter=3; tierOpen=4; stKill=11; monOn=1;
        for(int t=1;t<=3*2048;t++){ tick=t; simulate(); }
        u32 h0=hashAll(); int ok=saveGame("/tmp/godguy_test.sav"); int tk=tick;
        for(int t=tk+1;t<=tk+600;t++){ tick=t; simulate(); } u32 hA=hashAll();     // the run that never stopped
        planetSize=128; rs=5; newWorld(); chapter=0; tierOpen=1; stKill=0; memset(seenB,0,sizeof seenB);        // a different world in memory
        int lo=loadGame("/tmp/godguy_test.sav"); u32 h1=hashAll();
        for(int t=tk+1;t<=tk+600;t++){ tick=t; simulate(); } u32 hB=hashAll();
        printf("size %d: save %d load %d | loaded state equals saved state: %s | 600 ticks later equal to the unsaved run: %s | creatures %d villages %d chapter %d\n",size,ok,lo,h0==h1?"YES":"NO",hA==hB?"YES":"NO",nCr,nVilOn,chapter);
        bad+=!(ok&&lo&&h0==h1&&hA==hB); }
    // a damaged file is refused
    FILE*f=fopen("/tmp/godguy_test.sav","r+b"); fputc(0,f); fclose(f); printf("damaged header refused: %s\n",loadGame("/tmp/godguy_test.sav")?"NO":"YES");
    f=fopen("/tmp/godguy_short.sav","wb"); fwrite("GGS1",1,4,f); fclose(f); printf("truncated file refused: %s\n",loadGame("/tmp/godguy_short.sav")?"NO":"YES");
    return bad; }
