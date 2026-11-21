// The Unwritten Reign: reactive 3DS score using GodGuy's existing 168
// tracker songs, 16 sampled wavetable timbres, drum kit and vocal ducking.
// Fifteen states, five NEW bass-aware counterpoint motifs. No extra PCM
// voice, wavetable buffer, sample load, sound thread or second sequencer.
#ifndef GG_WORLD_MUSIC3D_H
#define GG_WORLD_MUSIC3D_H
enum { GG_MUSIC_FIELDS,GG_MUSIC_FOREST,GG_MUSIC_COAST,
       GG_MUSIC_HIGHLANDS,GG_MUSIC_TOWN,GG_MUSIC_CITY,GG_MUSIC_NIGHT,
       GG_MUSIC_DANGER,GG_MUSIC_BATTLE,GG_MUSIC_BOSS,GG_MUSIC_AWAKEN,
       GG_MUSIC_CHOICE,GG_MUSIC_END,GG_MUSIC_DEFEAT,GG_MUSIC_PREP,
       GG_MUSIC_STATES };
typedef struct {
    int active,state,wanted,track,hold,candidate,stable,fade;
    int scanned,frames,urgent,transitions;
    int completedSeen,celebrate;
} GgWorldMusic;
static GgWorldMusic ggWorldMusic;
// Themes are variations, not loops started on every step. Existing
// remastered tracks are reorchestrated in real time; the urban score can
// use CORPORATIONS and the combat score can use DUCK & COVER without
// trampling their original lyric arrangements.
static const int ggWorldTracks[GG_MUSIC_STATES][3]={
    {GM_MORNING,GM_FIRSTSUN,GM_GLASSRIVER},
    {GM_JUNGLE,GM_DREAMINGDEEP,GM_LANTERNRIVER},
    {GM_OCEAN,GM_SAILS,GM_OCEANKEEPS},
    {GM_MTNKING,GM_STONECIRCLES,GM_LONGWINTER},
    {GM_HEARTH,GM_VILLAGE,GM_CARAVAN},
    {GM_CITY,GM_GLASSCITY,GM_CORPORATIONS},
    {GM_NIGHT,GM_MIDNIGHTP,GM_COUNTINGSTARS},
    {GM_BLOODMOON,GM_MONSTER,GM_LASTBEACON},
    {GM_WAR,GM_SIEGE,GM_DUCKCOVER},
    {GM_SIEGE,GM_CROWNTHORNS,GM_GREATCLOCK},
    {GM_FIRSTSUN,GM_GENESIS,GM_HEARTH},
    {GM_LASTBEACON,GM_ALLKINGS,GM_CROWNTHORNS},
    {GM_HOMEWORLD,GM_ASCEND,GM_GODHOOD},
    {GM_ASHES,GM_GAMEOVER,GM_NIGHT},
    {GM_CARAVAN,GM_HEARTH,GM_FIELDSGOLD}
};
static const unsigned char ggWorldLayers[GG_MUSIC_STATES][9]={
    {72,35,50,60,100,100,96,36,54},
    {67,38,60,67,100,100,93,36,49},
    {62,30,52,72,100,100,95,26,63},
    {80,38,58,70,96,96,100,38,48},
    {75,52,76,80,90,90,87,60,49},
    {93,60,80,80,89,89,96,72,66},
    {44,20,35,48,100,100,88,25,52},
    {63,34,58,60,93,93,100,34,38},
    {100,85,90,93,93,93,100,96,80},
    {100,80,100,100,100,100,100,94,90},
    {46,28,49,50,100,100,80,32,52},
    {60,35,50,60,100,100,100,45,50},
    {83,62,88,86,100,100,100,74,78},
    {40,20,35,55,100,100,77,16,27},
    {65,38,57,66,98,98,86,43,48}
};
static const signed char ggWorldMotifFor[GG_MUSIC_STATES]={
    0,1,2,4,0,3,2,4,3,3,2,4,0,4,1
};
static const unsigned char ggWorldPatchFor[GG_MUSIC_STATES]={
    6,10,5,11,6,14,13,13,11,11,12,12,5,13,6
};
static int ggWorldDanger(void){
    int x=ggExplore.px>>8,y=ggExplore.py>>8,best=999;
    if(!ggChron.active)return best;
    for(int i=0;i<ggChron.foeN;i++)if(ggChron.foe[i].hp>0){
        int d=ggChronNear(x,y,ggChron.foe[i].x,ggChron.foe[i].y);
        if(d<best)best=d;
    }
    return best;
}
static int ggWorldMusicScene(void){
    if(ggOrigin.active)return GG_MUSIC_AWAKEN;
    if(ggSaga.mode){
        if(ggSaga.mode==GG_SAGA_CHOICE)return GG_MUSIC_CHOICE;
        return ggSaga.mode==GG_SAGA_OUTRO?GG_MUSIC_END:GG_MUSIC_PREP;
    }
    if(ggChron.active&&(ggChron.lost||ggChron.hp<=0))return GG_MUSIC_DEFEAT;
    if(ggChron.active&&ggChron.won)return GG_MUSIC_END;
    int threat=ggWorldDanger();
    if(threat<=7){
        if(ggChron.act==3&&ggChron.post==GG_CHRON_POSTS-1)
            return GG_MUSIC_BOSS;
        return threat<=4?GG_MUSIC_BATTLE:GG_MUSIC_DANGER;
    }
    if(ggChron.active&&ggChron.briefTime>0)return GG_MUSIC_PREP;
    int x=ggExplore.px>>8,y=ggExplore.py>>8;
    if(x<0||y<0||x>=WW||y>=WH)return GG_MUSIC_FIELDS;
    int v=ggReignVillage(x,y,7);
    if(v>=0)return vil[v].lvl>=3||ggReign.office>=3?
                    GG_MUSIC_CITY:GG_MUSIC_TOWN;
    if(ggChron.active&&ggChron.day==3)return GG_MUSIC_NIGHT;
    int t=world[y][x];
    if(t<=T_WATER)return GG_MUSIC_COAST;
    if(t==T_MOUNT||t==T_HILL||t==T_SNOW)return GG_MUSIC_HIGHLANDS;
    if(t==T_FOREST||t==T_JUNGLE||t==T_SWAMP)return GG_MUSIC_FOREST;
    return GG_MUSIC_FIELDS;
}
static int ggWorldMusicTrack(int scene){
    // World seed, act, route and civic history change the score without
    // allocating unique scores for every biome or settlement.
    unsigned variety=(unsigned)scene*7u+(unsigned)(ggChron.act+1)*3u+
                     (unsigned)(ggQuest.completed/2)+(unsigned)ggReign.office;
    if(scene==GG_MUSIC_CITY&&ggReign.role==GG_ROLE_TRADER)
        return GM_CORPORATIONS;
    if(scene==GG_MUSIC_BATTLE&&ggChron.act>=2&&
       ggReign.role==GG_ROLE_SOLDIER)
        return GM_DUCKCOVER;
    return ggWorldTracks[scene][variety%3u];
}
static int ggWorldMusicNext(int previous){
    (void)previous;
    // On song end, repeat the appropriate world theme, not PAUSE (the
    // underlying 2D simulation is intentionally paused while roaming).
    return ggWorldMusic.track>=0?ggWorldMusic.track:GM_HEARTH;
}
static void ggWorldMusicReset(void){
    if(!ggWorldMusic.active)return;
    ggWorldMusic.active=0;
    gg3dMusicNext=0;
    gmWorldOff();
    gmFilmMixSet(0,0,64);
    gmStop();
    musLast=-1;musHold=0;
    // Re-enter the original musical director immediately rather than
    // relying on a potentially paused 2D simulation to advance tick&31.
    musReq(musDirector());
}
static int ggWorldMusicTick(void){
    if(!ggExplore.active){
        ggWorldMusicReset();
        return 0;
    }
    if(!ggWorldMusic.active){
        ggWorldMusic.active=1;
        ggWorldMusic.frames=0;ggWorldMusic.scanned=0;
        ggWorldMusic.hold=0;ggWorldMusic.fade=0;
        ggWorldMusic.state=-1;ggWorldMusic.candidate=-1;
        ggWorldMusic.stable=0;ggWorldMusic.track=-1;
        ggWorldMusic.transitions=0;
        ggWorldMusic.completedSeen=ggQuest.completed;
        ggWorldMusic.celebrate=0;
    }
    gg3dMusicNext=ggWorldMusicNext;
    ggWorldMusic.frames++;
    if(ggWorldMusic.hold>0)ggWorldMusic.hold--;
    if(ggQuest.completed>ggWorldMusic.completedSeen){
        ggWorldMusic.completedSeen=ggQuest.completed;
        ggWorldMusic.celebrate=192; // original 3DS-only victory phrase
    }
    if(ggWorldMusic.celebrate>0)ggWorldMusic.celebrate--;
    // A fixed 12-tick polling rate keeps world/village/foe checks out
    // of the per-sample and most per-frame paths. Urgent transitions
    // still start within one 12-frame block.
    if(!ggWorldMusic.scanned||--ggWorldMusic.scanned<=0){
        ggWorldMusic.scanned=12;
        int scene=ggWorldMusicScene();
        if(scene!=ggWorldMusic.candidate){
            ggWorldMusic.candidate=scene;
            ggWorldMusic.stable=0;
        }else ggWorldMusic.stable+=12;
        int urgency=(scene==GG_MUSIC_BATTLE||scene==GG_MUSIC_BOSS||
                     scene==GG_MUSIC_DEFEAT||scene==GG_MUSIC_CHOICE);
        if(ggWorldMusic.state<0||
           (scene!=ggWorldMusic.state&&
            (urgency||(ggWorldMusic.stable>=48&&ggWorldMusic.hold==0)))){
            ggWorldMusic.state=scene;
            ggWorldMusic.wanted=ggWorldMusicTrack(scene);
            ggWorldMusic.urgent=urgency;
            ggWorldMusic.hold=urgency?48:300;
        }
    }
    int s=ggWorldMusic.state;
    if(s<0)return 1;
    int talk=ggQuest.talk||ggReign.menu;
    int cinematic=ggSaga.mode||ggOrigin.active;
    gmFilmMixSet(cinematic||talk,cinematic||talk,64);
    // Re-orchestrate the existing track without adding more voices.
    // Quiet worlds emphasize strings/pads, urban worlds brass/arp, war
    // brings the drums forward. Instrument gain never exceeds 100%.
    int motif=ggWorldMotifFor[s];
    if(ggWorldMusic.celebrate>0&&s!=GG_MUSIC_BATTLE&&
       s!=GG_MUSIC_BOSS)motif=5;
    if(talk||s==GG_MUSIC_CHOICE||s==GG_MUSIC_END)
        motif=-1;  // dialogue and voiced finales retain clear headroom
    int drums=(s==GG_MUSIC_BOSS||s==GG_MUSIC_BATTLE)?100:
              (s==GG_MUSIC_CITY?74:s==GG_MUSIC_DANGER?60:43);
    int bed=talk?82:cinematic?89:100;
    if(ggWorldMusic.track!=ggWorldMusic.wanted){
        // Two-phase, click-free change: ramp down through the normal
        // mixer, switch a tracker song once, then ramp back up with the
        // sequencer's existing 45-frame fade-in.
        if(ggWorldMusic.track>=0&&gmCur>=0){
            ggWorldMusic.fade=1;
            gmWorldSet(0,drums,motif,ggWorldPatchFor[s],
                       ggWorldLayers[s]);
        }
        if(ggWorldMusic.track<0||gmCur<0||gmWorldGain<=8){
            gmWorldSet(100,drums,motif,ggWorldPatchFor[s],
                       ggWorldLayers[s]);
            ggWorldMusic.track=ggWorldMusic.wanted;
            gmPlay(ggWorldMusic.track);
            gmInN=gmInT=64;
            ggWorldMusic.fade=0;
            ggWorldMusic.transitions++;
        }
    }else{
        ggWorldMusic.fade=0;
        gmWorldSet(bed,drums,motif,ggWorldPatchFor[s],
                   ggWorldLayers[s]);
        if(gmCur<0){
            gmPlay(ggWorldMusic.track);
            gmInN=gmInT=64;
        }
    }
    // No 2D musDirector switching or paused-state track stealing while
    // the 3DS RPG owns the score.
    return 1;
}
#endif
