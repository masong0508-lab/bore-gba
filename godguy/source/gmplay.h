// gmplay.h - the GodGuy music player, second engine: a tracker-style sequencer that runs once a frame (60 Hz). Nine looped single-cycle wave voices (lead, ping-pong echo, counter,
// harmony, two detuned pads, bass, arpeggio, sparkle) with attack/decay/release, vibrato and auto-panning, plus a sampled drum kit on four slots (two for the kick and snare part,
// two for the cymbal part) and a PSG/noise channel pair for the sound effects. The same code runs on the DS (backend: libnds sound channels) and on the PC (backend: a software
// stereo mixer that writes a WAV), so timing, loops, envelopes and panning are tested on the PC.
// Stream = pairs of bytes (note, rows): note 0 = rest, else MIDI 24..83; drum streams hold drum ids (bit 7 = soft).
// Backend, defined before this file is included:
//   gb_vstart(v,wave) / gb_vstop(v)            start or stop looped wave voice v (0..8)
//   gb_vset(v,rate,vol,pan)                    rate = playback rate in Hz (note Hz * 64), vol 0..127, pan 0 (left)..127 (right)
//   gb_hit(slot,part,idx,vol,pan)              play a drum one-shot (part 0 = kit A, 1 = kit B)
//   gb_ptone(midi8,vol,duty) gb_pvol(vol) gb_poff()   the effect tone (PSG, midi8 = MIDI note x 8)      gb_pnoise(freq,vol) gb_pnoff()   the effect noise
#ifndef GMPLAY_H
#define GMPLAY_H
#include "gmkit.h"
#include "gmdata.h"
#define GMV 10                      // voice 9 is a Haas double of the lead (one frame late, a hair sharp, on the other side): it widens the lead
static int gmCur=-1, gmAcc, gmRow, gmMaster=100, gmPaused, gmTick;
static int gmPos[11], gmRem[11], gmNote[11], gmAge[11], gmRel[11], gmAlt[11];
static int gmHz64[128], gmSin[32], gmInit;
static int gmSlotA, gmSlotB;
static int fxCur=-1, fxPos, fxFrames, fxAge, fxNote, fxVol, fxKind, fxSlide, fxPitch8, fxOn, fxNoiseOn;
// envelope per voice: attack frames, decay frames, sustain %, release frames
static const u8 gmEnv[GMV][4]={ {1,10,86,3}, {1,10,80,3}, {1,14,70,2}, {2,10,88,3}, {10,1,100,8}, {10,1,100,8}, {1,6,85,2}, {1,8,45,1}, {1,16,25,1}, {1,10,86,3} };
static int ldRate, ldVol, ldPan;     // the lead's values of the last frame, which the double plays now
static void gmTables(void){
    if(gmInit) return; gmInit=1;
    for(int n=0;n<128;n++){ gmHz64[n]=(int)(440.0*pow(2.0,(n-69)/12.0)*64.0+0.5); }
    for(int i=0;i<32;i++) gmSin[i]=(int)(127.0*sin(i*6.2831853/32.0));
}
static void gmSeek(int row){
    const GmSong*S=&gmSongs[gmCur];
    for(int v=0;v<11;v++){ int pos=0, acc=0; gmNote[v]=0; gmRem[v]=0; gmAge[v]=0; gmRel[v]=-1;
        while(pos<S->n[v]){ int len=S->v[v][pos+1]; if(acc+len>row){ gmNote[v]=S->v[v][pos]; gmRem[v]=acc+len-row; pos+=2; break; } acc+=len; pos+=2; }
        gmPos[v]=pos; }
    gmRow=row;
}
static void gmStop(void){ for(int v=0;v<GMV;v++) gb_vstop(v); gmCur=-1; }
static void gmPlay(int s){
    gmTables(); if(s==gmCur) return; if(gmCur>=0) gmStop(); if(s<0||s>=GM_SONGS) return;
    gmCur=s; gmAcc=0; gmSlotA=gmSlotB=0; gmSeek(0); gmRow=0; ldRate=gmHz64[60]; ldVol=0; ldPan=64;
    for(int v=0;v<GMV;v++){ gb_vstart(v,gmSongs[s].inst[v<9?v:0]); gmAge[v]=0; gmRel[v]=-1; gmAlt[v]=0; }
}
static void gmDrum(int part,int id){
    int soft=(id>>7)&1; id&=127; if(!id) return; int vol, pan, idx, slot;
    if(part==0){ static const u8 dv[8]={0,120,105,95,100,100,100,80}, dp[8]={0,64,64,70,38,62,88,76}; if(id>7) return; idx=id-1; vol=dv[id]; pan=dp[id]; slot=(gmSlotA^=1); }
    else { static const u8 dv[7]={0,60,62,80,66,45,42}; if(id>6) return; idx=id-1; vol=dv[id]; gmAlt[10]^=1;
        pan=id==3?34:id==4?100:id==2?92:(gmAlt[10]?46:82); slot=2+(gmSlotB^=1); }
    if(soft) vol=vol*55/100; vol=vol*gmMaster/100*40/100;
    gb_hit(slot,part,idx,vol,pan);
}
static void gmNoteOn(int v,int n){
    gmNote[v]=n; gmAge[v]=0; gmRel[v]=-1; gmAlt[v]^=1;
}
static void gmVoiceFrame(int v,const GmSong*S){
    if(v==9){ gb_vset(9,ldRate+ldRate/400,ldVol*8/10,127-ldPan); return; }
    int n=gmNote[v]; if(!n){ if(gmRel[v]<0) return; }
    const u8*E=gmEnv[v]; int atk=E[0], dec=E[1], sus=E[2], rel=E[3];
    int w=S->inst[v]; if(w==5||w==6||w==13){ if(v>=2){ dec=16; sus=12; } }      // bell, pluck and glass voices ring out
    int age=gmAge[v], lev;
    if(age<atk) lev=100*(age+1)/(atk+1); else if(age<atk+dec) lev=100-(100-sus)*(age-atk)/(dec?dec:1); else lev=sus;
    if(gmRel[v]>=0){ int r=gmRel[v]; if(r>=rel){ gmRel[v]=-1; gmNote[v]=0; gb_vset(v,gmHz64[60],0,64); if(v==0){ ldVol=0; } return; } lev=lev*(rel-r)/rel; gmRel[v]++; }
    else gmAge[v]++;
    int nn=gmNote[v]; if(!nn) nn=60;
    int rate=gmHz64[nn];
    if(v==4) rate-=rate/285; else if(v==5) rate+=rate/250;
    if(v<=3&&age>12) rate+=(rate>>8)*gmSin[(gmTick>>1)&31]>>7;                                   // vibrato once a note has settled
    if(v==4||v==5) rate+=(rate>>9)*gmSin[(gmTick>>3)&31]*(v==4?1:-1)>>7;                         // slow chorus drift
    int pan=S->pan[v];
    if(v==0) pan+=gmSin[(gmTick>>4)&31]*10>>7; else if(v==4) pan+=gmSin[(gmTick>>5)&31]*8>>7; else if(v==5) pan-=gmSin[(gmTick>>5)&31]*8>>7;
    else if(v==7) pan=gmAlt[v]?26:102; else if(v==8) pan=gmAlt[v]?104:24; else if(v==2) pan=gmAlt[v]?24:104;
    int vol=S->vol[v]*lev/100*gmMaster/100*56/100;
    if(pan<0) pan=0; if(pan>127) pan=127;
    gb_vset(v,rate,vol,pan);
    if(v==0){ ldRate=rate; ldVol=vol; ldPan=pan; }
}
static void gmFrame(void){
    gmTick++;
    if(gmPaused) return;
    if(gmCur>=0){ const GmSong*S=&gmSongs[gmCur];
        gmAcc+=S->bpm*4*65536/3600;
        while(gmAcc>=65536){ gmAcc-=65536;
            if(gmRow>=S->rows){ if(S->loopRow==65535){ gmStop(); break; } gmSeek(S->loopRow); for(int v=0;v<GMV;v++){ gmAge[v]=8; } }
            for(int v=0;v<11;v++){
                if(gmRem[v]==0){ if(gmPos[v]>=S->n[v]) continue; int n=S->v[v][gmPos[v]], len=S->v[v][gmPos[v]+1]; gmPos[v]+=2; gmRem[v]=len;
                    if(v<GMV){ if(n){ if(!(n==gmNote[v]&&(v==4||v==5))) gmNoteOn(v,n); else gmRel[v]=-1; } else if(gmNote[v]&&gmRel[v]<0) gmRel[v]=0; }
                    else gmDrum(v-9,n); }
                if(gmRem[v]>0) gmRem[v]--; }
            gmRow++; }
        if(gmCur>=0){ gmVoiceFrame(9,S); for(int v=0;v<9;v++) gmVoiceFrame(v,S); } }
    // the sound effect channel
    if(fxCur>=0){ const GmFx*F=&gmFx[fxCur];
        if(fxAge==0){ if(fxPos>=F->n){ if(fxOn){ gb_poff(); fxOn=0; } if(fxNoiseOn){ gb_pnoff(); fxNoiseOn=0; } fxCur=-1; return; }
            const u8*e=F->ev+fxPos*5; fxNote=e[0]; fxFrames=e[1]; fxVol=e[2]; fxKind=e[3]; fxSlide=(int)(signed char)e[4]; fxPitch8=fxNote*8; fxPos++;
            if(fxOn){ gb_poff(); fxOn=0; } if(fxNoiseOn){ gb_pnoff(); fxNoiseOn=0; }
            if(fxNote&&fxKind<4){ gb_ptone(fxPitch8,0,fxKind); fxOn=1; } }
        int vol=fxVol*(fxFrames-fxAge)/fxFrames*gmMaster/100*8/10;
        if(fxNote){ if(fxKind<4){ fxPitch8+=fxSlide; gb_ptone(fxPitch8,vol,fxKind); } else { gb_pnoise(1000+fxNote*170,vol); fxNoiseOn=1; } }
        if(++fxAge>=fxFrames) fxAge=0; }
}
static void fxPlay(int i){ if(i<0||i>=GM_FX) return; if(fxOn){ gb_poff(); fxOn=0; } if(fxNoiseOn){ gb_pnoff(); fxNoiseOn=0; } fxCur=i; fxPos=0; fxAge=0; }
#endif
