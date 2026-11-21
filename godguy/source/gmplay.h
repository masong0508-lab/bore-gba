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
#include "gglz.h"
#include "gmkit.h"
#ifdef GG_HQVOX
#include "gmvox_hq.h"      // the 3DS: the lyrics at 16 kHz (tools/gm_vox.py with GM_HQ=1)
#elif defined(GG_DSI16)
#include "gmvox_full.h"    // DSi 16 MiB: complete bank in ARM9
#elif defined(NDSHW)
#include "gmvox.h"         // Normal 4 MiB DS: all phrases, banked ARM9/ARM7
#else
#include "gmvox_compact.h" // Optional libnds ROM: smaller fallback
#endif        // the vocal chop kit: drum-B stream ids GM_VOX0 and up play a chop instead of a cymbal
#include "gmdata.h"
#define GMV 10                      // voice 9 is a Haas double of the lead (one frame late, a hair sharp, on the other side): it widens the lead
#ifdef __3DS__
// A small adaptive 3DS-world orchestration layer. It reuses the EXISTING
// ten wavetable voices, four drum channels and 16-instrument bank, so there
// is no second synthesizer, heap, mixer pass or sample-bank allocation.
static int gmWorldMode,gmWorldGain=100,gmWorldGoal=100;
static int gmWorldStem[9]={100,100,100,100,100,100,100,100,100};
static int gmWorldStemGoal[9]={100,100,100,100,100,100,100,100,100};
static int gmWorldDrums=100,gmWorldDrumGoal=100;
static int gmWorldMotif=-1,gmWorldPatch=13,gmWorldLastBeat=-1;
static int gmWorldMotifBase;
static const signed char gmWorldMotifs[6][16]={
    {0,99,7,4,99,9,7,99,4,99,7,2,99,5,4,99},
    {0,3,7,99,10,7,5,99,0,5,8,99,7,5,3,99},
    {0,99,12,7,99,5,7,99,0,99,10,7,99,5,3,99},
    {0,7,10,7,3,7,12,7,0,7,10,7,5,7,12,7},
    {0,99,3,99,7,99,10,99,12,99,7,99,5,99,3,99},
    // A brief homecoming fanfare after real quest completion.
    {0,4,7,12,7,4,12,16,12,7,4,99,7,99,99,99}
};
static int gmWorldToward(int x,int y,int step){
    return x<y?(y-x<step?y:x+step):x>y?(x-y<step?y:x-step):x;
}
static void gmWorldOff(void){
    gmWorldMode=0;gmWorldGain=gmWorldGoal=100;
    gmWorldDrums=gmWorldDrumGoal=100;gmWorldMotif=-1;
    gmWorldLastBeat=-1;
    for(int v=0;v<9;v++)gmWorldStem[v]=gmWorldStemGoal[v]=100;
}
static void gmWorldSet(int gain,int drums,int motif,int patch,
                       const unsigned char *stem){
    gmWorldMode=1;
    gmWorldGoal=gain;gmWorldDrumGoal=drums;
    if(gmWorldMotif!=motif)gmWorldLastBeat=-1;
    gmWorldMotif=motif;gmWorldPatch=patch;
    for(int i=0;i<9;i++)gmWorldStemGoal[i]=stem[i];
}
#endif
static int gmCur=-1, gmAcc, gmRow, gmMaster=100, gmPaused, gmTick;
// Referenced by gmPlay, so these must be declared before playback init.
static int gmLyrSlot=2, gmLyrDuck=45, gmEchoSide;
static int gmLyrEnd[2];
static int gmLyrT;      // frames the lyric phrase sounding now has left: the melody voices make room for it
static int gmCrowd, gmCrowdS;   // the size of the battle (0..400, set by the game) and how far the war music has opened up to it
static int gmInT, gmInN;                       // a song that follows another one fades in: frames left, frames in all
static int (*gmNextHook)(int);                 // asked when a song reaches its end: which song follows it (-1 = none: the music fades to silence and stops). Set by the game; without one every song just ends
static int gmPos[11], gmRem[11], gmNote[11], gmAge[11], gmRel[11], gmAlt[11];
static int gmHz64[128], gmSin[32], gmInit;
static int gmSlotA, gmSlotB;
static unsigned char gmBuf[GM_BUFMAX]; static const unsigned char*gmStr[11];   // the song's streams, unpacked when it starts (the ROM keeps them compressed)
static int fxCur=-1, fxPos, fxFrames, fxAge, fxNote, fxVol, fxKind, fxSlide, fxPitch8, fxOn, fxNoiseOn;
// Film-only three-bus sound direction. All music voices and drums move as
// ONE coherent bed. Dialogue beats and foreground actions gently lower it;
// effects retain their own headroom. Single integers, no buffers or DSP tax.
static int gmFilmOn,gmFilmDialogue,gmFilmPan=64;
static int gmFilmGain=100,gmFilmFxHold;
static void gmFilmMixSet(int active,int caption,int pan){
    gmFilmOn=!!active;
    gmFilmDialogue=gmFilmOn&&!!caption;
    gmFilmPan=gmFilmOn?(pan<32?32:pan>96?96:pan):64;
    if(!gmFilmOn)gmFilmFxHold=0;
}
static int gmFilmDesired(void){
    int goal=gmFilmOn?(gmFilmDialogue?76:90):100;
    if(gmFilmOn&&gmFilmFxHold>0&&goal>65)goal=65;
    return goal;
}
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
    gmInT=0; gmCur=s;
    gmLyrT=0;gmLyrEnd[0]=gmLyrEnd[1]=0;gmLyrSlot=2;
    { int o=0; for(int v=0;v<11;v++){ gmStr[v]=gmBuf+o; ggUnpack(gmSongs[s].v[v],gmBuf+o,gmSongs[s].n[v]); o+=gmSongs[s].n[v]; } } gmAcc=0; gmSlotA=gmSlotB=0; gmSeek(0); gmRow=0; ldRate=gmHz64[60]; ldVol=0; ldPan=64;
    for(int v=0;v<GMV;v++){
        int wave=gmSongs[s].inst[v<9?v:0];
#ifdef __3DS__
        if(gmWorldMode&&v==8&&gmWorldMotif>=0)wave=gmWorldPatch;
        gmWorldLastBeat=-1;
#endif
        gb_vstart(v,wave); gmAge[v]=0; gmRel[v]=-1; gmAlt[v]=0;
    }
}
// THE END OF A SONG. A song plays through once (from row 0 to its last row) and then it is over: it fades out over its last seconds and the next song takes over
// (gmNextHook), or the music stops. (The old player wrapped to the song's loop row and played the same song for ever.) A long suite brings its own closing fade (S->fade is the
// row it starts at); every other song gets one of 4 seconds. A jingle (loopRow 65535) is short and just ends.
static int gmFadeStart(const GmSong*S){ if(S->fade) return S->fade; if(S->loopRow==65535) return 0; int f=S->rows-S->bpm*4/15; return f<S->rows/2?S->rows/2:f; }
static int gmFadeMul(void){ const GmSong*S=&gmSongs[gmCur]; int fs=gmFadeStart(S), m=256;   // the closing fade, then the fade-in of a song that follows another
    if(fs&&gmRow>=fs){ int r=S->rows-fs; if(r<=0||gmRow>=S->rows) return 0; m=256*(S->rows-gmRow)/r; m=m*m>>8; }
    if(gmInT>0){ int a=256-256*gmInT/gmInN; m=m*(a*a>>8)>>8; }
    return m; }
static int gmNoClyr;      // (a test switch: CORPORATIONS' own voice is left out)
// Both streamed lyric channels are tracked independently; their
// declarations are above gmPlay, which resets them at song changes.
static void gmDrum(int part,int id){
    int soft=(id>>7)&1; id&=127; if(!id) return; int vol, pan, idx, slot; if(gmCur==GM_WAR&&part==1&&gmCrowdS<120) return; int wm=gmCur==GM_WAR?(gmCrowdS>300?78:100):100;      // (a small fight keeps to the war drums alone: the cymbals and chops join as the crowd grows)
    int lyr=0, echo=0;
    if(part==0){ static const u8 dv[8]={0,120,105,95,100,100,100,80}, dp[8]={0,64,64,70,38,62,88,76}; if(id>7) return; idx=id-1; vol=dv[id]; pan=dp[id]; slot=(gmSlotA^=1); }
    else if(id>=GM_VOX0){ idx=id-GM_VOX0; if(idx>=GM_VOX_N||!gmVoxLen[idx]) return; gmAlt[10]^=1; vol=gmVoxVol[idx]; pan=gmVoxPan[idx]; part=2;      // a vocal chop: it takes a cymbal channel for as long as it sounds
        if(idx>=GM_VOX_LYR0&&idx<GM_VOX_LYR0+GM_VOX_LYRN){ lyr=1; if(soft){ echo=1; soft=0; vol=vol*62/100; pan=(gmEchoSide^=1)?100:28; } }     // (a Duck & Cover lyric with the soft bit is its stereo echo copy: quieter, far to one side)
        else if(idx>=GM_VOX_CLYR0&&idx<GM_VOX_CLYR0+GM_VOX_CLYRN) lyr=2;
        if(lyr==2&&gmNoClyr)return;
        slot=2+(gmSlotB^=1);
        if(lyr&&!echo){
            int alt=5-slot;
            // Prefer the free channel. If both are occupied by live
            // lyrics, replace whichever will end sooner.
            if(gmLyrEnd[slot-2]>gmTick &&
               (gmLyrEnd[alt-2]<=gmTick ||
                gmLyrEnd[alt-2]<gmLyrEnd[slot-2]))slot=alt;
            int f=(int)((unsigned)gmVoxLen[idx]*60u/
                        (gmVoxRate[idx]?gmVoxRate[idx]:1))+2;
            gmLyrEnd[slot-2]=gmTick+f;
            gmLyrSlot=slot;
            if(f>gmLyrT){gmLyrT=f;gmLyrDuck=lyr==1?52:30;}
        }else{
            // Echoes and incidental chops must not erase active singing.
            // Under two concurrent lyrics, dropping an echo is better
            // than losing a word of foreground dialogue.
            if(gmLyrEnd[slot-2]>gmTick)slot=5-slot;
            if(gmLyrEnd[slot-2]>gmTick)return;
        }
    }
    else { static const u8 dv[9]={0,72,72,84,72,56,52,112,127}; if(id>8) return; idx=id-1; vol=dv[id]; gmAlt[10]^=1;      // (hats, open hats, crash, ride, shaker, soft hats: lifted a little, the top octaves were only 1% of the sound)
        pan=id>=7?64:id==3?34:id==4?100:id==2?92:(gmAlt[10]?46:82);
        slot=2+(gmSlotB^=1);
        if(gmLyrEnd[slot-2]>gmTick)slot=5-slot;
        if(gmLyrEnd[slot-2]>gmTick)return;
    } // A hi-hat never steals the channel of a still-playing lyric.
    if(soft) vol=vol*48/100;
    if(part==2&&!lyr){
        // Older tracker arrangements can still call the original 24
        // novelty IDs. Rebalance them as quiet, musical call-and-response
        // accents, never horn-blast lead vocals. Keep the singer's lyric
        // phrases (including all Duck & Cover / Corporations hooks) intact.
        if(idx<12)vol=vol*66/100;
        else if(idx<24)vol=vol*72/100;
        if(pan<40)pan=40;
        if(pan>87)pan=87;
    }
    // CORPORATIONS and DUCK & COVER: ease cymbal/vocal-chop backing
    // behind their own foreground lyrics instead of drowning consonants.
    // The original melody notes and lead vocal timing are unchanged.
    if(!lyr&&gmLyrT>0&&part!=0&&
       (gmCur==GM_CORPORATIONS||gmCur==GM_DUCKCOVER))
        vol=vol*78/100;
    vol=vol*wm/100*gmMaster/100*(lyr?92:40)/100;
    vol=vol*gmFilmGain/100; vol=vol*gmFadeMul()>>8;
#ifdef __3DS__
    if(gmWorldMode)
        vol=vol*gmWorldGain/100*gmWorldDrums/100;
#endif
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
    int vol=S->vol[v]*lev/100*gmMaster/100*56/100;
    vol=vol*gmFilmGain/100; vol=vol*gmFadeMul()>>8;
#ifdef __3DS__
    if(gmWorldMode)
        vol=vol*gmWorldGain/100*gmWorldStem[v]/100;
#endif
    if(gmLyrT>0&&(v<=3||v==7||v==8)) vol=vol*(100-gmLyrDuck*(gmLyrT>12?12:gmLyrT)/12)/100; if(gmCur==GM_WAR){ vol=vol*gmWarMix(v)/100; vol=vol*(92+gmCrowdS/40)/100; }
    if(pan<0) pan=0; if(pan>127) pan=127;
    gb_vset(v,rate,vol,pan);
    if(v==0){ ldRate=rate; ldVol=vol; ldPan=pan; }
}
// ORCHESTRA FOR THE BATTLE MUSIC, put on top at run time by the size of the crowd: timpani on the beats, crashes and a deep impact at the bar lines, tom fills before them, and a RISER
// (a reversed-cymbal swell with a low swell under it, sampled: gm_kit.py) into every big downbeat.
static void gmWarExtras(void){
    int c=gmCrowdS, r=gmRow&63, b=r&15; if(c<100) return;
    if(r==0&&c>=200){ gmDrum(1,8); gmDrum(0,4); gmDrum(1,3); }                                                 // the impact, a drum and a crash under it on the big downbeat
    else if(b==0||b==8) gmDrum(0,4);                                                                           // timpani on beats 1 and 3
    if(c>=200&&r>=60) gmDrum(0,4+(r-60));                                                                      // a tom fill going up into the next bar of four
    if(c>=200&&r==53) gmDrum(1,7);                                                                             // the riser through the last bar of four (it is 1.3 s long)
    if(c>=300&&r==21) gmDrum(1,7);
    if(c>=300&&r==32) gmDrum(1,8);
}
static void gmFrame(void){
    gmTick++;
    if(gmPaused) return;
    // Two gain points per 60 Hz tick: no sudden pumping when subtitles
    // appear or a hammer lands; settle back after the action.
    int goal=gmFilmDesired();
    if(gmFilmGain<goal) gmFilmGain+=(goal-gmFilmGain<2?goal-gmFilmGain:2);
    else if(gmFilmGain>goal)gmFilmGain-=(gmFilmGain-goal<2?gmFilmGain-goal:2);
    if(gmFilmFxHold>0)gmFilmFxHold--;
#ifdef __3DS__
    if(gmWorldMode){
        gmWorldGain=gmWorldToward(gmWorldGain,gmWorldGoal,2);
        gmWorldDrums=gmWorldToward(gmWorldDrums,gmWorldDrumGoal,2);
        for(int v=0;v<9;v++)
            gmWorldStem[v]=gmWorldToward(gmWorldStem[v],gmWorldStemGoal[v],2);
    }
#endif
    if(gmLyrT>0) gmLyrT--;
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
        if(gmInT>0) gmInT--;
#ifdef __3DS__
        // Five original context-dependent melodic figures are arranged
        // against the score's live bass notes, replacing ONLY the
        // sparkle voice. No extra polyphony or per-sample DSP. Spare the
        // full vocal mixes of CORPORATIONS and DUCK & COVER.
        if(gmWorldMode&&gmWorldMotif>=0&&
           gmCur!=GM_CORPORATIONS&&gmCur!=GM_DUCKCOVER){
            int tickBeat=gmRow>>2;
            if(gmWorldLastBeat<0)gmWorldMotifBase=tickBeat;
            int beat=tickBeat-gmWorldMotifBase;
            if(beat!=gmWorldLastBeat){
                gmWorldLastBeat=beat;
                int off=gmWorldMotifs[gmWorldMotif][beat&15];
                if(off==99){
                    if(gmNote[8]&&gmRel[8]<0)gmRel[8]=0;
                }else{
                    int root=gmNote[6];
                    if(root<36||root>74)root=48;
                    int n=root+off;
                    if(n>82)n=82;
                    gmNoteOn(8,n);
                }
            }else if((gmRow&3)==3&&gmRel[8]<0)gmRel[8]=0;
        }
#endif
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
static void fxPlay(int i){
    if(i<0||i>=GM_FX) return;
    if(gmFilmOn)gmFilmFxHold=36; // music makes room for actual physical sound
    if(fxOn){gb_poff();fxOn=0;}
    if(fxNoiseOn){gb_pnoff();fxNoiseOn=0;}
    fxCur=i;fxPos=0;fxAge=0;
}
#endif
