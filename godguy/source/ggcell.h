// ggcell.h -- GodGuy step 10c: finding enemies by cells instead of by tiles.
//
// The planet is cut into 8 x 8 tile cells. Each cell keeps, for nine kinds of creature, how many of them stand in it (at most 64, so a byte), and a 9-bit mask of the kinds present:
// kind 0..7 is a people (the archetype of its species, when below 8), kind 8 is a monster (a diet-5 archetype). Everything else (animals, kind 9) is not counted: nobody goes to war with a sheep.
// A soldier looking for the nearest enemy reads the masks of its own cell and the eight around it (nine reads) and only walks the tiles of a cell that holds an enemy,
// instead of reading all 49 tiles of its 7 x 7 window. The answer is the one the tile scan gives, tie for tie (nearest by distance, then the first in row order).
//
// Nothing here calls rnd() and nothing changes a creature, so the simulation is exactly what it was. The grid is kept in step with occ[] by addCr, killCr, moveTo and the other places that move a
// creature (cellAdd / cellDel), and rebuilt from scratch wherever occ[] is (compact, new world, load). cellCheck() compares the kept grid with a rebuilt one for the tests.
// Memory: 9 bytes + 2 bytes a cell: 11 KB on the DS (256 x 256 planet: 1,024 cells), 16 x 128 x 128 cells x 11 B = 180 KB on a 1024 planet.
#ifndef GGCELL_H
#define GGCELL_H

#define CELL_SH 3
#define CELL_MAX ((MAXD>>CELL_SH)*(MAXD>>CELL_SH))
#define CELL_K 9
static u8 cellCnt[CELL_MAX][CELL_K]; static u16 cellMask[CELL_MAX];

static int cellKind(int spIdx){ int a=sp[spIdx].a; if(arch[a].diet==5) return 8; return a<8?a:-1; }
static int cellOf(int x,int y){ return (y>>CELL_SH)*((gW+7)>>CELL_SH)+(x>>CELL_SH); }
static void cellClear(void){ memset(cellCnt,0,sizeof cellCnt); memset(cellMask,0,sizeof cellMask); }
static void cellAdd(int x,int y,int spIdx){ int k=cellKind(spIdx); if(k<0) return; int c=cellOf(x,y); cellCnt[c][k]++; cellMask[c]|=(u16)(1<<k); }
static void cellDel(int x,int y,int spIdx){ int k=cellKind(spIdx); if(k<0) return; int c=cellOf(x,y); if(cellCnt[c][k]&&--cellCnt[c][k]==0) cellMask[c]&=(u16)~(1<<k); }
static void cellRebuild(void){ cellClear(); for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].x<gW&&cr[i].y<gH&&occ[cr[i].y][cr[i].x]==(occ_t)(i+1)) cellAdd(cr[i].x,cr[i].y,cr[i].sp); }

// The nearest creature of any kind in the mask em (bit 0..7 a people, bit 8 monsters) within the (2R+1) x (2R+1) window round creature ci, R at most 8. Returns its index, or -1;
// *dist gets the Manhattan distance. Same answer as scanning the window row by row and keeping the first nearest (the creature itself counts, as it does in the tile scan).
static int cellNearest(int ci,int R,unsigned em,int*dist){
    int cx=cr[ci].x, cy=cr[ci].y, cw=(gW+7)>>CELL_SH, ch=(gH+7)>>CELL_SH; int x0=cx-R<0?0:cx-R, x1=cx+R>=gW?gW-1:cx+R, y0=cy-R<0?0:cy-R, y1=cy+R>=gH?gH-1:cy+R;
    int best=-1; unsigned bk=~0u;
    for(int gy=y0>>CELL_SH;gy<=(y1>>CELL_SH)&&gy<ch;gy++) for(int gx2=x0>>CELL_SH;gx2<=(x1>>CELL_SH)&&gx2<cw;gx2++){
        if(!(cellMask[gy*cw+gx2]&em)) continue;
        int ya=gy<<CELL_SH, yb=ya+7, xa=gx2<<CELL_SH, xb=xa+7; if(ya<y0) ya=y0; if(yb>y1) yb=y1; if(xa<x0) xa=x0; if(xb>x1) xb=x1;
        for(int y=ya;y<=yb;y++) for(int x=xa;x<=xb;x++){ if(!occ[y][x]) continue; int j=occ[y][x]-1; int k=cellKind(cr[j].sp); if(k<0||!((em>>k)&1)) continue;
            int dx=x-cx, dy=y-cy; int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); unsigned key=((unsigned)d<<10)|(unsigned)((dy+8)<<5)|(unsigned)(dx+8); if(key<bk){ bk=key; best=j; } } }
    if(best>=0&&dist) *dist=(int)(bk>>10); return best; }

// For the tests: does the kept grid equal one rebuilt from the creatures? (returns the number of cells that differ)
static int cellCheck(void){ static u8 sc[CELL_MAX][CELL_K]; static u16 sm[CELL_MAX]; memcpy(sc,cellCnt,sizeof sc); memcpy(sm,cellMask,sizeof sm); cellRebuild(); int bad=0, n=((gW+7)>>CELL_SH)*((gH+7)>>CELL_SH);
    for(int c=0;c<n;c++) if(memcmp(sc[c],cellCnt[c],CELL_K)||sm[c]!=cellMask[c]) bad++; return bad; }
#endif
