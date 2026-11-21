// The hardware DS save writer previously held a 700-KiB raw world in
// ARM9 BSS AND a 512-KiB compressed world above the ARM9 stack. Encode
// incrementally instead: one 4-KiB input window and one 512-KiB compressed
// buffer in BSS. The high 0x02300000..0x02380000 region is now reserved
// for the SECOND half of the complete DS vocal bank. The format is the
// original RLE byte stream, including compatibility with older saves.
#ifndef GG_SAVE_STREAM_H
#define GG_SAVE_STREAM_H
#define GG_SV_CHUNK 4096u
typedef struct {
    u32 raw,used,packed;
    int bad;
    u8 window[GG_SV_CHUNK];
} GgSaveWriter;
typedef struct {
    const u8 *in;
    u32 packed,raw,ip,op;
    u32 left;
    u8 repeat,literal;
} GgSaveReader;
static int ggSvPack(GgSaveWriter *w,u8 *out,u32 max){
    u32 i=0,o=w->packed,n=w->used;
    while(i<n){
        u32 run=1;
        while(i+run<n&&w->window[i+run]==w->window[i]&&run<129)
            run++;
        if(run>=3){
            if(o+2>max)return w->bad=1,0;
            out[o++]=(u8)(126+run);
            out[o++]=w->window[i]; i+=run;
        }else{
            u32 start=i,len=0;
            while(i<n&&len<128){
                u32 r=1;
                while(i+r<n&&w->window[i+r]==w->window[i]&&r<3)r++;
                if(r>=3)break;
                i++;len++;
            }
            if(o+1+len>max)return w->bad=1,0;
            out[o++]=(u8)(len-1);
            for(u32 j=0;j<len;j++)out[o++]=w->window[start+j];
        }
    }
    w->packed=o; w->used=0;
    return 1;
}
static int ggSvWrite(GgSaveWriter*w,u8*out,u32 max,
                     const void*data,u32 len,u32 rawMax){
    if(w->bad||len>rawMax-w->raw)return w->bad=1,0;
    const u8*p=(const u8*)data;
    u32 n=len;
    while(n){
        u32 take=GG_SV_CHUNK-w->used;
        if(take>n)take=n;
        for(u32 j=0;j<take;j++)w->window[w->used+j]=p[j];
        w->used+=take;w->raw+=take;p+=take;n-=take;
        if(w->used==GG_SV_CHUNK&&!ggSvPack(w,out,max))return 0;
    }
    return 1;
}
static int ggSvValidate(const u8 *data,u32 packed,u32 rawMax,u32 raw){
    u32 ip=0,total=0;
    if(raw>rawMax)return 0;
    while(ip<packed){
        u8 c=data[ip++];
        u32 len;
        if(c<128){
            len=(u32)c+1;
            if(len>packed-ip)return 0;
            ip+=len;
        }else{
            len=(u32)c-126;
            if(ip>=packed)return 0;
            ip++;
        }
        if(len>raw-total)return 0;
        total+=len;
    }
    return total==raw;
}
static void ggSvReaderStart(GgSaveReader*r,const u8*data,
                            u32 packed,u32 raw){
    r->in=data;r->packed=packed;r->raw=raw;
    r->ip=r->op=r->left=0;
    r->repeat=r->literal=0;
}
static int ggSvRead(GgSaveReader*r,void*output,u32 n){
    if(n>r->raw-r->op)return 0;
    u8*out=(u8*)output;
    while(n){
        if(!r->left){
            if(r->ip>=r->packed)return 0;
            u8 c=r->in[r->ip++];
            if(c<128){
                r->left=(u32)c+1;
                r->literal=1;
                if(r->left>r->packed-r->ip)return 0;
            }else{
                r->left=(u32)c-126;
                r->literal=0;
                if(r->ip>=r->packed)return 0;
                r->repeat=r->in[r->ip++];
            }
        }
        u32 take=r->left<n?r->left:n;
        if(r->literal){
            for(u32 i=0;i<take;i++)out[i]=r->in[r->ip+i];
            r->ip+=take;
        }else for(u32 i=0;i<take;i++)out[i]=r->repeat;
        r->left-=take;r->op+=take;out+=take;n-=take;
    }
    return 1;
}
#endif
