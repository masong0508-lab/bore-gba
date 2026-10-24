// voxscare.h - voice clips that were RECORDED IN BOTH VOICES (the ghost scare: ghost_scared_male_1..4 and ghost_scared_female_1..4 in tools/voices_src).
// Every other clip is recorded once, as the male voice, and voxPitch (house.h) speeds it up about five semitones for a female voice. These eight were recorded
// in both voices and ALREADY sped up, so a female Sim says a female take AT HER OWN PITCH: no second speed-up. Pure C (tested by tools/test_voxscare.sh).
// vsPick: who (look[LK_SEX], look[LK_VOICE]: 0 BY GENDER, 1 MALE, 2 FEMALE, uid), a random byte r, and p = voxPitch(uid) (the pitch percent THAT includes the gender speed-up)
// -> which clip to play and at what pitch percent. A non-binary Sim gets the male or the female takes by uid, half and half.
// The four takes of a voice must follow one another in the clip list (voices.h sorts the clips by file name); vsOk checks it, house.h asserts it at compile time.
#define VS_TAKES 4
static inline int vsFemale(int sx,int voice,int uid){ return voice==2||(voice==0&&(sx==SX_FEMALE||(sx==SX_NB&&(uid&1)))); }
static inline int vsBase(int sx,int voice){ return voice==2?133:voice==1?100:sx==SX_FEMALE?133:sx==SX_MALE?100:116; }   // voxPitch's own first step
static inline void vsPick(int sx,int voice,int uid,unsigned r,int p,int*clip,int*pct){
    *clip=(vsFemale(sx,voice,uid)?V_ghost_scared_female_1:V_ghost_scared_male_1)+(int)(r%VS_TAKES);
    *pct=p*100/vsBase(sx,voice);   // the speed-up is in the recording: take it out again (age, mood and the Sim's own bit stay)
}
static inline int vsOk(void){ return V_ghost_scared_female_4==V_ghost_scared_female_1+VS_TAKES-1&&V_ghost_scared_male_4==V_ghost_scared_male_1+VS_TAKES-1&&VOICE_N<127; }   // (the chain queue holds clips in signed chars)
