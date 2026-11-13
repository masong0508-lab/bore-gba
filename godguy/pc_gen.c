// PC tool and test of the planet generator: land share, number of land masses, and a picture, for the choices of the create-a-planet screen
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int masses(int minsz,int*biggest){ static u8 seen[MAXD][MAXD]; memset(seen,0,sizeof seen); static int stk[MAXD*MAXD]; int n=0,big=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++){ if(seen[y][x]||world[y][x]<=T_WATER) continue; int sp=0,sz=0; stk[sp++]=y*MAXD+x; seen[y][x]=1; while(sp){ int c=stk[--sp]; int cx=c%MAXD, cy=c/MAXD; sz++; for(int d=0;d<4;d++){ int nx=cx+dxs[d], ny=cy+dys[d]; if(nx<0||ny<0||nx>=WW||ny>=WH||seen[ny][nx]||world[ny][nx]<=T_WATER) continue; seen[ny][nx]=1; stk[sp++]=ny*MAXD+nx; } } if(sz>=minsz){ n++; if(sz>big) big=sz; } } *biggest=big; return n; }
static void ppm(const char*fn){ FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n%d %d\n255\n",WW,WH); for(int y=0;y<WH;y++) for(int x=0;x<WW;x++){ int t=world[y][x]; unsigned char c[3]={(unsigned char)(terrCol[t][0]*8),(unsigned char)(terrCol[t][1]*8),(unsigned char)(terrCol[t][2]*8)}; fwrite(c,1,3,f); } fclose(f); }
static unsigned wh(void){ unsigned h=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) h=h*31+world[y][x]; return h; }
static int selftest(void){ int ok=1; makeSpecies();
    // the land masses follow the LAND choice, land share follows SEA, the same seed makes the same planet, a different seed another
    int nm[6]; for(int sh=0;sh<6;sh++){ int sum=0; for(unsigned sd=1;sd<=5;sd++){ gW=gH=256; planetSize=256; cShape=sh; cSea=0; cCoast=0; cMtn=0; genWorld(sd*977); int big; sum+=masses(256*256/400,&big); } nm[sh]=sum; }
    printf("land masses over 5 seeds for PANGAEA .. ISLANDS: %d %d %d %d %d %d\n",nm[0],nm[1],nm[2],nm[3],nm[4],nm[5]); if(!(nm[0]<=7&&nm[0]<nm[2]&&nm[2]<nm[4]&&nm[4]<nm[5])) ok=0;
    int lp[5]; for(int sea=-2;sea<=2;sea++){ cShape=2; cSea=sea; gW=gH=256; genWorld(4242); int l=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) l+=world[y][x]>T_WATER; lp[sea+2]=l*100/(256*256); }
    printf("land share for sea level DRY .. FLOODED: %d%% %d%% %d%% %d%% %d%%\n",lp[0],lp[1],lp[2],lp[3],lp[4]); if(!(lp[0]>lp[1]&&lp[1]>lp[2]&&lp[2]>lp[3]&&lp[3]>lp[4]&&lp[2]>=25&&lp[2]<=60)) ok=0;
    cSea=0; cShape=2; gW=gH=192; genWorld(99); unsigned a=wh(); genWorld(99); unsigned b=wh(); genWorld(100); unsigned c=wh(); printf("same seed same planet: %s, other seed other planet: %s\n",a==b?"YES":"NO",a!=c?"YES":"NO"); if(a!=b||a==c) ok=0;
    cCoast=-1; genWorld(99); unsigned r1=wh(); cCoast=1; genWorld(99); unsigned r2=wh(); if(r1==r2) ok=0; cCoast=0;
    printf("%s\n",ok?"GEN OK":"GEN WEAK"); return ok?0:1; }
int main(int argc,char**argv){ if(argc==1) return selftest();
    int sz=argc>1?atoi(argv[1]):128; cShape=argc>2?atoi(argv[2]):2; cSea=argc>3?atoi(argv[3]):0; cCoast=argc>4?atoi(argv[4]):0; cMtn=argc>5?atoi(argv[5]):0; unsigned seed=argc>6?(unsigned)atoi(argv[6]):1234; const char*out=argc>7?argv[7]:0;
    gW=gH=sz; planetSize=sz; makeSpecies(); genWorld(seed); int land=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) land+=world[y][x]>T_WATER; int big; int n=masses(sz*sz/400,&big);
    printf("size %d shape %d sea %+d coast %+d mtn %+d seed %u: land %d%%, %d land masses over %d tiles, biggest %d%% of the land\n",sz,cShape,cSea,cCoast,cMtn,seed,land*100/(sz*sz),n,sz*sz/400,land?big*100/land:0); if(out) ppm(out); return 0; }
