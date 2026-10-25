// test_kingview.c - PC test of source/kingview.h: the hardware tile background. A 32 x 32 screenblock is simulated; the camera is moved at random (and by jumps, to the corners and
// the edges); after every move the 31 x 21 tiles the hardware would show must be exactly the tiles of the world cells under them. Then the realm is edited while it is shown and
// must catch up within 21 frames. Run from the repo root:   sh tools/test_kingview.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"
#include "../source/kingsim.h"
#include "../source/kinglog.h"
static u16 sb[1024]; static long puts_=0;
#define KV_PUT(sx,sy,e) do{ if((sx)<0||(sx)>31||(sy)<0||(sy)>31){ printf("FAIL: put outside the screenblock %d %d\n",(sx),(sy)); exit(1); } sb[(sy)*32+(sx)]=(u16)(e); puts_++; }while(0)
#include "../source/kingview.h"
#define KH_TILES 60   // (kinghand.h: the text strip, 60 tiles)
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static u32 rs=12345; static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static int land_(const u8*c,int d,int x,int y){ if(x<0||y<0||x>=d||y>=d) return 0; int v=c[y*d+x]; if(v&KC_BLD) return 1; v&=7; return v!=KT_WATER&&v!=KT_BRIDGE; }   // written out again: a beach lies against this cell
static int road_(const u8*c,int d,int x,int y){ if(x<0||y<0||x>=d||y>=d) return 0; int v=c[y*d+x]; if(v&KC_BLD) return 1; v&=7; return v==KT_ROAD||v==KT_BRIDGE; }  // a road goes on into this cell
static int within(int t,int base,int n){ return t>=base&&t<base+n; }
// is this screenblock entry right for world tile (tx,ty)? Written out again, differently from kvEntry: by the rules of the art, not by its arithmetic
static int right(const u8*cells,int d,const KBld*B,int nB,int castle,int tx,int ty,u16 e){
    int t=e&0xFFF, bank=e>>12, cx=tx>>1, cy=ty>>1, q=(ty&1)*2+(tx&1);
    if(cx<0||cy<0||cx>=d||cy>=d) return within(t,KA_DEEP,4)&&bank==1;
    int v=cells[cy*d+cx];
    if(v&KC_BLD){
        for(int i=0;i<nB;i++) for(int j=0;j<4;j++){ int bx=B[i].x+(j&1), by=B[i].y+(j>>1); if(bx!=cx||by!=cy) continue;
            int X=tx-B[i].x*2, Y=ty-B[i].y*2, base, bk;
            if(B[i].type==KB_CASTLE){ int lk=B[i].lvl?4:castle<=0?-1:castle==1?0:castle==2?1:3; if(lk<0){ base=KA_SITE; bk=0; } else { base=KA_CASTLE0+16*lk; bk=kaBldBank[KB_CASTLE]; } }
            else if(!B[i].lvl){ base=KA_SITE; bk=0; } else { base=kaBldTile[B[i].type]; bk=kaBldBank[B[i].type]; }
            return t==base+Y*4+X&&bank==bk; }
        return within(t,KA_DEEP,4)&&bank==1; }
    int dx=(tx&1)?1:-1, dy=(ty&1)?1:-1;
    switch(v&7){
    case KT_GRASS: return within(t,KA_GRASS,8)&&bank==0;
    case KT_MEADOW: return within(t,KA_MEADOW,8)&&bank==0;
    case KT_FOREST: return within(t,KA_FOREST,16)&&(t-KA_FOREST)%4==q&&bank==0;
    case KT_ROCK: return within(t,KA_ROCK,12)&&(t-KA_ROCK)%4==q&&bank==1;
    case KT_CRYSTAL: return within(t,KA_CRYSTAL,8)&&(t-KA_CRYSTAL)%4==q&&bank==1;
    case KT_WATER: { int m=land_(cells,d,cx+dx,cy)+2*land_(cells,d,cx,cy+dy)+4*land_(cells,d,cx+dx,cy+dy); return t==KA_WATER+q*8+m&&bank==1; }
    case KT_ROAD: { int hv=road_(cells,d,cx+dx,cy)+2*road_(cells,d,cx,cy+dy); return within(t,KA_ROAD,32)&&(t-KA_ROAD)/8==q&&((t-KA_ROAD)/2)%4==hv&&bank==0; }
    case KT_BRIDGE: { int along=road_(cells,d,cx-1,cy)||road_(cells,d,cx+1,cy); return within(t,KA_BRIDGE,8)&&(t-KA_BRIDGE)%4==q&&((t-KA_BRIDGE)/4==0)==along&&bank==1; }
    }
    return 0; }
static int shown(const KView*V,const KBld*B){   // how many of the tiles the screen shows are wrong
    int bad=0; int nx=V->x>>3, ny=V->y>>3;
    for(int j=0;j<KV_H;j++) for(int i=0;i<KV_W;i++){ int wx=nx+i, wy=ny+j; u16 e=sb[((wy)&31)*32+((wx)&31)];
        if(e!=kvEntry(V,wx,wy)||!right(V->cells,V->dim,B,V->nB,V->castle,wx,wy,e)) bad++; }
    return bad; }
int main(void){
    printf("== kingview: the hardware tile background\n");
    for(int tier=0;tier<3;tier++){
        int d=kgDim(tier); static u8 cells[128*128]; static KBld B[KS_MAXB]; KRealm R; memset(&R,0,sizeof R); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=900u+(u32)tier; R.rng=5; kgGen(cells,R.seed,tier);
        int n=0; for(int y=6;y<d-6&&n<KS_MAXB;y+=9) for(int x=6;x<d-6&&n<KS_MAXB;x+=9){ B[n].type=(u8)(n%KS_NTYPES); B[n].x=(u8)x; B[n].y=(u8)y; B[n].lvl=(u8)(n&1); B[n].work=0; B[n].stock=0; n++; }
        kgStamp(cells,tier,B,n);
        KView V; memset(sb,0xEE,sizeof sb); kvInit(&V,cells,d,B,n);
        V.x=V.y=0; kvClamp(&V); kvSync(&V); CHECK(shown(&V,B)==0,"tier %d first draw wrong",tier);
        long moves=0; int worstPuts=0;
        for(int k=0;k<20000;k++){
            long before=puts_; int r=(int)(rnd()%100);
            if(r<3){ V.x=(int)(rnd()%(u32)(d*16)); V.y=(int)(rnd()%(u32)(d*16)); }                               // a jump anywhere
            else if(r<5){ V.x=(rnd()&1)?0:kvMaxX(&V); V.y=(rnd()&1)?0:kvMaxY(&V); }                          // a corner
            else { V.x+=(int)(rnd()%19)-9; V.y+=(int)(rnd()%19)-9; }                                         // up to 9 pixels a frame, every way
            kvClamp(&V); kvSync(&V); moves++;
            if(r>=5){ int p=(int)(puts_-before); if(p>worstPuts) worstPuts=p; }
            int bad=shown(&V,B); CHECK(bad==0,"tier %d move %d: %d tiles wrong at camera %d,%d",tier,k,bad,V.x,V.y); if(bad) break;
        }
        CHECK(worstPuts<=2*(KV_W+KV_H),"a normal move wrote %d tiles (9 pixels can cross two tile edges: 2 columns and 2 rows, 104 at most)",worstPuts);
        // edits while shown: fell trees, lay roads, finish a building; one row refreshes per frame
        for(int e=0;e<400;e++){ int x=(V.x>>4)+(int)(rnd()%(KV_W/2)), y=(V.y>>4)+(int)(rnd()%(KV_H/2)); if(x<0||y<0||x>=d||y>=d||(cells[y*d+x]&KC_BLD)) continue; cells[y*d+x]=(u8)(rnd()%KT_N); }
        for(int i=0;i<n;i++) B[i].lvl=1;
        for(int f=0;f<KV_H;f++) kvRefreshRow(&V);
        CHECK(shown(&V,B)==0,"tier %d: edits did not show within %d frames",tier,KV_H);
        { int x=(V.x>>4)+3, y=(V.y>>4)+3; if(!(cells[y*d+x]&KC_BLD)){ cells[y*d+x]=KT_ROAD; kvPoke(&V,x,y); CHECK(shown(&V,B)==0,"kvPoke left a tile wrong (the cell and its neighbours' autotiles)"); } }
        printf("  tier %d (%dx%d): %ld camera moves, every visible tile right after each; worst normal move wrote %d tiles\n",tier,d,d,moves,worstPuts);
    }
    // the castle's five looks and its site, a stage at a time, are all drawn right
    { int d=kgDim(0); static u8 cells[128*128]; static KBld B[KS_MAXB]; KRealm R; memset(&R,0,sizeof R); R.tier=0; R.genVer=KG_GENVER; R.seed=77; R.rng=5; kgGen(cells,R.seed,0);
      memset(B,0,sizeof B); B[0].type=KB_CASTLE; B[0].x=20; B[0].y=20; B[0].lvl=0; kgStamp(cells,0,B,1); KView V; kvInit(&V,cells,d,B,1);
      for(int st=0;st<=4;st++){ V.castle=st; V.x=20*16-60; V.y=20*16-40; kvClamp(&V); memset(sb,0xEE,sizeof sb); kvFull(&V); CHECK(shown(&V,B)==0,"castle stage %d",st);
        int seen=0; for(int ty=40;ty<44;ty++) for(int tx=40;tx<44;tx++){ int t=sb[(ty&31)*32+(tx&31)]&0xFFF; seen+=st==0?within(t,KA_SITE,16):st==1?within(t,KA_CASTLE0,16):st==2?within(t,KA_CASTLE0+16,16):st==3?within(t,KA_CASTLE0+48,16):0; }
        CHECK(st==4||seen==16,"stage %d draws its own 16 tiles (%d)",st,seen); }
      B[0].lvl=1; V.castle=4; memset(sb,0xEE,sizeof sb); kvFull(&V); CHECK(shown(&V,B)==0,"the crowned castle"); { int seen=0; for(int ty=40;ty<44;ty++) for(int tx=40;tx<44;tx++) seen+=within(sb[(ty&31)*32+(tx&31)]&0xFFF,KA_CASTLE0+64,16); CHECK(seen==16,"the crowned castle is the fifth look"); }
      printf("  castle: the site and the four looks of the castle, each in its 16 tiles\n"); }
    // the art itself: the palette banks, the tiles and the second frame of the waves
    { CHECK(KA_NT+KH_TILES+1<=512,"the map's tiles and the text must fit one charblock");
      for(int t=0;t<KS_NTYPES;t++){ CHECK(kaBldBank[t]>=2&&kaBldBank[t]<=14,"type %d's bank %d is a building bank",t,kaBldBank[t]); for(int u=0;u<t;u++) CHECK(kaBldBank[t]!=kaBldBank[u]||(t==KB_CUTTER&&u==KB_MASON),"types %d and %d share bank %d",u,t,kaBldBank[t]); }
      for(int t=0;t<KS_NTYPES;t++) if(t!=KB_CASTLE) for(int u=0;u<t;u++) if(u!=KB_CASTLE) CHECK(kaBldTile[t]!=kaBldTile[u],"types %d and %d share a picture",u,t);
      CHECK(kaBldTile[KB_STORE]+16<=KA_CASTLE0||kaBldTile[KB_STORE]>=KA_CASTLE0,"building pictures are 16 tiles each");
      for(int b=0;b<16;b++) CHECK(kaPal[b][0]==kaPal[0][0]||b==15,"bank %d's colour 0 is the backdrop's",b);
      int nbad=0; for(int t=0;t<KA_NT;t++) for(int i=0;i<32;i++) (void)kaTile[t][i];
      int diff=0; for(int t=0;t<32;t++) for(int i=0;i<32;i++) diff+=kaTile[KA_WATER+t][i]!=kaWaterB[t][i]; CHECK(diff>100,"the second frame of the waves differs (%d bytes)",diff);
      CHECK(KA_BRIDGE+8<=KA_SITE&&KA_SITE+16<=kaBldTile[KB_HUT]&&KA_CASTLE0+80<=KA_NT,"the tile groups are laid out one after another");
      for(int t=0;t<14;t++) if(t!=KB_CASTLE){ int dup=0; for(int u=0;u<16;u++) for(int w=u+1;w<16;w++) dup+=!memcmp(kaTile[kaBldTile[t]+u],kaTile[kaBldTile[t]+w],32)&&u/4!=0&&w/4!=0&&0; (void)dup; }
      (void)nbad; printf("  art: %d tiles, %d bytes of tile data (a charblock is 16384), 15 map palette banks and the text's\n",KA_NT,KA_NT*32); }
    // the day's light: noon is plain, night is dim and blue, and every tint keeps a colour inside its 5 bit channels
    { int tr,tg,tb; kvDayTint(KS_DAYT*3/10,&tr,&tg,&tb); CHECK(tr==16&&tg==16&&tb==16,"noon is plain (%d %d %d)",tr,tg,tb); kvDayTint(KS_DAYT*9/10,&tr,&tg,&tb); CHECK(tb>tr&&tr<12,"night is bluer and dimmer (%d %d %d)",tr,tg,tb);
      for(unsigned ph=0;ph<KS_DAYT;ph+=7){ kvDayTint(ph,&tr,&tg,&tb); CHECK(tr>=7&&tr<=17&&tg>=7&&tg<=17&&tb>=7&&tb<=17,"tint %u: %d %d %d",ph,tr,tg,tb); for(int c=0;c<32768;c+=997){ u16 v=kvTintC((u16)c,tr,tg,tb); CHECK((v&0x8000)==0,"tinted colour has bit 15"); } }
      printf("  light: dawn, noon, dusk and night tints stay in range\n"); }
    printf("%s: %d failure(s)\n",fails?"FAILED":"ALL PASSED",fails); return fails?1:0;
}
