/* Host regression: cc -std=c11 -O2 tools/test_jukebox_meter.c -lm -o /tmp/bore-meter && /tmp/bore-meter */
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../source/jukebox_meter.h"
int main(void){
    signed char l[304],r[304],before[304];
    unsigned short bands[8]={0};int lp[5]={0};
    unsigned char eq[14]={0},pk[14]={0},hold[14]={0},beat=0;int prev=0;volatile unsigned char live=0;
    _Alignas(4) unsigned short pixels[240*160];
    memset(l,0,sizeof(l));memset(r,0,sizeof(r));meter_analyze(l,r,lp,bands);
    for(int i=0;i<8;i++)assert(bands[i]==0);
    for(int f=0;f<3;f++){
        double hz=f==0?80:f==1?880:6000;memset(lp,0,sizeof(lp));
        for(int frame=0;frame<20;frame++){
            for(int i=0;i<304;i++)l[i]=r[i]=(signed char)(80*sin(6.28318530718*hz*(frame*304+i)/18157.0));
            memcpy(before,l,sizeof(l));meter_analyze(l,r,lp,bands);assert(!memcmp(before,l,sizeof(l))&&!memcmp(before,r,sizeof(r)));
        }
        int bass=bands[6]+bands[7],treble=bands[0]+bands[1];
        if(f==0)assert(bass>treble);if(f==2)assert(treble>bass);
        for(int k=0;k<8;k++){assert(meter_level(bands[k],k)<=48);assert(meter_level(0,k)==0);}
        for(int i=0;i<240*160;i++)pixels[i]=0x1234;
        for(int n=0;n<30;n++)meter_paint(pixels,bands,&live,eq,pk,hold,&beat,&prev,1);
        for(int i=0;i<240*160;i++){int x=i%240,y=i/240;if(x<8||x>=66||y<28||y>=57)assert(pixels[i]==0x1234);}
        for(int i=0;i<14;i++)assert(eq[i]<=48&&pk[i]<=48);
    }
    /* Center and opposite-polarity stereo must retain the same bands. */
    for(int f=0;f<3;f++){
        unsigned short center[8];int center_lp[5]={0},side_lp[5]={0};
        double hz=f==0?80:f==1?880:6000;
        for(int frame=0;frame<20;frame++){
            for(int i=0;i<304;i++)l[i]=r[i]=(signed char)(80*sin(6.28318530718*hz*(frame*304+i)/18157.0));
            meter_analyze(l,r,center_lp,center);
            for(int i=0;i<304;i++)r[i]=-l[i];
            memcpy(before,r,sizeof(r));meter_analyze(l,r,side_lp,bands);
            assert(!memcmp(before,r,sizeof(r)));
            assert(!memcmp(center,bands,sizeof(bands)) && "stereo phase cancellation");
        }
    }
    meter_paint(pixels,bands,&live,eq,pk,hold,&beat,&prev,0);
    for(int i=0;i<14;i++)assert(eq[i]==0&&pk[i]==0&&hold[i]==0);
    puts("jukebox meter: PCM preservation, frequency response, bounds, stereo and reset PASS");
    return 0;
}
