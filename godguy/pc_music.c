// PC test of the GodGuy music player: renders a track (or a sound effect) to a stereo WAV with a software copy of the DS sound hardware (looped 8-bit waves and drum samples with
// linear pan, nearest-neighbour sampling at 32768 Hz, PSG square and noise for the effects), and reports clipping, loop timing and the stereo image.
//   gcc -O2 -DPCTEST -o pc_music pc_music.c -lm && ./pc_music <track | fxN> out.wav [seconds]        ./pc_music list
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16;
#define SR 32768
static struct { int on, wave; double rate, pos; int vol, pan; } tv[10];
static struct { int on, part, idx; double pos; int vol, pan; } hit[4];
static struct { int on; double midi8; int vol, duty; double ph; } ps; static struct { int on; double freq; int vol; double ph; unsigned lfsr; float cur; } pn;
static void gb_vstart(int v,int wave){ tv[v].on=1; tv[v].wave=wave; tv[v].vol=0; tv[v].pan=64; tv[v].rate=440*64; tv[v].pos=0; }
static void gb_vstop(int v){ tv[v].on=0; }
static void gb_vset(int v,int rate,int vol,int pan){ tv[v].rate=rate; tv[v].vol=vol; tv[v].pan=pan; }
static void gb_hit(int slot,int part,int idx,int vol,int pan){ hit[slot].on=1; hit[slot].part=part; hit[slot].idx=idx; hit[slot].pos=0; hit[slot].vol=vol; hit[slot].pan=pan; }
static void gb_ptone(int midi8,int vol,int duty){ ps.on=1; ps.midi8=midi8; ps.vol=vol; ps.duty=duty; }
static void gb_pvol(int vol){ ps.vol=vol; }
static void gb_poff(void){ ps.on=0; }
static void gb_pnoise(int freq,int vol){ pn.on=1; pn.freq=freq; pn.vol=vol; if(!pn.lfsr) pn.lfsr=0x7FFF; }
static void gb_pnoff(void){ pn.on=0; }
#include "source/gmplay.h"
static const float dutyF[4]={0.125f,0.25f,0.5f,0.75f};
int main(int argc,char**argv){
    if(argc>1&&!strcmp(argv[1],"list")){ for(int i=0;i<GM_SONGS;i++) printf("%2d %s\n",i,gmSongs[i].name); return 0; }
    if(argc<3){ fprintf(stderr,"usage\n"); return 1; }
    int isfx=!strncmp(argv[1],"fx",2); int id=atoi(argv[1]+(isfx?2:0)); double secs=argc>3?atof(argv[3]):30;
    gmTables(); if(isfx) fxPlay(id); else { gmPlay(id); if(argc>4){ gmSeek(atoi(argv[4])*16); } }
    int frames=(int)(secs*60); long ns=0; float*buf=malloc(sizeof(float)*2*(size_t)(SR*(secs+2))); double carry=0; long clip=0; double peak=0; int loops=0, lastRow=0; double eL=0,eR=0,eM=0,eS=0;
    for(int f=0;f<frames;f++){
        gmFrame(); if(gmCur>=0){ if(gmRow<lastRow) loops++; lastRow=gmRow; }
        double want=SR/60.0+carry; int n=(int)want; carry=want-n;
        for(int i=0;i<n;i++){ float L=0,R=0;
            for(int v=0;v<10;v++) if(tv[v].on&&tv[v].vol>0){ tv[v].pos+=tv[v].rate/SR; float s=gmWaveT[tv[v].wave][((int)tv[v].pos)&63]/127.f*tv[v].vol/127.f; L+=s*(128-tv[v].pan)/128.f; R+=s*tv[v].pan/128.f; }
            for(int h=0;h<4;h++) if(hit[h].on){ const GmDrum*D=hit[h].part?&gmKitB[hit[h].idx]:&gmKitA[hit[h].idx]; hit[h].pos+=(double)D->rate/SR; int p=(int)hit[h].pos; if(p>=D->n){ hit[h].on=0; continue; }
                float s=D->d[p]/127.f*hit[h].vol/127.f; L+=s*(128-hit[h].pan)/128.f; R+=s*hit[h].pan/128.f; }
            if(ps.on&&ps.vol>0){ double hz=440.0*pow(2.0,(ps.midi8/8.0-69.0)/12.0); ps.ph+=hz/SR; ps.ph-=floor(ps.ph); float s=(ps.ph<dutyF[ps.duty&3]?1.f:-1.f)*ps.vol/127.f; L+=s*0.5f; R+=s*0.5f; }
            if(pn.on){ pn.ph+=pn.freq/SR; while(pn.ph>=1){ pn.ph-=1; unsigned x=((pn.lfsr^(pn.lfsr>>1))&1); pn.lfsr=(pn.lfsr>>1)|(x<<14); pn.cur=(pn.lfsr&1)?1.f:-1.f; } float s=pn.cur*pn.vol/127.f; L+=s*0.5f; R+=s*0.5f; }
            if(fabs(L)>peak) peak=fabs(L); if(fabs(R)>peak) peak=fabs(R);
            if(fabs(L)>1.f){ clip++; L=L>0?1.f:-1.f; } if(fabs(R)>1.f){ clip++; R=R>0?1.f:-1.f; }
            eL+=L*L; eR+=R*R; float m=(L+R)*.5f, sd=(L-R)*.5f; eM+=m*m; eS+=sd*sd; buf[ns*2]=L; buf[ns*2+1]=R; ns++; } }
    FILE*w=fopen(argv[2],"wb"); int dl=(int)(ns*4); unsigned hdr[11]={0x46464952,36+dl,0x45564157,0x20746d66,16,0x00020001,SR,SR*4,0x00100004,0x61746164,dl}; fwrite(hdr,4,11,w);
    for(long i=0;i<ns*2;i++){ short s=(short)(buf[i]*30000); fwrite(&s,2,1,w); } fclose(w);
    double bal=(eL+eR)>0?(eL-eR)/(eL+eR):0, wid=eM>0?eS/eM:0;
    if(!isfx){ const GmSong*S=&gmSongs[id]; printf("%-28s %3d bpm %3d bars = %5.1fs loop@%-5d loops %d  peak %.2f clip %ld  L/R balance %+.2f  side/mid %.2f\n",S->name,S->bpm,S->rows/16,S->rows*15.0/S->bpm,S->loopRow,loops,peak,clip,bal,wid); }
    else printf("fx %-16s [%s] %d events, peak %.2f, clipped %ld\n",gmFx[id].name,gmFx[id].src,gmFx[id].n,peak,clip);
    return 0; }
