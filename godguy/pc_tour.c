// Runs the REAL Bore: GodGuy program (source/main.c, DS user interface and all) on the PC through compat/ndsshim.h with scripted keys and stylus, saves screenshots of both screens
// and the sound it produced. gcc -O2 -DNDSSHIM -Icompat -Isource -o pc_ds pc_ds.c -lm && ./pc_ds outdir
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
static const char*outDir="/tmp/gg_ds"; static short*audio; static long audioN, audioCap; static double carry;
typedef struct { int f0, f1, keys, tx, ty; } Ev;
#define K_ (KEY_TOUCH)
static const Ev script[]={ {20,22,KEY_A,0,0}, {60,62,KEY_A,0,0}, {500,502,KEY_A,0,0} };
static int shots[]={310,322,334,346,358,370,382,394,406,418,430,442,454,466,478,490,502,514,526,538,550,562,574,586,598,610,622,634,646,658,670,682,694,706,718,730,742,754,766,778,790,802,814,826,838,850,862,874,886,-1};
void shimInput(int frame,int*held,int*tx,int*ty){
    { int n=(frame-300)/12; if(frame>=300&&(frame-300)%12==0){ menuOn=0; menuAsk=0; extrasView=0; sceneOn=0; dlgOn=0; dlgPend=-1; titleOn=0; page=0; paused=1;
        switch(n){ case 0: titleOn=1; mScr=0; mSel=0; break; case 1: titleOn=1; mScr=0; mSel=1; break; case 2: titleOn=1; mScr=1; mSel=0; break; case 3: titleOn=1; mScr=2; mSel=0; break; case 4: titleOn=1; mScr=2; mSel=9; break;
            case 5: titleOn=1; mScr=3; mSel=0; cSel=0; createStart(); break; case 6: titleOn=1; mScr=3; cSel=9; break; case 7: titleOn=1; mScr=3; cSel=2; break;
            case 8: menuOn=1; mScr=0; mSel=0; break; case 9: menuOn=1; mScr=1; mSel=0; break; case 10: menuOn=1; mScr=2; mSel=0; break; case 11: menuOn=1; mScr=0; mSel=4; menuAsk=6; break;
            case 12: page=0; break; case 13: page=1; break; case 14: page=2; extrasView=1; break; case 15: page=3; extrasView=1; break; case 16: page=4; extrasView=1; break; case 17: page=5; extrasView=1; break;
            case 18: page=6; extrasView=1; break; case 19: paused=0; planetSize=192; rs=777u*2654435761u+31337u; newWorld(); monOn=1; for(int t=1;t<=45000;t++){ tick=t; year=t/1200; simulate(); } tick=700; page=7; extrasView=1; break; case 20: page=8; extrasView=1; break; case 21: page=9; extrasView=1; break;
            case 22: page=3; extrasView=1; auralOpen=1; break; case 23: dlgSet=dlgTab[3]; dlgIdx=1; dlgChars=999; dlgOn=1; break; case 24: sceneStart(introTxt,0); break; case 25: sceneStart(credTxt,1); sceneOff=10; break;
            case 26: menuOn=1; mScr=2; mSel=5; break; case 27: titleOn=1; mScr=2; mSel=3; break; case 28: menuOn=1; mScr=1; mSel=1; break;
            case 29: page=10; extrasView=1; break; case 30: page=11; extrasView=1; langSel=2; break; case 31: page=7; extrasView=1; break;
            case 32: { int v=0; for(int i=0;i<VIL_N;i++) if(vil[i].on&&vil[i].pop>3){ v=i; break; } inspKind=2; inspIdx=v; inspT=900; page=0; } break;
            case 33: { int ci=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].rk){ ci=i; break; } if(!cr[ci].rk) for(int i=0;i<nCr;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==5){ ci=i; break; } for(int i=0;i<nCr;i++) if(cr[i].alive&&abs(cr[i].x-cr[ci].x)<=6&&abs(cr[i].y-cr[ci].y)<=6) sick[i]=1; sick[ci]=1; inspKind=1; inspIdx=ci; inspT=900; page=0; } break;
            case 34: inspT=0; page=0; power=24; break; case 35: page=8; extrasView=1; break; case 36: titleOn=1; mScr=0; mSel=1; break;
            case 37: page=12; extrasView=1; brCat=2; brSel=0; for(int v=0;v<VIL_N;v++) if(vil[v].on){ favVil[v]=1; break; } favKd[0]=1; favPow[4]=1; break; case 38: page=12; extrasView=1; brCat=0; brSel=0; break; case 39: page=12; extrasView=1; brCat=3; brSel=0; break;
            case 40: menuOn=1; mScr=0; mSel=0; titleOn=0; break; case 41: menuOn=1; mScr=4; mSel=4; lawOff|=(1u<<LW_PEACEMON)|(1u<<LW_AGE); break; case 45: menuOn=1; mScr=8; mSel=3; lawOff|=(1u<<LW_POP100)|(1u<<LW_REBEL); break; case 47: menuOn=1; titleOn=0; mScr=9; mSel=0; nSlots=9; for(int k=0;k<9;k++) memset(&slotMeta[k],0,sizeof slotMeta[k]); slotMeta[0]=(SlotMeta){1,1,128,12,15000,2750}; slotMeta[1]=(SlotMeta){1,0,256,41,80000,17539}; slotMeta[4]=(SlotMeta){1,1,192,4,9000,3876}; curSlot=1; slotSel=1; slotAsk=2; strcpy(slotMsg,"PRESS X AGAIN TO OVERWRITE SLOT 2"); break; case 48: nSlots=3; slotSel=0; slotAsk=0; slotMsg[0]=0; break; case 46: page=13; extrasView=1; atO2=14*16; atCO2=44*16; atTemp=9*16; atDust=12*16; for(int i=0;i<48;i++) atHist[i]=(s8)(i/4-3); atHistN=48; break; case 42: menuOn=1; mScr=5; mSel=0; break; case 43: menuOn=1; mScr=6; mSel=0; break; case 44: menuOn=1; mScr=7; mSel=0; break;
            default: page=0; break; } hud(); } }
    if(frame==1140){ weather=2; weatherT=3000; } if(frame==1248||frame==1249) flashT=6; if(frame==1290){ weather=3; weatherT=3000; }
    *held=0; for(unsigned i=0;i<sizeof script/sizeof script[0];i++) if(frame>=script[i].f0&&frame<script[i].f1){ *held|=script[i].keys; if(script[i].keys&KEY_TOUCH){ *tx=script[i].tx; *ty=script[i].ty; } }
    if(frame>=900){ char fn[200]; sprintf(fn,"%s/audio.wav",outDir); FILE*w=fopen(fn,"wb"); int dl=(int)(audioN*4); unsigned hdr[11]={0x46464952,36+dl,0x45564157,0x20746d66,16,0x00020001,SHIM_SR,SHIM_SR*4,0x00100004,0x61746164,dl}; fwrite(hdr,4,11,w); fwrite(audio,4,audioN,w); fclose(w);
        printf("frames %d, audio %.1fs, peak %.2f, clipped samples %ld, failed channel allocations %d, creatures %d, year %d, chapter %d, stUse %u\n",frame,audioN/(double)SHIM_SR,shimPeak,shimClip,shimChFail,nCr,year,chapter,(unsigned)stUse);
        printf("sound channels in use now: "); int u=0; for(int i=0;i<16;i++) u+=shimCh[i].on; printf("%d of 16\n",u); exit(0); } }
void shimPresent(void){
    double want=SHIM_SR/60.0+carry; int n=(int)want; carry=want-n; if(audioN+n>audioCap){ audioCap=audioCap?audioCap*2:SHIM_SR*8; audio=realloc(audio,audioCap*4); } shimMix(audio+audioN*2,n); audioN+=n;
    for(int i=0;shots[i]>=0;i++) if(shots[i]==shimFrame){ static u8 top[400*240*3], bot[320*240*3]; shimRender(top,bot); char fn[200]; sprintf(fn,"%s/top_%d.ppm",outDir,shimFrame); FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n400 240\n255\n"); fwrite(top,1,sizeof top,f); fclose(f);
        sprintf(fn,"%s/bot_%d.ppm",outDir,shimFrame); f=fopen(fn,"wb"); fprintf(f,"P6\n320 240\n255\n"); fwrite(bot,1,sizeof bot,f); fclose(f); } }
int main(int argc,char**argv){ if(argc>1) outDir=argv[1]; mkdir(outDir,0777); if(chdir(outDir)) return 1; return ds_main(); }
