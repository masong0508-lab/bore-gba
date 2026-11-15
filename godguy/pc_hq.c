// Plays one song of the REAL game through the shim mixer and writes the sound: the way to hear (and measure) the 3DS mixer.
//   gcc -O2 -DNDSSHIM [-DSHIM_HQ -DGG_HQVOX] -Icompat -Isource -include unistd.h -o pc_hq pc_hq.c -lm && ./pc_hq out.wav <track> <seconds>
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
static const char*outWav; static int track, secs; static short*audio; static long audioN, audioCap; static double carry; static double dry2, wet2;
void shimInput(int frame,int*held,int*tx,int*ty){ (void)tx;(void)ty; *held=0;
    if(frame==30){ gmStop(); jukeOn=1; gmPlay(track); musHold=999999; }
    if(getenv("GMCROWD")&&frame>30){ int cv=atoi(getenv("GMCROWD")); gmCrowd=cv<0?(frame-30)*400/(secs*60):cv; }
    if(frame>=30+secs*60){ FILE*w=fopen(outWav,"wb"); int dl=(int)(audioN*4); unsigned hdr[11]={0x46464952,36+dl,0x45564157,0x20746d66,16,0x00020001,SHIM_SR,SHIM_SR*4,0x00100004,0x61746164,dl}; fwrite(hdr,4,11,w); fwrite(audio,4,audioN,w); fclose(w);
        printf("%s: %d s, peak %.2f, clipped %ld, failed allocations %d\n",gmSongs[track].name,secs,shimPeak,shimClip,shimChFail); exit(0); } }
void shimPresent(void){ double want=SHIM_SR/60.0+carry; int n=(int)want; carry=want-n; if(audioN+n>audioCap){ audioCap=audioCap?audioCap*2:SHIM_SR*8; audio=realloc(audio,audioCap*4); } shimMix(audio+audioN*2,n); audioN+=n; }
int main(int argc,char**argv){ outWav=argv[1]; track=atoi(argv[2]); secs=atoi(argv[3]); static char d[]="/tmp/gg_hqdir"; mkdir(d,0777); if(chdir(d)) return 1; return ds_main(); }
