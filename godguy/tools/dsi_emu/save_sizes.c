#define PCTEST 1
#include <stdlib.h>
#include "/home/user/bore-gba/godguy/source/main.c"
static u32 rleEnc(const u8*in,u32 n,u8*out,u32 max){ u32 o=0,i=0; while(i<n){ if(o+140>max) return 0; u32 r=1; while(i+r<n&&in[i+r]==in[i]&&r<129) r++; if(r>=3){ out[o++]=(u8)(126+r); out[o++]=in[i]; i+=r; } else { u32 st=i; u32 l=0; while(i<n&&l<128){ u32 rr=1; while(i+rr<n&&in[i+rr]==in[i]&&rr<3) rr++; if(rr>=3) break; i++; l++; } out[o++]=(u8)(l-1); for(u32 k=0;k<l;k++) out[o++]=in[st+k]; } } return o; }
int main(int argc,char**argv){ int sz=atoi(argv[1]); int ticks=atoi(argv[2]); planetSize=sz; rs=99u; makeSpecies(); makeItems(); newWorld(); monOn=1; for(int t=1;t<=ticks;t++){ tick=t; year=t/1200; simulate(); }
  saveGame("/tmp/svsz.sav"); FILE*f=fopen("/tmp/svsz.sav","rb"); fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET); u8*b=malloc(n); fread(b,1,n,f); fclose(f); u8*o=malloc(n*2); u32 c=rleEnc(b,(u32)n,o,(u32)n*2);
  int al=0; for(int i=0;i<nCr;i++) al+=cr[i].alive; printf("%dx%d, %d creatures: raw %ld KB, packed %u KB\n",sz,sz,al,n/1024,c/1024); return 0; }
