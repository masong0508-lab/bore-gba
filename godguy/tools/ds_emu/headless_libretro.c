// Minimal deterministic libretro frontend for actual GodGuy NDS ROM QA.
// stdin: run FRAMES LABEL BUTTON_MASK TOUCH_X TOUCH_Y | save LABEL | load LABEL | quit
// Touch coordinates are native bottom-screen pixels; -1 disables touch.
#define _POSIX_C_SOURCE 200809L
#include "libretro.h"
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define API(ret,name,args) static ret (*core_##name)args
API(void,set_environment,(retro_environment_t));
API(void,set_video_refresh,(retro_video_refresh_t));
API(void,set_audio_sample,(retro_audio_sample_t));
API(void,set_audio_sample_batch,(retro_audio_sample_batch_t));
API(void,set_input_poll,(retro_input_poll_t));
API(void,set_input_state,(retro_input_state_t));
API(void,init,(void)); API(void,deinit,(void));
API(void,get_system_info,(struct retro_system_info*));
API(void,get_system_av_info,(struct retro_system_av_info*));
API(bool,load_game,(const struct retro_game_info*));
API(void,unload_game,(void)); API(void,run,(void));
API(size_t,serialize_size,(void));
API(bool,serialize,(void*,size_t));
API(bool,unserialize,(const void*,size_t));

static char output_dir[4096];
static char session_prefix[129];
static unsigned pixel_format=RETRO_PIXEL_FORMAT_RGB565;
static unsigned width,height;
static uint8_t *last_video;
static size_t last_video_size;
static uint16_t buttons;
static int touch_x=-1,touch_y=-1;
static uint64_t video_frames,video_changes,last_hash,audio_frames,audio_nonzero;
static long double audio_energy;
static int audio_peak;
static FILE *wave_file,*report_file,*status_file;
static unsigned sample_rate;
static bool shutdown_requested;
static struct {const char *key;char *value;} variables[256];
static unsigned variable_count;

static double seconds(void){
    struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);
    return ts.tv_sec+ts.tv_nsec/1e9;
}
static void core_log(enum retro_log_level level,const char *format,...){
    if(level<RETRO_LOG_INFO)return;
    va_list ap;va_start(ap,format);vfprintf(stderr,format,ap);va_end(ap);
}
static const char *option(const char *key){
    if(!strcmp(key,"desmume_cpu_mode")){
        const char *mode=getenv("GG_EMU_CPU");
        if(mode)return mode;
    }
    static const char *const override[][2]={
        {"desmume_cpu_mode","jit"},
        {"desmume_use_external_bios","disabled"},
        {"desmume_boot_into_bios","disabled"},
        {"desmume_opengl_mode","disabled"},
        {"desmume_internal_resolution","256x192"},
        {"desmume_num_cores","1"},
        {"desmume_screens_layout","top/bottom"},
        {"desmume_screens_gap","0"},
        {"desmume_pointer_mouse","enabled"},
        {"desmume_pointer_type","touch"},
        {"desmume_frameskip","0"}
    };
    for(size_t i=0;i<sizeof override/sizeof override[0];i++)
        if(!strcmp(key,override[i][0]))return override[i][1];
    for(unsigned i=0;i<variable_count;i++)
        if(!strcmp(key,variables[i].key))return variables[i].value;
    return NULL;
}
static bool environment(unsigned command,void *data){
    switch(command){
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((struct retro_log_callback*)data)->log=core_log;return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char**)data=output_dir;return true;
    case RETRO_ENVIRONMENT_GET_USERNAME:
        *(const char**)data="GodGuy QA";return true;
    case RETRO_ENVIRONMENT_GET_LANGUAGE:
        *(unsigned*)data=RETRO_LANGUAGE_ENGLISH;return true;
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        *(unsigned*)data=0;return true;
    case RETRO_ENVIRONMENT_SET_VARIABLES:{
        const struct retro_variable *v=data;
        for(;v&&v->key&&variable_count<256;v++){
            const char *start=strchr(v->value,';');
            if(!start)continue;
            do{start++;}while(*start==' ');
            size_t len=strcspn(start,"|");
            variables[variable_count].key=v->key;
            variables[variable_count].value=strndup(start,len);
            variable_count++;
        }
        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE:{
        struct retro_variable *v=data;v->value=option(v->key);
        return v->value!=NULL;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *(bool*)data=false;return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        pixel_format=*(unsigned*)data;return pixel_format<=RETRO_PIXEL_FORMAT_RGB565;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool*)data=true;return true;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:return true;
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:return true;
    case RETRO_ENVIRONMENT_SHUTDOWN:
        shutdown_requested=true;return true;
    default:return false;
    }
}
static void video(const void *pixels,unsigned w,unsigned h,size_t pitch){
    video_frames++;
    if(!pixels)return;
    if(pixels==RETRO_HW_FRAME_BUFFER_VALID){
        fprintf(stderr,"Unexpected hardware framebuffer\n");exit(10);
    }
    unsigned bytes=pixel_format==RETRO_PIXEL_FORMAT_XRGB8888?4:2;
    size_t size=(size_t)w*h*bytes;
    if(size>32u*1024u*1024u){fprintf(stderr,"Oversized video frame\n");exit(11);}
    if(size!=last_video_size){
        void *next=realloc(last_video,size);
        if(!next)exit(12);
        last_video=next;last_video_size=size;
    }
    width=w;height=h;
    uint64_t hash=UINT64_C(1469598103934665603);
    for(unsigned y=0;y<h;y++){
        const uint8_t *src=(const uint8_t*)pixels+y*pitch;
        memcpy(last_video+(size_t)y*w*bytes,src,(size_t)w*bytes);
        for(unsigned x=0;x<w*bytes;x++)hash=(hash^src[x])*UINT64_C(1099511628211);
    }
    if(hash!=last_hash)video_changes++;
    last_hash=hash;
}
static size_t audio(const int16_t *samples,size_t frames){
    audio_frames+=frames;
    for(size_t i=0;i<frames*2;i++){
        int s=samples[i];int absolute=s<0?-s:s;
        audio_nonzero+=s!=0;
        if(absolute>audio_peak)audio_peak=absolute;
        audio_energy+=(long double)s*s;
    }
    if(wave_file)fwrite(samples,sizeof *samples,frames*2,wave_file);
    return frames;
}
static void audio_one(int16_t left,int16_t right){int16_t s[2]={left,right};audio(s,1);}
static void poll(void){}
static int16_t input(unsigned port,unsigned device,unsigned index,unsigned id){
    (void)index;if(port)return 0;
    if(device==RETRO_DEVICE_JOYPAD){
        if(id==RETRO_DEVICE_ID_JOYPAD_MASK)return (int16_t)buttons;
        return id<16?((buttons>>id)&1):0;
    }
    if(device==RETRO_DEVICE_POINTER&&touch_x>=0&&touch_y>=0){
        if(id==RETRO_DEVICE_ID_POINTER_PRESSED)return 1;
        if(id==RETRO_DEVICE_ID_POINTER_X)
            return (int16_t)(((touch_x*2+1)*65536)/(256*2)-32768);
        if(id==RETRO_DEVICE_ID_POINTER_Y)
            return (int16_t)((((touch_y+192)*2+1)*65536)/(384*2)-32768);
    }
    return 0;
}
static bool valid_label(const char *text){
    if(!*text)return false;
    for(;*text;text++)if(!((*text>='a'&&*text<='z')||
        (*text>='A'&&*text<='Z')||(*text>='0'&&*text<='9')||
        *text=='_'||*text=='-'))return false;
    return true;
}
static void write_image(const char *label){
    if(!last_video)return;
    char path[8192];snprintf(path,sizeof path,"%s/%s%s.ppm",output_dir,session_prefix,label);
    FILE *f=fopen(path,"wb");if(!f){perror(path);exit(13);}
    fprintf(f,"P6\n%u %u\n255\n",width,height);
    for(size_t i=0;i<(size_t)width*height;i++){
        uint8_t rgb[3];
        if(pixel_format==RETRO_PIXEL_FORMAT_XRGB8888){
            uint32_t v;memcpy(&v,last_video+i*4,4);
            rgb[0]=v>>16;rgb[1]=v>>8;rgb[2]=v;
        }else{
            uint16_t v;memcpy(&v,last_video+i*2,2);
            if(pixel_format==RETRO_PIXEL_FORMAT_RGB565){
                rgb[0]=(v>>11)*255/31;rgb[1]=((v>>5)&63)*255/63;
            }else{
                rgb[0]=((v>>10)&31)*255/31;rgb[1]=((v>>5)&31)*255/31;
            }
            rgb[2]=(v&31)*255/31;
        }
        fwrite(rgb,1,3,f);
    }
    fclose(f);
}
static void put16(FILE*f,unsigned v){fputc(v&255,f);fputc((v>>8)&255,f);}
static void put32(FILE*f,unsigned v){put16(f,v&65535);put16(f,v>>16);}
static void wave_header(void){
    unsigned bytes=(unsigned)(audio_frames*4);
    rewind(wave_file);fwrite("RIFF",1,4,wave_file);put32(wave_file,36+bytes);
    fwrite("WAVEfmt ",1,8,wave_file);put32(wave_file,16);put16(wave_file,1);
    put16(wave_file,2);put32(wave_file,sample_rate);put32(wave_file,sample_rate*4);
    put16(wave_file,4);put16(wave_file,16);fwrite("data",1,4,wave_file);put32(wave_file,bytes);
}
static void state_command(bool save,const char *label){
    char path[8192];snprintf(path,sizeof path,"%s/%s.state",output_dir,label);
    size_t size=core_serialize_size();void *data=malloc(size);bool ok=false;
    if(data){
        FILE *f=fopen(path,save?"wb":"rb");
        if(f){
            if(save)ok=core_serialize(data,size)&&fwrite(data,1,size,f)==size;
            else ok=fread(data,1,size,f)==size&&core_unserialize(data,size);
            fclose(f);
        }
        free(data);
    }
    fprintf(status_file,"STATE %s %s %zu %s\n",save?"save":"load",label,size,ok?"ok":"FAIL");fflush(status_file);
}
int main(int argc,char **argv){
    if(argc!=4){fprintf(stderr,"usage: %s CORE.so ROM.nds OUTPUT_DIR\n",argv[0]);return 2;}
    if(strlen(argv[3])>=sizeof output_dir)return 3;
    strcpy(output_dir,argv[3]);
    const char *session=getenv("GG_EMU_SESSION");
    if(session){
        if(strlen(session)>127||!valid_label(session)){
            fprintf(stderr,"GG_EMU_SESSION must be a short alphanumeric label\n");return 3;
        }
        snprintf(session_prefix,sizeof session_prefix,"%s_",session);
    }
    if(mkdir(output_dir,0755)&&errno!=EEXIST){perror(output_dir);return 3;}
    status_file=fdopen(dup(STDOUT_FILENO),"w");
    if(!status_file)return 3;
    char log_path[8192];snprintf(log_path,sizeof log_path,"%s/%semulator.log",output_dir,session_prefix);
    if(!freopen(log_path,"w",stdout))return 3;
    dup2(fileno(stdout),STDERR_FILENO);setvbuf(stdout,NULL,_IOLBF,0);
    void *library=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if(!library){fprintf(stderr,"dlopen: %s\n",dlerror());return 4;}
#define LOAD(name) do{*(void**)(&core_##name)=dlsym(library,"retro_"#name);if(!core_##name){fprintf(stderr,"Missing retro_%s\n",#name);return 5;}}while(0)
    LOAD(set_environment);LOAD(set_video_refresh);LOAD(set_audio_sample);
    LOAD(set_audio_sample_batch);LOAD(set_input_poll);LOAD(set_input_state);
    LOAD(init);LOAD(deinit);LOAD(get_system_info);LOAD(get_system_av_info);
    LOAD(load_game);LOAD(unload_game);LOAD(run);LOAD(serialize_size);
    LOAD(serialize);LOAD(unserialize);
    core_set_environment(environment);core_set_video_refresh(video);
    core_set_audio_sample(audio_one);core_set_audio_sample_batch(audio);
    core_set_input_poll(poll);core_set_input_state(input);
    struct retro_system_info info={0};core_get_system_info(&info);
    fprintf(status_file,"CORE %s %s\n",info.library_name,info.library_version);fflush(status_file);
    core_init();
    struct retro_system_av_info av={0};core_get_system_av_info(&av);
    sample_rate=(unsigned)av.timing.sample_rate;
    char path[8192];snprintf(path,sizeof path,"%s/%saudio.wav",output_dir,session_prefix);
    wave_file=fopen(path,"wb");if(!wave_file)return 6;wave_header();
    snprintf(path,sizeof path,"%s/%ssteps.jsonl",output_dir,session_prefix);report_file=fopen(path,"w");if(!report_file){perror(path);return 6;}
    struct retro_game_info game={argv[2],NULL,0,NULL};
    if(!core_load_game(&game)){fprintf(stderr,"ROM load failed\n");return 7;}
    fprintf(status_file,"READY %.6f fps %u Hz\n",av.timing.fps,sample_rate);fflush(status_file);
    char line[512];
    while(!shutdown_requested&&fgets(line,sizeof line,stdin)){
        char verb[32]={0},label[128]={0};unsigned frames=0,mask=0;int x=-1,y=-1;
        if(sscanf(line,"%31s",verb)!=1)continue;
        if(!strcmp(verb,"quit"))break;
        if(!strcmp(verb,"save")||!strcmp(verb,"load")){
            if(sscanf(line,"%31s %127s",verb,label)==2&&valid_label(label))state_command(!strcmp(verb,"save"),label);
            continue;
        }
        if(strcmp(verb,"run")||sscanf(line,"%31s %u %127s %x %d %d",verb,&frames,label,&mask,&x,&y)<3){
            fprintf(stderr,"Expected run FRAMES LABEL MASK X Y\n");continue;
        }
        if(!valid_label(label)||mask>65535||x>255||y>191||x< -1||y< -1){
            fprintf(status_file,"Invalid label, button mask, or touch coordinate\n");fflush(status_file);continue;
        }
        buttons=(uint16_t)mask;touch_x=x;touch_y=y;
        uint64_t start_video=video_frames,start_changes=video_changes,start_audio=audio_frames,start_nonzero=audio_nonzero;
        long double start_energy=audio_energy;audio_peak=0;
        double start=seconds();
        for(unsigned i=0;i<frames&&!shutdown_requested;i++)core_run();
        double elapsed=seconds()-start;buttons=0;touch_x=touch_y=-1;
        write_image(label);
        uint64_t count=audio_frames-start_audio;
        double rms=count?sqrt((double)((audio_energy-start_energy)/(count*2))):0;
        fprintf(report_file,"{\"label\":\"%s\",\"requested_frames\":%u,\"buttons\":%u,\"touch_x\":%d,\"touch_y\":%d,\"video_frames\":%"PRIu64",\"changed_frames\":%"PRIu64",\"video_hash\":\"%016"PRIx64"\",\"width\":%u,\"height\":%u,\"audio_frames\":%"PRIu64",\"nonzero_audio_samples\":%"PRIu64",\"audio_peak\":%d,\"audio_rms\":%.3f,\"wall_seconds\":%.3f}\n",label,frames,mask,x,y,video_frames-start_video,video_changes-start_changes,last_hash,width,height,count,audio_nonzero-start_nonzero,audio_peak,rms,elapsed);fflush(report_file);
        fprintf(status_file,"CAPTURE %s %ux%u changed=%"PRIu64"/%"PRIu64" audio_nonzero=%"PRIu64" peak=%d rms=%.1f wall=%.2fs\n",label,width,height,video_changes-start_changes,video_frames-start_video,audio_nonzero-start_nonzero,audio_peak,rms,elapsed);fflush(status_file);
    }
    core_unload_game();core_deinit();
    wave_header();fclose(wave_file);if(report_file)fclose(report_file);
    free(last_video);for(unsigned i=0;i<variable_count;i++)free(variables[i].value);
    dlclose(library);fclose(status_file);return 0;
}
