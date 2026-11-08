// PC test harness for GODGUY's world and ecosystem: runs the same code as the DS build, prints the populations over time and draws pictures.
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static void ppm(const char*fn,int scale,int x0,int y0,int w,int h,int sprites){
    FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n%d %d\n255\n",w*8*scale/ (sprites?1:8)*1,h*8*scale/(sprites?1:8)); fclose(f); }
int main(int argc,char**argv){
    unsigned seed=argc>1?atoi(argv[1]):7; int years=argc>2?atoi(argv[2]):6; rs=seed*2654435761u+1; makeSpecies(); newWorld();
    int div[5];
    for(int t=0;t<years*2048;t++){ tick=t; simulate(); if(t%1024==0){ int alive=0; for(int d=0;d<5;d++) div[d]=0; for(int i=0;i<nCr;i++) if(cr[i].alive){ alive++; div[arch[sp[cr[i].sp].a].diet]++; }
            { int hh; int pp=vilTotals(&hh); int wars=0; for(int v=0;v<VIL_N;v++) wars+=vil[v].on&&vil[v].foe; printf("t=%5d alive=%4d herb=%4d carn=%3d omni=%3d fish=%3d civ=%3d | villages=%2d folk=%3d homes=%3d wars=%d\n",t,alive,div[0],div[1],div[2],div[3],div[4],nVilOn,pp,hh,wars); } } }
    // picture: the world 4 pixels a tile, creatures as dots; then a sprite crop
    int S=4; FILE*f=fopen("/tmp/gg_world.ppm","wb"); fprintf(f,"P6\n%d %d\n255\n",WW*S,WH*S);
    for(int y=0;y<WH*S;y++) for(int x=0;x<WW*S;x++){ int tx=x/S,ty=y/S; int r,g,b; shadeRGB(world[ty][tx]*8+4,&r,&g,&b); u8 px[3]={(u8)(r*8),(u8)(g*8),(u8)(b*8)};
        if(occ[ty][tx]&&(x%S>0)&&(y%S>0)){ const Cr*c=&cr[occ[ty][tx]-1]; int fm=sp[c->sp].fam; px[0]=(u8)(famCol[fm][0][0]*8); px[1]=(u8)(famCol[fm][0][1]*8); px[2]=(u8)(famCol[fm][0][2]*8); }
        fwrite(px,1,3,f); } fclose(f);
    // a zoomed window at 8 px a tile with real tile art and sprite shapes
    int Z=3, X0=argc>3?atoi(argv[3]):40, Y0=argc>4?atoi(argv[4]):40, TW=32, TH=24; f=fopen("/tmp/gg_zoom.ppm","wb"); fprintf(f,"P6\n%d %d\n255\n",TW*8*Z,TH*8*Z);
    for(int y=0;y<TH*8*Z;y++) for(int x=0;x<TW*8*Z;x++){ int px8=x/Z, py8=y/Z; int tx=X0+px8/8, ty=Y0+py8/8; int t=world[ty][tx]; int v=(int)(hsh(tx,ty,3)&3); int r,g,b; int pi=bld[ty][tx]?bldPix(bld[ty][tx],bldv[ty][tx]?vil[bldv[ty][tx]-1].race:0,px8%8,py8%8):tilePix(t,v,px8%8,py8%8); u8 px[3]; if(pi>=128){ int k=pi-128; if(k<8){ px[0]=(u8)(roofCol[k][0]*8); px[1]=(u8)(roofCol[k][1]*8); px[2]=(u8)(roofCol[k][2]*8);} else { px[0]=(u8)(bldCol[k-8][0]*8); px[1]=(u8)(bldCol[k-8][1]*8); px[2]=(u8)(bldCol[k-8][2]*8);} } else { shadeRGB(pi,&r,&g,&b); px[0]=(u8)(r*8); px[1]=(u8)(g*8); px[2]=(u8)(b*8); }
        if(occ[ty][tx]){ const Cr*c=&cr[occ[ty][tx]-1]; char ch=shpRow(arch[sp[c->sp].a].shape,0,py8%8)[px8%8]; int fm=sp[c->sp].fam; int k=ch=='b'?0:ch=='d'?1:ch=='a'?2:-1;
            if(k>=0){ px[0]=(u8)(famCol[fm][k][0]*8); px[1]=(u8)(famCol[fm][k][1]*8); px[2]=(u8)(famCol[fm][k][2]*8); } else if(ch=='e'){ px[0]=px[1]=px[2]=255; } }
        fwrite(px,1,3,f); } fclose(f);
    for(int v=0;v<VIL_N;v++) if(vil[v].on) printf("village %d race %s at %d,%d lvl %d homes %d farms %d towers %d pop %d\n",v,arch[vil[v].race].nm,vil[v].x,vil[v].y,vil[v].lvl,vil[v].houses,vil[v].farms,vil[v].towers,vil[v].pop);
    printf("species %d\n",nSp); return 0; }
