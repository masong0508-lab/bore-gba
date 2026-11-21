// Test actual 60Hz GodGuy playback channel arbitration using the
// production song/ADPCM catalog. Full-length foreground lyrics should
// survive cymbals, decorative honks and optional stereo echo copies.
// A PC software backend records all four slots without hardware audio.
#define main godguy_music_renderer_main
#include "pc_music.c"
#undef main
#include <assert.h>
static int live(int n){return gmLyrEnd[n-2]>gmTick;}
int main(void){
    gmTables();gmPlay(GM_CORPORATIONS);
    gmTick=120;
    assert(gmLyrEnd[0]==0&&gmLyrEnd[1]==0);
    int first=GM_VOX0+GM_VOX_CLYR0;
    assert(gmVoxLen[GM_VOX_CLYR0]>0);
    assert(gmVoxLen[GM_VOX_CLYR0+1]>0);
    gmDrum(1,first);
    int a=gmLyrSlot,firstId=hit[a].idx,firstEnd=gmLyrEnd[a-2];
    assert((a==2||a==3)&&live(a));
    gmTick+=8;
    gmDrum(1,first+1);
    int b=gmLyrSlot,secondId=hit[b].idx,secondEnd=gmLyrEnd[b-2];
    assert(b==5-a&&live(a)&&live(b));
    for(int k=0;k<30;k++){
        // Multiple ordinary cymbals and legacy novelty chops cannot
        // clobber PCM in either occupied channel.
        gmDrum(1,(k&1)?3:GM_VOX0+12);
        assert(hit[a].idx==firstId&&hit[b].idx==secondId);
        assert(gmLyrEnd[a-2]==firstEnd&&gmLyrEnd[b-2]==secondEnd);
    }
    // Duck & Cover's optional stereo echo has lower priority than
    // the two existing lyric lines.
    gmDrum(1,(GM_VOX0+GM_VOX_LYR0)|128);
    assert(hit[a].idx==firstId&&hit[b].idx==secondId);
    int preferred=firstEnd<secondEnd?a:b;
    gmDrum(1,first+2);
    assert(gmLyrSlot==preferred&&live(preferred));
    gmPlay(GM_DUCKCOVER);
    assert(!gmLyrEnd[0]&&!gmLyrEnd[1]);
    gmTick+=20;
    gmDrum(1,GM_VOX0+GM_VOX_LYR0);
    assert(live(gmLyrSlot));
    gmPlay(GM_CORPORATIONS);
    gmNoClyr=1;
    gmDrum(1,first);
    assert(!gmLyrEnd[0]&&!gmLyrEnd[1]&&gmLyrT==0);
    printf("VOCAL PRIORITY OK: simultaneous lyric lines protected; "
           "cymbals, novelty chops, echoes deferred; tracks reset\n");
    return 0;
}
