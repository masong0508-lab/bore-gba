// jukebox.h - which songs the jukebox, the menus and the game music may play, and picking one at random. No hardware in here
// (the player and screen live in main.c). Needs before it: u8 / u16, rnd8(), SRAM_BASE.
//
// Every song has an on/off flag: the check box in the jukebox's PLAYLIST tab. Only songs that are ON are picked at random: when the jukebox opens,
// when a main menu opens, when a playlist song ends, and for GAME MUSIC. The flags are saved in SRAM (JB_OFF). Songs added at the END of songs.h
// come in switched on; the saved flags are dropped (everything on again) only if the songs in front of them changed.
#define JB_MAX 64          // most songs the jukebox can hold (the flags need one bit each; songs.h may list up to this many, secret ones included)
#define JB_OFF 5056        // SRAM block: 'J' 'B' '3', song count, hash of the song names (2 bytes), then one bit per song (1 = on). 14 bytes of the 80 reserved (slots.h)
static u8 jbMap[JB_MAX];   // visible song number -> index into songs[] (hides the placeholder tunes and the secret ones)
static u8 jbOn[JB_MAX];    // by songs[] index: 1 = may be picked at random
static int jbN, jbAll;     // visible songs, songs in songs[]
static u8 jbUl;                // unlocked songs (unlocks.h): a set bit = unlocked. Saved at JB_OFF+16: 'U' 'L', the bits, the bits xor 0x5A
static u16 (*jbHashFn)(int);   // hash of the names of the first n songs (main.c): tells whether the saved flags still belong to this song list

static void jbSave(void){
    volatile u8*m=SRAM_BASE+JB_OFF; u16 h=jbHashFn(jbAll);
    m[0]='J'; m[1]='B'; m[2]='3'; m[3]=(u8)jbAll; m[4]=(u8)h; m[5]=(u8)(h>>8);
    for(int b=0;b<8;b++){ u8 v=0; for(int k=0;k<8;k++){ int i=b*8+k; if(i<jbAll&&jbOn[i]) v|=(u8)(1<<k); } m[6+b]=v; }
}
static void jbInit(int nAll,int nVis,u16 (*hash)(int)){   // call at boot (and after an erase), once the visible list jbMap[] is built
    jbAll=nAll>JB_MAX?JB_MAX:nAll; jbN=nVis; jbHashFn=hash;
    for(int i=0;i<JB_MAX;i++) jbOn[i]=1;
    volatile u8*m=SRAM_BASE+JB_OFF; int n=m[3];
    if(m[0]=='J'&&m[1]=='B'&&m[2]=='3'&&n<=jbAll&&(u16)(m[4]|(m[5]<<8))==hash(n))
        for(int i=0;i<n;i++) jbOn[i]=(u8)((m[6+(i>>3)]>>(i&7))&1);   // songs added after the save stay on
}
static void jbUlSave(void){ volatile u8*m=SRAM_BASE+JB_OFF; m[16]='U'; m[17]='L'; m[18]=jbUl; m[19]=(u8)(jbUl^0x5A); }
static void jbUlLoad(void){ volatile u8*m=SRAM_BASE+JB_OFF; jbUl=(m[16]=='U'&&m[17]=='L'&&(u8)(m[18]^0x5A)==m[19])?m[18]:0; }
static int jbOnVis(int v){ return jbOn[jbMap[v]]; }
static int jbCount(void){ int c=0; for(int i=0;i<jbN;i++) c+=jbOnVis(i); return c; }
static void jbToggle(int v){ jbOn[jbMap[v]]^=1; jbSave(); }
// A random visible song that is ON, never `avoid` (the one that just played) while there is another. With every song off, all of them count.
static int jbPick(int avoid){
    int any=(jbCount()==0), c=0;
    for(int i=0;i<jbN;i++) if((any||jbOnVis(i))&&i!=avoid) c++;
    if(c==0) return (avoid>=0&&avoid<jbN)?avoid:-1;                 // the only candidate is the one to avoid (or there is no song at all)
    int r=(int)((((unsigned)rnd8()<<8|(unsigned)rnd8())*(unsigned)c)>>16);
    for(int i=0;i<jbN;i++){ if(i==avoid||!(any||jbOnVis(i))) continue; if(r--==0) return i; }
    return -1;
}
