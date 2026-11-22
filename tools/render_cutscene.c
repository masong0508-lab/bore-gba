/* Native development preview of the actual cutscene renderer.
 * render_cutscene.sh supplies the game's drawing/font routines from main.c.
 * This does not emulate GBA timing, sound, hardware fades, or gameplay.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#define SW 240
#define SH 160
#define RGB(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
#define WHITE RGB(31,31,31)
#define GOLD RGB(31,26,6)
#define EWRAM_BSS
#define IWRAM_CODE
#define IWRAM_ARM
#define IWRAM_THUMB
#define IWRAM_OVL2
#define CS_HOST
#define VOICE_N 0

static struct {
    u32 before[4];
    u16 pixels[SW*SH];
    u32 after[4];
} frame;
static u16 *fb = frame.pixels;
static int cX0, cY0;
static unsigned cW=SW, cH=SH;
static u32 lrng=12345;
static void fill32(u32 *q,u32 v,int n){ while(n-->0) *q++=v; }
#include "render_core.h"
#include "cutscene.h"
#include "render_caption.h"

static int integer(const char *s){
    char *end; errno=0;
    long n=strtol(s,&end,10);
    if(errno||!*s||*end||n<0||n>100000){
        fprintf(stderr,"Invalid nonnegative integer: %s\n",s); exit(2);
    }
    return (int)n;
}
static void verify_frame(void){
    for(int i=0;i<4;i++) if(frame.before[i]!=0x1234abcdu||frame.after[i]!=0x1234abcdu){
        fputs("Cutscene wrote outside its framebuffer\n",stderr); exit(1);
    }
}
static void output(const char *path){
    FILE *f=fopen(path,"wb");
    if(!f){ perror(path); exit(1); }
    fprintf(f,"P6\n%d %d\n255\n",SW,SH);
    for(int i=0;i<SW*SH;i++){
        u16 c=fb[i];
        /* Expand GBA 5-bit channels as mGBA's native video buffer does. */
        unsigned char rgb[3];
        for(int k=0;k<3;k++){ int v=(c>>(k*5))&31; rgb[k]=(u8)((v<<3)|(v>>2)); }
        if(fwrite(rgb,1,3,f)!=3){ perror(path); exit(1); }
    }
    if(fclose(f)){ perror(path); exit(1); }
}
/* Broad image properties, independent of the individual paint calls. The
 * isolated patient uses a black backdrop so connected skin and contact with
 * the light bedding can be checked without the standing cast or room art.
 */
static int warm_pixel(u16 c){
    int r=c&31, g=(c>>5)&31, b=(c>>10)&31;
    return r>g&&g>=b;
}
static int bedding_pixel(u16 c){
    return (c&31)>=14&&((c>>5)&31)>=21&&((c>>10)&31)>=24;
}
static int patient_failure(const char *what,int zoom,int pose,int t){
    fprintf(stderr,"Patient check failed: %s (zoom=%d pose=%d frame=%d)\n",
            what,zoom,pose,t);
    return 1;
}
static int check_patient(void){
    static u16 head[SW*SH];
    static u8 arm[SW*SH];
    static int queue[SW*SH];
    const int zooms[]={256,560,1024};
    int frames=0;
    csCurSc=3; csCurBi=0; csBgNow=CB_HOSP;
    csOx=csOy=csFtrack=csTalking=csSpk=0;
    for(unsigned zi=0;zi<sizeof(zooms)/sizeof(zooms[0]);zi++){
        int zoom=zooms[zi];
        /* This view contains the complete patient even at 4x. Do not clamp
         * to the room: this is a geometry check, not a directed scene shot.
         */
        csCz=zoom; csCxF=146<<4; csCyF=88<<4;
        int neck_end=csCamX(141), arm_start=csCamX(142), arm_end=csCamX(158);
        for(int stirring=0;stirring<2;stirring++) for(int t=0;t<=180;t++){
            int pose=stirring?CP_STIR:CP_LIE;
            memset(fb,0,sizeof(frame.pixels));
            clipSet(0,12,SW,116);
            csWardPatient(164,98,CA_MISSY,pose,t,RGB(24,16,10),RGB(5,3,2));
            verify_frame(); frames++;
            int head_count=0, support=0, arm_count=0, seed=-1;
            int left=SW, right=-1;
            memset(arm,0,sizeof(arm));
            for(int y=12;y<116;y++) for(int x=0;x<SW;x++){
                int i=y*SW+x; u16 c=fb[i];
                if(zoom==256&&c&&(x<csCamX(115)||x>=csCamX(184)))
                    return patient_failure("patient extends beyond bed sides",zoom,pose,t);
                u16 h=x<neck_end&&warm_pixel(c)?c:0;
                if(!stirring&&!t) head[i]=h;
                else if(head[i]!=h)
                    return patient_failure("asleep head or neck moves",zoom,pose,t);
                if(h){
                    head_count++;
                    if(x<csCamX(136)&&y+1<116&&bedding_pixel(fb[i+SW])) support++;
                }
                if(x>=arm_start&&x<arm_end&&warm_pixel(c)&&(c&31)>=20){
                    arm[i]=1; arm_count++; if(seed<0) seed=i;
                    if(x<left) left=x; if(x>right) right=x;
                }
            }
            if(!head_count||!support)
                return patient_failure("head lacks contact with light bedding",zoom,pose,t);
            if(seed<0||right-left+1<6*zoom/256)
                return patient_failure("elbow-to-finger skin span is missing",zoom,pose,t);
            int read=0, count=1; queue[0]=seed; arm[seed]=2;
            while(read<count){
                int i=queue[read++], x=i%SW, y=i/SW;
                for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){
                    int nx=x+dx, ny=y+dy;
                    if(nx<0||nx>=SW||ny<12||ny>=116) continue;
                    int j=ny*SW+nx;
                    if(arm[j]==1){ arm[j]=2; queue[count++]=j; }
                }
            }
            if(count!=arm_count)
                return patient_failure("forearm or fingers have detached skin pixels",zoom,pose,t);
        }
    }
    printf("Patient checks passed: %d frames; zooms 256/560/1024; lying and stirring 0..180.\n"
           "Stationary asleep head/neck, bedding contact, connected forearm/fingers, "
           "bed width, framebuffer guards.\n",frames);
    return 0;
}
int main(int argc,char **argv){
    for(int i=0;i<4;i++) frame.before[i]=frame.after[i]=0x1234abcdu;
    if(argc==2&&!strcmp(argv[1],"--check-patient")) return check_patient();
    if(argc<5||argc>6){
        fputs("Usage: render_cutscene SCENE BEAT FRAME OUTPUT.ppm [FIRST_BEAT]\n"
              "       render_cutscene --check-patient\n"
              "Scene/beat/frame are zero-based. Default starts at BEAT.\n"
              "FIRST_BEAT replays preceding beats with automatic caption timing.\n",stderr);
        return 2;
    }
    int sc=integer(argv[1]), beat=integer(argv[2]), end=integer(argv[3]);
    int first=argc==6?integer(argv[5]):beat;
    if(sc>=(int)(sizeof(csScenes)/sizeof(csScenes[0]))||beat>=csScenes[sc].n||first>beat){
        fputs("Scene or beat out of range\n",stderr); return 2;
    }
    clipAll(); csCapB=0; csCamReset(); csAliveReset(); csRateSc=-1;
    csWholeOk=0; csCamKey=0; csCurSc=sc; csSlow=0;
    for(int bi=first;bi<=beat;bi++){
        const CsBeat *b=renderCaption(sc,bi,&csScenes[sc].b[bi]);
        csCurBi=bi;
        int total=csLayout(b), pages=(csNL+CS_PL-1)/CS_PL, pg=0, tp=0;
        int shown=0,rest=0;
        csPg=0; csTypingNow=0;
        for(int t=0;;t++){
            if(total){
                int ps=csPgS(pg), pe=csPgE(pg);
                if(shown<pe){ shown=ps+(t-tp)/2; if(shown>pe) shown=pe; rest=0; }
                else{
                    rest++;
                    int last=pg+1>=pages, au=last&&(b->fx&CF_AUTO);
                    int hold=au?(b->dur?b->dur:60):30+(pe-ps);
                    if(rest>hold){
                        if(last){ if(bi<beat) break; }
                        else { pg++; tp=t; shown=csPgS(pg); rest=0; }
                    }
                }
                csPg=pg; csTypingNow=shown<csPgE(pg);
            } else if(bi<beat&&t>=(b->dur?b->dur:60)) break;
            csDraw(b,t,shown);
            verify_frame();
            if(bi==beat&&t>=end) break;
            if(t>100000){ fputs("Beat failed to advance\n",stderr); return 1; }
        }
    }
    output(argv[4]);
    printf("scene=%d beat=%d frame=%d camera=%d/256 center=(%.2f,%.2f) output=%s\n",
           sc,beat,end,csCz,csCxF/16.0,csCyF/16.0,argv[4]);
    return 0;
}
