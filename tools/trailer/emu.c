// headless mGBA runner: emu rom.gba script.txt
// script commands (one per line, # = comment):
//   run N            run N frames            hold KEYS N   hold keys (A+B+START...) N frames, then release
//   tap KEYS         hold 3 frames, release 3          wait N   = run N
//   shot file.ppm    screenshot                          wav file.wav / wavstop   capture audio (32768 Hz stereo s16)
//   poke32 ADDR VAL  write a word through the bus        peek32 ADDR  print a word      peek8 ADDR
#include <mgba/core/core.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static struct mCore* core; static color_t* vbuf; static unsigned vw, vh;
static FILE* wav; static uint32_t wavBytes; static int16_t* abuf; static int aCap;
static const char* keyNames[]={"A","B","SELECT","START","RIGHT","LEFT","UP","DOWN","R","L",0};
static uint32_t parseKeys(char* s){ uint32_t k=0; for(char* t=strtok(s,"+");t;t=strtok(0,"+")){ for(int i=0;keyNames[i];i++) if(!strcmp(t,keyNames[i])) k|=1u<<i; } return k; }
static void wavHeader(FILE* f,uint32_t bytes){ uint32_t sr=32768, br=sr*4; uint16_t ch=2, ba=4, bps=16, fmt=1; uint32_t sz=36+bytes, fs=16;
    fseek(f,0,SEEK_SET); fwrite("RIFF",1,4,f); fwrite(&sz,4,1,f); fwrite("WAVEfmt ",1,8,f); fwrite(&fs,4,1,f); fwrite(&fmt,2,1,f); fwrite(&ch,2,1,f);
    fwrite(&sr,4,1,f); fwrite(&br,4,1,f); fwrite(&ba,2,1,f); fwrite(&bps,2,1,f); fwrite("data",1,4,f); fwrite(&bytes,4,1,f); }
static void frame(void){
    core->runFrame(core);
    struct blip_t* l=core->getAudioChannel(core,0); struct blip_t* r=core->getAudioChannel(core,1);
    int n=blip_samples_avail(l);
    if(n>aCap){ aCap=n*2; abuf=realloc(abuf,aCap*4); }
    if(n>0){ blip_read_samples(l,abuf,n,1); blip_read_samples(r,abuf+1,n,1); if(wav){ fwrite(abuf,4,n,wav); wavBytes+=n*4; } }
}
static FILE* recF; static void recFrame(void){ const void* px; size_t stride; core->getPixels(core,&px,&stride); static uint8_t row[240*3];
    for(unsigned y=0;y<vh;y++){ for(unsigned x=0;x<vw;x++){ uint32_t c=((const uint32_t*)px)[y*stride+x]; row[x*3]=c&0xFF; row[x*3+1]=(c>>8)&0xFF; row[x*3+2]=(c>>16)&0xFF; } fwrite(row,1,vw*3,recF); } }
static void run(int n){ while(n-->0){ frame(); if(recF) recFrame(); } }
static void shot(const char* path){
    const void* px; size_t stride; core->getPixels(core,&px,&stride);
    FILE* f=fopen(path,"wb"); fprintf(f,"P6\n%u %u\n255\n",vw,vh);
    for(unsigned y=0;y<vh;y++) for(unsigned x=0;x<vw;x++){ uint32_t c=((const uint32_t*)px)[y*stride+x]; uint8_t rgb[3]={c&0xFF,(c>>8)&0xFF,(c>>16)&0xFF}; fwrite(rgb,1,3,f); }
    fclose(f);
}
static void nullLog(struct mLogger* l,int cat,enum mLogLevel lv,const char* fmt,va_list ap){ (void)l;(void)cat;(void)lv;(void)fmt;(void)ap; }
int main(int argc,char** argv){
    static struct mLogger logger={.log=nullLog}; mLogSetDefaultLogger(&logger);
    if(argc<3){ fprintf(stderr,"usage: emu rom script\n"); return 1; }
    core=mCoreFind(argv[1]); if(!core){ fprintf(stderr,"no core\n"); return 1; }
    core->init(core); mCoreInitConfig(core,NULL);
    core->setAudioBufferSize(core,4096);
    core->desiredVideoDimensions(core,&vw,&vh); vbuf=malloc(vw*vh*4); core->setVideoBuffer(core,vbuf,vw);
    if(!mCoreLoadFile(core,argv[1])){ fprintf(stderr,"load failed\n"); return 1; }
    blip_set_rates(core->getAudioChannel(core,0),core->frequency(core),32768); blip_set_rates(core->getAudioChannel(core,1),core->frequency(core),32768);
    if(argc>3){ struct VFile* sv=VFileOpen(argv[3],O_RDWR|O_CREAT); if(!sv||!core->loadSave(core,sv)){ fprintf(stderr,"save file failed\n"); return 1; } }   // optional: a .sav file, kept up to date
    core->reset(core);
    FILE* sc=fopen(argv[2],"r"); char line[512];
    while(fgets(line,sizeof line,sc)){
        char* p=line; while(*p==' ') p++; if(*p=='#'||*p=='\n'||!*p) continue; p[strcspn(p,"\r\n")]=0;
        char cmd[32]={0}, a1[256]={0}, a2[64]={0}; sscanf(p,"%31s %255s %63s",cmd,a1,a2);
        if(!strcmp(cmd,"run")||!strcmp(cmd,"wait")) run(atoi(a1));
        else if(!strcmp(cmd,"hold")){ core->setKeys(core,parseKeys(a1)); run(atoi(a2)); core->setKeys(core,0); }
        else if(!strcmp(cmd,"tap")){ core->setKeys(core,parseKeys(a1)); run(10); core->setKeys(core,0); run(10); }
        else if(!strcmp(cmd,"shot")) shot(a1);
        else if(!strcmp(cmd,"rec")){ if(recF) fclose(recF); recF=fopen(a1,"wb"); }   // rec file.rgb: every frame from now as raw RGB24 240x160 (recstop ends it)
        else if(!strcmp(cmd,"recstop")){ if(recF){ fclose(recF); recF=0; } }
        else if(!strcmp(cmd,"wav")){ wav=fopen(a1,"wb"); wavBytes=0; wavHeader(wav,0); }
        else if(!strcmp(cmd,"wavstop")){ if(wav){ wavHeader(wav,wavBytes); fclose(wav); wav=0; } }
        else if(!strcmp(cmd,"poke8")) core->busWrite8(core,strtoul(a1,0,0),strtoul(a2,0,0));
        else if(!strcmp(cmd,"poke32")) core->busWrite32(core,strtoul(a1,0,0),strtoul(a2,0,0));
        else if(!strcmp(cmd,"peek32")) printf("%s = 0x%08x\n",a1,core->busRead32(core,strtoul(a1,0,0)));
        else if(!strcmp(cmd,"dump32")){ uint32_t a=strtoul(a1,0,0); int n=atoi(a2); for(int i=0;i<n;i++) printf("%08x%c",core->busRead32(core,a+4*i),i==n-1?'\n':' '); }
        else if(!strcmp(cmd,"reset")) core->reset(core);
        else if(!strcmp(cmd,"echo")) printf("--- %s\n",p+5);
        else if(!strcmp(cmd,"keys")) core->setKeys(core,parseKeys(a1));   // keys KEYS: held until the next keys / hold (keys NONE: none)
        else if(!strcmp(cmd,"proft3")){   // proft3 N file: like proft, on timer 3 (the cutscenes' clock: 1024 cycles a tick)
            int n=atoi(a1); static uint32_t* h; static uint32_t* hp; if(!h){ h=calloc(1<<20,4); hp=calloc(1<<20,4); }
            for(int i=0;i<(1<<20);i++) h[i]=0;
            uint16_t last=core->busRead16(core,0x0400010C);
            for(int fr=0;fr<n;fr++){ uint32_t f0=core->frameCounter(core);
                while(core->frameCounter(core)==f0){ core->step(core); uint16_t t=core->busRead16(core,0x0400010C); if(t!=last){ uint32_t pc=0; core->readRegister(core,"pc",&pc); h[(pc>>1)&((1<<20)-1)]+=(uint16_t)(t-last); hp[(pc>>1)&((1<<20)-1)]=pc; last=t; } } }
            FILE*f=fopen(a2,"w"); for(int i=0;i<(1<<20);i++) if(h[i]) fprintf(f,"%08x %u\n",hp[i],h[i]); fclose(f); }
        else if(!strcmp(cmd,"proflr")||!strcmp(cmd,"proflr3")){   // proflr N file LO-HI: time-weighted callers (lr) of the code in LO..HI (hex); proflr3: on timer 3
            int n=atoi(a1); char fn[256]={0}, rg[64]={0}; sscanf(p,"%*s %*s %255s %63s",fn,rg); uint32_t lo=0,hi=0; sscanf(rg,"%x-%x",&lo,&hi);
            static uint32_t* h; static uint32_t* hp; if(!h){ h=calloc(1<<20,4); hp=calloc(1<<20,4); }
            for(int i=0;i<(1<<20);i++) h[i]=0;
            uint16_t last=core->busRead16(core,cmd[6]=='3'?0x0400010C:0x04000108);
            for(int fr=0;fr<n;fr++){ uint32_t f0=core->frameCounter(core);
                while(core->frameCounter(core)==f0){ core->step(core); uint16_t t=core->busRead16(core,cmd[6]=='3'?0x0400010C:0x04000108); if(t!=last){ uint32_t pc=0, lr=0; core->readRegister(core,"pc",&pc);
                    if(pc>=lo&&pc<hi){ core->readRegister(core,"lr",&lr); h[(lr>>1)&((1<<20)-1)]+=(uint16_t)(t-last); hp[(lr>>1)&((1<<20)-1)]=lr; } last=t; } } }
            FILE*f=fopen(fn,"w"); for(int i=0;i<(1<<20);i++) if(h[i]) fprintf(f,"%08x %u\n",hp[i],h[i]); fclose(f); }
        else if(!strcmp(cmd,"proft")){   // proft N file: like prof, but time-weighted: a sample each time timer 2 (256 cycles a tick in the life game) ticks
            int n=atoi(a1); static uint32_t* h; static uint32_t* hp; if(!h){ h=calloc(1<<20,4); hp=calloc(1<<20,4); }
            for(int i=0;i<(1<<20);i++) h[i]=0;
            uint16_t last=core->busRead16(core,0x04000108);
            for(int fr=0;fr<n;fr++){ uint32_t f0=core->frameCounter(core);
                while(core->frameCounter(core)==f0){ core->step(core); uint16_t t=core->busRead16(core,0x04000108); if(t!=last){ uint32_t pc=0; core->readRegister(core,"pc",&pc); h[(pc>>1)&((1<<20)-1)]+=(uint16_t)(t-last); hp[(pc>>1)&((1<<20)-1)]=pc; last=t; } } }
            FILE*f=fopen(a2,"w"); for(int i=0;i<(1<<20);i++) if(h[i]) fprintf(f,"%08x %u\n",hp[i],h[i]); fclose(f); }
        else if(!strcmp(cmd,"prof")){   // prof N file: run N frames one instruction at a time, sampling the PC every 8 steps; a histogram "pc count" per line
            int n=atoi(a1); static uint32_t* h; static uint32_t* hp; if(!h){ h=calloc(1<<20,4); hp=calloc(1<<20,4); }
            for(int i=0;i<(1<<20);i++) h[i]=0;
            for(int fr=0;fr<n;fr++){ uint32_t f0=core->frameCounter(core); unsigned c=0;
                while(core->frameCounter(core)==f0){ core->step(core); if(((++c)&7)==0){ uint32_t pc=0; core->readRegister(core,"pc",&pc); h[(pc>>1)&((1<<20)-1)]++; hp[(pc>>1)&((1<<20)-1)]=pc; } } }
            FILE*f=fopen(a2,"w"); for(int i=0;i<(1<<20);i++) if(h[i]) fprintf(f,"%08x %u\n",hp[i],h[i]); fclose(f); }
        else if(!strcmp(cmd,"dumpmem")){ uint32_t a=strtoul(a1,0,0); int n=atoi(a2); char fn[256]={0}; sscanf(p,"%*s %*s %*s %255s",fn); FILE*f=fopen(fn,"wb"); for(int i=0;i<n;i++){ uint8_t b=core->busRead8(core,a+i); fwrite(&b,1,1,f);} fclose(f); }
        else if(!strcmp(cmd,"peek8")) printf("%s = 0x%02x\n",a1,core->busRead8(core,strtoul(a1,0,0)));
        else fprintf(stderr,"unknown command: %s\n",cmd);
    }
    if(wav){ wavHeader(wav,wavBytes); fclose(wav); }
    core->deinit(core); return 0;
}
