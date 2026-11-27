#include <stdint.h>
#include <stddef.h>
#include <limits.h>
static inline int pred(int f,int a,int b){
    switch(f){case 1:return a+((-a)>>4);case 2:return a*2+((-3*a)>>5)-b+(b>>4);case 3:return a*2+((-13*a)>>6)-b+((3*b)>>4);default:return 0;}
}
static inline int quant(int v,int s){int d=1<<(s-1);int q=v>=0?(v+d/2)/d:-((-v+d/2)/d);return q<-8?-8:q>7?7:q;}
// Input samples are signed 15-bit (half of PCM16). All reconstructed samples
// are kept in the valid 15-bit range to avoid hardware wrap-around.
void encode_brr(const int16_t *src,int n,uint8_t *out){
 int a=0,b=0;
 for(int i=0;i<n;i+=16){
   uint64_t best=UINT64_MAX;int bs=12,bf=0,ba=0,bb=0;uint8_t bn[16]={0};
   for(int f=0;f<(i?4:1);f++)for(int s=1;s<=12;s++){
     int p=a,q=b,ok=1;uint64_t err=0;uint8_t nn[16];
     for(int j=0;j<16;j++){
       int x=src[i+j],pr=pred(f,p,q),v=quant(x-pr,s),y=pr+v*(1<<(s-1));
       if(y<-16384||y>16383){ok=0;break;}
       int e=y-x;err+=(uint64_t)((int64_t)e*e);if(err>=best){ok=0;break;}
       nn[j]=v&15;q=p;p=y;
     }
     if(ok&&err<best){best=err;bs=s;bf=f;ba=p;bb=q;for(int j=0;j<16;j++)bn[j]=nn[j];}
   }
   a=ba;b=bb;*out++=(bs<<4)|(bf<<2)|(i+16==n?1:0);
   for(int j=0;j<16;j+=2)*out++=(bn[j]<<4)|bn[j+1];
 }
}
