// test_voxscare.c - PC test of source/voxscare.h: who says which scream, and at what pitch. Uses the real source/voices.h (run from the repo root: sh tools/test_voxscare.sh).
#include <stdio.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
enum { SX_FEMALE, SX_MALE, SX_NB, SX_N };
#include "../source/voices.h"
#include "../source/voxscare.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static int isF(int c){ return c>=V_ghost_scared_female_1&&c<=V_ghost_scared_female_4; }
static int isM(int c){ return c>=V_ghost_scared_male_1&&c<=V_ghost_scared_male_4; }
int main(void){
    printf("== voxscare: the ghost scare in both voices\n");
    CHECK(vsOk(),"the takes do not follow one another, or too many clips for the chain queue");
    int seen[VOICE_N]={0};
    for(int sx=0;sx<SX_N;sx++) for(int voice=0;voice<3;voice++) for(int uid=0;uid<4;uid++){
        int fem=(voice==2)||(voice==0&&(sx==SX_FEMALE||(sx==SX_NB&&(uid&1))));
        int base=voice==2?133:voice==1?100:sx==SX_FEMALE?133:sx==SX_MALE?100:116;
        for(unsigned r=0;r<256;r++) for(int age=0;age<5;age++){
            static const int ag[5]={30,16,5,0,-6}; int p=base*(100+ag[age])/100+(int)(r%7)-3;   // what voxPitch makes of it
            int clip=-1,pct=-1; vsPick(sx,voice,uid,r,p,&clip,&pct);
            CHECK(fem?isF(clip):isM(clip),"sx %d voice %d uid %d: %s take expected, got clip %d",sx,voice,uid,fem?"female":"male",clip);
            CHECK(clip==(fem?V_ghost_scared_female_1:V_ghost_scared_male_1)+(int)(r%4),"take does not follow r");
            int want=p*100/base; CHECK(pct==want,"pitch %d, wanted %d",pct,want);
            CHECK(pct>=(100*(100+ag[age])/100)-4&&pct<=(100*(100+ag[age])/100)+4,"sx %d voice %d age %d: pitch %d: the speed-up is not out of the female take",sx,voice,age,pct);
            if(clip>=0&&clip<VOICE_N) seen[clip]=1; }
    }
    int all=1; for(int c=V_ghost_scared_female_1;c<=V_ghost_scared_female_4;c++) all&=seen[c]; for(int c=V_ghost_scared_male_1;c<=V_ghost_scared_male_4;c++) all&=seen[c];
    CHECK(all,"some of the eight scream takes can never be chosen");
    // the cases the player will meet
    { int c,p; vsPick(SX_FEMALE,0,0,0,133,&c,&p); CHECK(isF(c)&&p==100,"a female Sim: her take at 100 %% (got %d)",p);
      vsPick(SX_MALE,0,0,0,100,&c,&p); CHECK(isM(c)&&p==100,"a male Sim: his take at 100 %%");
      vsPick(SX_MALE,2,0,0,133,&c,&p); CHECK(isF(c)&&p==100,"VOICE: FEMALE on a male body says the female take");
      vsPick(SX_FEMALE,1,0,0,100,&c,&p); CHECK(isM(c)&&p==100,"VOICE: MALE on a female body says the male take");
      vsPick(SX_NB,0,2,0,116,&c,&p); CHECK(isM(c)&&p==100,"non-binary, even uid: male take"); vsPick(SX_NB,0,3,0,116,&c,&p); CHECK(isF(c)&&p==100,"non-binary, odd uid: female take"); }
    printf("  54 clips in the list (%d), 8 scream takes, every Sim / voice setting / age gets a take of its own voice at its own pitch\n",VOICE_N);
    printf("%s: %d failure(s)\n",fails?"FAILED":"ALL PASSED",fails); return fails?1:0;
}
