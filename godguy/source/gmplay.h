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
#include "gmvox.h"        // the vocal chop kit: drum-B stream ids GM_VOX0 and up play a chop instead of a cymbal
#include "gmdata.h"
#define GMV 10                      // voice 9 is a Haas double of the lead (one frame late, a hair sharp, on the other side): it widens the lead
static int gmCur=-1, gmAcc, gmRow, gmMaster=100, gmPaused, gmTick;
static int gmCrowd, gmCrowdS, gmSwF, gmSwN, gmSwUp;   // the size of the battle (0..400, set by the game) and how far the war music has opened up to it; the riser or fall of the swoop now sounding
static int gmInT, gmInN;                       // a song that follows another one fades in: frames left, frames in all
static int (*gmNextHook)(int);                 // asked when a song reaches its end: which song follows it (-1 = none: the music fades to silence and stops). Set by the game; without one every song just ends
static int gmPos[11], gmRem[11], gmNote[11], gmAge[11], gmRel[11], gmAlt[11];
static int gmHz64[128], gmSin[32], gmInit;
static int gmSlotA, gmSlotB;
static unsigned char gmBuf[GM_BUFMAX]; static const unsigned char*gmStr[11];   // the song's streams, unpacked when it starts (the ROM keeps them compressed)
static void gmUnpack(const unsigned char*s,unsigned char*d,int n){   // flag byte (1 = literal), or a 2-byte match: 12 bits back, 4 bits length-3 (15 = 18 + one more byte)
    int o=0; while(o<n){ int fl=*s++|0x100; while(fl>1&&o<n){ if(fl&1) d[o++]=*s++; else { int a=s[0], b=s[1]; s+=2; int off=((a<<4)|(b>>4))+1, len=(b&15)+3; if(len==18) len+=*s++; while(len--&&o<n){ d[o]=d[o-off]; o++; } } fl>>=1; } } }
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
        while(pos<S->n[v]){ int len=gmStr[v][pos+1]; if(acc+len>row){ gmNote[v]=gmStr[v][pos]; gmRem[v]=acc+len-row; pos+=2; break; } acc+=len; pos+=2; }
        gmPos[v]=pos; }
    gmRow=row;
}
static void gmStop(void){ for(int v=0;v<GMV;v++) gb_vstop(v); gmCur=-1; }
static void gmPlay(int s){
    gmTables(); if(s==gmCur) return; if(gmCur>=0) gmStop(); if(s<0||s>=GM_SONGS) return;
    gmInT=0; gmCur=s; { int o=0; for(int v=0;v<11;v++){ gmStr[v]=gmBuf+o; gmUnpack(gmSongs[s].v[v],gmBuf+o,gmSongs[s].n[v]); o+=gmSongs[s].n[v]; } } gmAcc=0; gmSlotA=gmSlotB=0; gmSeek(0); gmRow=0; ldRate=gmHz64[60]; ldVol=0; ldPan=64;
    for(int v=0;v<GMV;v++){ gb_vstart(v,gmSongs[s].inst[v<9?v:0]); gmAge[v]=0; gmRel[v]=-1; gmAlt[v]=0; }
}
// THE END OF A SONG. A song plays through once (from row 0 to its last row) and then it is over: it fades out over its last seconds and the next song takes over
// (gmNextHook), or the music stops. (The old player wrapped to the song's loop row and played the same song for ever.) A long suite brings its own closing fade (S->fade is the
// row it starts at); every other song gets one of 4 seconds. A jingle (loopRow 65535) is short and just ends.
static int gmFadeStart(const GmSong*S){ if(S->fade) return S->fade; if(S->loopRow==65535) return 0; int f=S->rows-S->bpm*4/15; return f<S->rows/2?S->rows/2:f; }
static int gmFadeMul(void){ const GmSong*S=&gmSongs[gmCur]; int fs=gmFadeStart(S), m=256;   // the closing fade, then the fade-in of a song that follows another
    if(fs&&gmRow>=fs){ int r=S->rows-fs; if(r<=0||gmRow>=S->rows) return 0; m=256*(S->rows-gmRow)/r; m=m*m>>8; }
    if(gmInT>0){ int a=256-256*gmInT/gmInN; m=m*(a*a>>8)>>8; }
    return m; }
static void gmDrum(int part,int id){
    int soft=(id>>7)&1; id&=127; if(!id) return; int vol, pan, idx, slot; if(gmCur==GM_WAR&&part==1&&gmCrowdS<120) return; int wm=gmCur==GM_WAR?(gmCrowdS>300?78:100):100;      // (a small fight keeps to the war drums alone: the cymbals and chops join as the crowd grows)
    if(part==0){ static const u8 dv[8]={0,120,105,95,100,100,100,80}, dp[8]={0,64,64,70,38,62,88,76}; if(id>7) return; idx=id-1; vol=dv[id]; pan=dp[id]; slot=(gmSlotA^=1); }
    else if(id>=GM_VOXE0){ int e=id-GM_VOXE0; idx=GM_VOX_LYR0+(e>>1); if(idx>=GM_VOX_N) return; vol=gmVoxVol[idx]*62/100; pan=(e&1)?100:28; slot=2+(gmSlotB^=1); part=2; }      // the stereo echo copy of a lyric phrase: the same voice a moment later, quieter, far to one side
    else if(id>=GM_VOX0){ idx=id-GM_VOX0; if(idx>=GM_VOX_N) return; gmAlt[10]^=1; vol=gmVoxVol[idx]; pan=gmVoxPan[idx]; slot=2+(gmSlotB^=1); part=2; }      // a vocal chop: it takes a cymbal channel for as long as it sounds
    else { static const u8 dv[7]={0,60,62,80,66,45,42}; if(id>6) return; idx=id-1; vol=dv[id]; gmAlt[10]^=1;
        pan=id==3?34:id==4?100:id==2?92:(gmAlt[10]?46:82); slot=2+(gmSlotB^=1); }
    if(soft) vol=vol*55/100; vol=vol*wm/100*gmMaster/100*40/100; vol=vol*gmFadeMul()>>8;
    gb_hit(slot,part,idx,vol,pan);
}
static void gmNoteOn(int v,int n){
    gmNote[v]=n; gmAge[v]=0; gmRel[v]=-1; gmAlt[v]^=1;
}
static int gmRamp(int c,int a,int b){ return c<=a?0:c>=b?100:(c-a)*100/(b-a); }
static int gmWarMix(int v){      // the war drums grow with the crowd: bass and pads first, then the lead, the counter-line, the harmony, the arpeggio and the glitter on top (percent of the voice)
    int c=gmCrowdS; switch(v){ case 6: return 100; case 4: case 5: return 45+c*14/100; case 0: return gmRamp(c,30,130); case 1: return gmRamp(c,150,250); case 2: return gmRamp(c,100,200); case 3: return gmRamp(c,180,280); case 7: return gmRamp(c,230,330); case 8: return gmRamp(c,300,400); } return 100; }
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
    int vol=S->vol[v]*lev/100*gmMaster/100*56/100; vol=vol*gmFadeMul()>>8; if(gmCur==GM_WAR){ vol=vol*gmWarMix(v)/100; vol=vol*(92+gmCrowdS/40)/100; }
    if(pan<0) pan=0; if(pan>127) pan=127;
    gb_vset(v,rate,vol,pan);
    if(v==0){ ldRate=rate; ldVol=vol; ldPan=pan; }
}
// ORCHESTRA AND SWOOPS for the battle music, put on top at run time by the size of the crowd: timpani on the beats, crashes at the bar lines, tom fills before them, and sweeps of noise and pitch
// (a riser into a big downbeat, a fall away after it) on the effect voices. Only when no sound effect is using them.
static void gmSwStart(int frames,int up){ if(fxCur>=0) return; gmSwF=gmSwN=frames; gmSwUp=up; }
static void gmSwoop(void){
    if(gmSwF<=0) return; if(fxCur>=0||gmCur!=GM_WAR){ gmSwF=0; gb_pnoff(); gb_poff(); return; }
    int t=gmSwN-gmSwF, p=gmSwUp?t*100/gmSwN:100-t*100/gmSwN;      // 0..100 along the sweep, rising or falling
    int pk=gmSwUp?p:100-t*100/gmSwN, amp=gmSwUp?(p*p/100):(100-p);   // a riser swells to its end; a fall starts loud and dies away
    gb_pnoise(1000+(8+p*40/100)*170,amp*(gmCrowdS>300?46:34)/100*gmMaster/100);
    gb_ptone((36+p*48/100)*8,amp*(gmCrowdS>300?34:24)/100*gmMaster/100,2); (void)pk;
    if(--gmSwF<=0){ gb_pnoff(); gb_poff(); } }
static void gmWarExtras(void){
    int c=gmCrowdS, r=gmRow&63, b=r&15; if(c<100) return;
    if(r==0&&c>=200){ gmDrum(1,3); gmDrum(0,4); if(c>=300) gmSwStart(70,0); }                                  // a crash and a drum under it on the big downbeat, and the fall away
    else if(b==0||b==8) gmDrum(0,4);                                                                           // timpani on beats 1 and 3
    if(c>=200&&r>=60) gmDrum(0,4+(r-60));                                                                      // a tom fill going up into the next bar of four
    if(c>=200&&r==48) gmSwStart(118,1);                                                                        // the riser through the last bar of four
    if(c>=300&&r==16) gmSwStart(60,1); if(c>=300&&r==32) gmDrum(1,3);
}
static void gmFrame(void){
    gmTick++;
    if(gmPaused) return;
    if(gmCur>=0){ const GmSong*S=&gmSongs[gmCur];
        gmAcc+=S->bpm*4*65536/3600;
        while(gmAcc>=65536){ gmAcc-=65536;
            if(gmRow>=S->rows){ int nx=(S->loopRow!=65535&&gmNextHook)?gmNextHook(gmCur):-1; gmStop();   // the song is over (it has faded out): the next one follows, or silence
                if(nx>=0&&nx<GM_SONGS){ gmPlay(nx); gmInN=gmInT=45; } break; }
            for(int v=0;v<11;v++){
                if(gmRem[v]==0){ if(gmPos[v]>=S->n[v]) continue; int n=gmStr[v][gmPos[v]], len=gmStr[v][gmPos[v]+1]; gmPos[v]+=2; gmRem[v]=len;
                    if(v<GMV){ if(n){ if(!(n==gmNote[v]&&(v==4||v==5))) gmNoteOn(v,n); else gmRel[v]=-1; } else if(gmNote[v]&&gmRel[v]<0) gmRel[v]=0; }
                    else gmDrum(v-9,n); }
                if(gmRem[v]>0) gmRem[v]--; }
            if(gmCur==GM_WAR) gmWarExtras();
            gmRow++; }
        if(gmCur==GM_WAR){ if(gmCrowdS<gmCrowd) gmCrowdS+=2; else if(gmCrowdS>gmCrowd) gmCrowdS--; } else gmCrowdS=0;
        gmSwoop();
        if(gmInT>0) gmInT--;
        if(gmCur>=0){ const GmSong*S=&gmSongs[gmCur]; gmVoiceFrame(9,S); for(int v=0;v<9;v++) gmVoiceFrame(v,S); } }
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
