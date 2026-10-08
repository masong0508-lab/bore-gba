// itembake.h - HOST-ONLY item art + rasteriser (never compiled into the GBA ROM; tools/bake_items.sh turns it into itemrom.h).
// BORE room items as true 2:1 isometric sprite art (crate, rail, fridge, toilet, door mat, skateboard, spawn marker).
// Each item is a few textured boxes, rendered ONCE on the PC into small sprites that live in ROM (itemrom.h). They cost NO RAM at all.
// Local frame of an item: a = across, b = front (+b faces the viewer at r=0),
// 8 units per tile, 1 unit = 1 px wide, z in px. Textures are authored as seen from OUTSIDE the face, left to right, top to bottom.
// Letters a..h index the material palette, '.' = see-through.
typedef struct { const char*const*rows; u8 w,h; const u16*pal; } Mat;
typedef struct { u8 a0,b0,a1,b1,z0,z1; const Mat*m[5]; } IBox;   // faces: 0 front(+b) 1 +a 2 back 3 -a 4 top
typedef struct { const IBox*b; u8 n; } IObj;
#define MAT(nm,pal,w,h,...) static const char*const nm##_r[]={__VA_ARGS__}; static const Mat nm={nm##_r,w,h,pal};

// ---- crate (2 stacked wooden crates) ----
static const u16 pCr[4]={RGB(8,4,2),RGB(24,16,7),RGB(17,10,4),RGB(29,21,10)};
#define CR8 "aaaaaaaa","acbbbbca","abcbbcba","abbccbba","abbccbba","abcbbcba","acbbbbca","aaaaaaaa"
MAT(mCrS,pCr,8,16,CR8,CR8)
MAT(mCrT,pCr,8,8,"aaaaaaaa","adddddda","abbbbbba","acccccca","abbbbbba","abbbbbba","acccccca","aaaaaaaa")
static const IBox bxCrate[1]={{0,0,8,8,0,16,{&mCrS,&mCrS,&mCrS,&mCrS,&mCrT}}};

// ---- fridge (front gets the doors and handles) ----
static const u16 pFr[5]={RGB(17,18,21),RGB(31,31,31),RGB(23,24,27),RGB(7,8,12),RGB(28,6,6)};
MAT(mFrF,pFr,6,16,"aaaaaa","abbbba","abbbba","abbdda","abbdda","abbbba","acccca","abbbba","abbdda","abedda","abbdda","abbdda","abbbba","abbbba","abbbba","aaaaaa")
MAT(mFrS,pFr,6,16,"aaaaaa","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","aaaaaa")
MAT(mFrT,pFr,6,6,"aaaaaa","abbbba","abbbba","abbbba","abbbba","aaaaaa")
static const IBox bxFridge[1]={{1,1,7,7,0,16,{&mFrF,&mFrS,&mFrS,&mFrS,&mFrT}}};

// ---- toilet: pedestal, bowl, tank at the back ----
static const u16 pTo[5]={RGB(17,18,22),RGB(31,31,31),RGB(23,24,26),RGB(9,17,27),RGB(12,13,16)};
MAT(mToBase,pTo,1,1,"c")
MAT(mToBowl,pTo,4,3,"bbbb","bbbb","cccc")
MAT(mToBowlT,pTo,4,4,"bbbb","bddb","bddb","bbbb")
MAT(mToTankF,pTo,4,5,"bbbb","bbbb","bbbb","bbbb","cccc")
MAT(mToTankS,pTo,2,5,"bb","bb","bb","bb","cc")
MAT(mToTankT,pTo,4,2,"bbeb","bbbb")
static const IBox bxToilet[3]={
 {3,4,5,6,0,2,{&mToBase,&mToBase,&mToBase,&mToBase,&mToBase}},
 {2,3,6,7,2,5,{&mToBowl,&mToBowl,&mToBowl,&mToBowl,&mToBowlT}},
 {2,1,6,3,3,8,{&mToTankF,&mToTankS,&mToTankF,&mToTankS,&mToTankT}} };

// ---- grind rail, kicker ramp, quarter pipe, ledge, bench: generated art (tools/make_skate_items.py) ----
#include "skateart.h"
// ---- bed, shower, sofa: the life-sim furniture ----
#include "simart.h"
// ---- home pack: TV, bookshelf, coffee maker, aquarium, treadmill ----
#include "homeart.h"

// ---- door mat / threshold ----
static const u16 pDr[4]={RGB(8,5,3),RGB(18,11,6),RGB(24,16,8),RGB(28,20,10)};
MAT(mDrSide,pDr,1,1,"a")
MAT(mDrTop,pDr,8,8,"aaaaaaaa","abbbbbba","abccccba","abcddcba","abcddcba","abccccba","abbbbbba","aaaaaaaa")
static const IBox bxDoor[1]={{0,0,8,8,0,1,{&mDrSide,&mDrSide,&mDrSide,&mDrSide,&mDrTop}}};

// ---- skateboard pickup ----
static const u16 pBd[3]={RGB(6,6,9),RGB(30,13,5),RGB(31,30,26)};
MAT(mBdEdge,pBd,1,1,"b")
MAT(mBdTop,pBd,4,6,"bbbb","baab","baab","baab","baab","bbbb")
MAT(mBdWh,pBd,1,1,"c")
static const IBox bxBoard[5]={
 {2,2,3,3,2,3,{&mBdWh,&mBdWh,&mBdWh,&mBdWh,&mBdWh}}, {5,2,6,3,2,3,{&mBdWh,&mBdWh,&mBdWh,&mBdWh,&mBdWh}},
 {2,5,3,6,2,3,{&mBdWh,&mBdWh,&mBdWh,&mBdWh,&mBdWh}}, {5,5,6,6,2,3,{&mBdWh,&mBdWh,&mBdWh,&mBdWh,&mBdWh}},
 {2,1,6,7,3,4,{&mBdEdge,&mBdEdge,&mBdEdge,&mBdEdge,&mBdTop}} };

// ---- chill pack: water pipe, lava lamp, beanbag ----
// water pipe: a green glass base, a tall tube with a mouthpiece flare, a metal downstem and bowl on the side
static const u16 pGl[5]={RGB(5,16,9),RGB(10,24,14),RGB(16,29,20),RGB(24,31,27),RGB(14,14,16)};
MAT(mGlBase,pGl,4,5,"bbbb","bccb","bccb","bbbb","aaaa")
MAT(mGlBaseT,pGl,4,4,"abba","bddb","bddb","abba")
MAT(mGlTube,pGl,2,14,"dc","cc","cc","cb","cb","cb","cb","cb","cb","cb","cb","cb","cb","bb")
MAT(mGlTubeT,pGl,2,2,"aa","aa")
MAT(mGlStem,pGl,1,4,"e","e","e","e")
static const IBox bxPipe[4]={
 {2,2,6,6,0,5,{&mGlBase,&mGlBase,&mGlBase,&mGlBase,&mGlBaseT}},
 {3,3,5,5,5,19,{&mGlTube,&mGlTube,&mGlTube,&mGlTube,&mGlTubeT}},
 {2,2,6,6,19,20,{&mGlBase,&mGlBase,&mGlBase,&mGlBase,&mGlBaseT}},
 {5,4,7,5,5,9,{&mGlStem,&mGlStem,&mGlStem,&mGlStem,&mGlStem}},
};
// lava lamp: a dark metal cone, a glass body of purple fluid with orange blobs, a metal cap
static const u16 pLv[5]={RGB(6,6,9),RGB(12,12,15),RGB(14,4,20),RGB(31,15,4),RGB(31,26,10)};
MAT(mLvMet,pLv,3,3,"bbb","bab","aaa")
MAT(mLvBody,pLv,3,11,"ccc","cdc","ddc","ccc","ccc","cdd","cde","ccc","dcc","ddc","ccc")
MAT(mLvTop,pLv,3,3,"bab","aaa","bab")
static const IBox bxLava[3]={
 {2,2,6,6,0,3,{&mLvMet,&mLvMet,&mLvMet,&mLvMet,&mLvTop}},
 {3,3,6,6,3,14,{&mLvBody,&mLvBody,&mLvBody,&mLvBody,&mLvTop}},
 {3,3,6,6,14,16,{&mLvMet,&mLvMet,&mLvMet,&mLvMet,&mLvTop}},
};
// beanbag: a squashy purple seat and a slouchy back (faces the open floor, like the sofa)
static const u16 pBb[4]={RGB(10,4,14),RGB(17,8,22),RGB(23,12,27),RGB(27,18,30)};
MAT(mBbS,pBb,6,5,"cccccc","ccdccc","bcccdb","bbbbbb","aaaaaa")
MAT(mBbT,pBb,6,6,"bccccb","ccddcc","cdddcc","ccddcc","cccccc","bccccb")
MAT(mBbBack,pBb,6,4,"cdcccc","cccddc","bccccb","bbbbbb")
static const IBox bxBeanbag[2]={
 {1,2,7,8,0,5,{&mBbS,&mBbS,&mBbS,&mBbS,&mBbT}},
 {1,0,7,3,0,9,{&mBbBack,&mBbS,&mBbBack,&mBbS,&mBbT}},
};

// ---- DeadSet 3Thousand VYBE: a parody VR headset on a display bust (hand-traced pixel art, not boxes; the bust faces right, mirrored for the other way) ----
static const u16 pDs[15]={RGB(3,3,4),RGB(5,5,7),RGB(13,13,15),RGB(6,20,31),RGB(28,21,16),RGB(22,15,11),RGB(13,7,3),RGB(19,11,5),RGB(6,4,3),RGB(6,18,8),RGB(4,12,5),RGB(12,12,14),RGB(20,20,23),RGB(27,15,13),RGB(31,31,31)};
static const char*const dsArt[28]={
 "......aaaaaa.........",
 ".....aiiiiiiaa.......",
 "....aibbbbbbbia......",
 "...aiiiiiiibbbia.....",
 "...aiiiiiiiibbba.....",
 "..aiiiiiiiibbbbbba...",
 "..aiifeiiibbccccbba..",
 "..aiifeeiibcbbbbdcba.",
 "..aiifeeeibcbbbobbcba",
 "..aiieeeeebbccccccbba",
 "..aiieeeeeebbbbbbbbe.",
 "...aieeeeeeeeeeeeeen.",
 "...aiheeeeeeeeeeefna.",
 "...ahhgeeeeefhhhnna..",
 "...ahhhghheehhghhha..",
 "...ahghhhhghhhhhhga..",
 "....ahhhhghhhhghha...",
 "....aghhhhhhghhga....",
 "..aaajghhghhhhgjaa...",
 ".ajjjjjghhhhhgjjjja..",
 "ajjjjjjjgggggjjjjjja.",
 "akjjjjjjjjjjjjjjjjka.",
 ".akkkjjjjjjjjjjkkka..",
 "..aaakkkkkkkkkkaa....",
 ".......alllla........",
 "....aallmmmllllaa....",
 "...almmmmmmmmmlla....",
 "....aallllllllaa....."};
#include "itemids.h"
static u16 bakeBuf[NIV][IH][IW];
static u16 itemTmp[IH][IW];
static void rotPt(int r,int a,int b,int*u,int*v){
    switch(r){ case 0:*u=a;*v=b;break; case 1:*u=b;*v=8-a;break; case 2:*u=8-a;*v=8-b;break; default:*u=8-b;*v=a; }
}
static inline u16 matPx(const Mat*m,int col,int row,int n){
    if(col>=m->w) col=m->w-1; if(row>=m->h) row=m->h-1; if(col<0) col=0; if(row<0) row=0;
    char ch=m->rows[row][col]; if(ch=='.') return IKEY; return shade(m->pal[ch-'a'],n);
}
typedef struct { int u0,v0,u1,v1,z0,z1; const IBox*s; } VBox;
static void drawBox(u16 (*d)[IW],const VBox*q,int r){
    const IBox*bx=q->s; int u0=q->u0,u1=q->u1,v0=q->v0,v1=q->v1,z1=q->z1, HT=bx->z1-bx->z0;
    const Mat*ml=bx->m[(4-r)&3], *mr=bx->m[(5-r)&3], *mt=bx->m[4];   // view +v face = local face (0-r), view +u face = local face (1-r)
    for(int u=u0;u<u1;u++){   // left face
        int X=IOX+u-v1, yh=IOY-4+((u+v1)>>1)-z1;
        for(int j=0;j<HT;j++){ int y=yh+1+j; u16 c=matPx(ml,u-u0,j,12); if(c!=IKEY&&(unsigned)X<IW&&(unsigned)y<IH) d[y][X]=c; }
    }
    for(int v=v1;v>=v0;v--){  // right face (the last column is the tip)
        int X=IOX+u1-v, yh=IOY-4+((u1+v)>>1)-z1;
        for(int j=0;j<HT;j++){ int y=yh+1+j; u16 c=matPx(mr,v1-v,j,9); if(c!=IKEY&&(unsigned)X<IW&&(unsigned)y<IH) d[y][X]=c; }
    }
    for(int X=u0-v1;X<=u1-v0;X++){   // top face
        int vlo=v0>u0-X?v0:u0-X, vhi=v1<u1-X?v1:u1-X, ylo=(2*vlo+X+1)>>1, yhi=(2*vhi+X)>>1;
        for(int Y=ylo;Y<=yhi;Y++){
            int v2=2*Y-X, cv=v2>>1, cu=(v2+2*X)>>1;
            if(cu<u0) cu=u0; if(cu>u1-1) cu=u1-1; if(cv<v0) cv=v0; if(cv>v1-1) cv=v1-1;
            int la,lb; switch(r){ case 0:la=cu;lb=cv;break; case 1:la=7-cv;lb=cu;break; case 2:la=7-cu;lb=7-cv;break; default:la=cv;lb=7-cu; }
            u16 c=matPx(mt,la-bx->a0,lb-bx->b0,16);
            int x=IOX+X, y=IOY-4+Y-z1; if(c!=IKEY&&(unsigned)x<IW&&(unsigned)y<IH) d[y][x]=c;
        }
    }
}
static int boxBehind(const VBox*A,const VBox*B){ return A->u1<=B->u0||A->v1<=B->v0||A->z1<=B->z0; }
static void drawObj(u16 (*d)[IW],const IBox*b,int n,int r,int zoff){
    VBox q[10]; int done=0;
    for(int i=0;i<n;i++){
        int ua,va,ub,vb; rotPt(r,b[i].a0,b[i].b0,&ua,&va); rotPt(r,b[i].a1,b[i].b1,&ub,&vb);
        q[i].u0=ua<ub?ua:ub; q[i].u1=ua<ub?ub:ua; q[i].v0=va<vb?va:vb; q[i].v1=va<vb?vb:va;
        q[i].z0=b[i].z0+zoff; q[i].z1=b[i].z1+zoff; q[i].s=&b[i];
    }
    while(done<n){   // painter order: a box waits for any box that must be drawn before it
        int pick=-1;
        for(int i=0;i<n&&pick<0;i++){ if(!q[i].s) continue; int ok=1;
            for(int j=0;j<n;j++){ if(j==i||!q[j].s) continue; if(boxBehind(&q[j],&q[i])&&!boxBehind(&q[i],&q[j])) { ok=0; break; } }
            if(ok) pick=i; }
        if(pick<0) for(int i=0;i<n;i++) if(q[i].s){ pick=i; break; }
        drawBox(d,&q[pick],r); q[pick].s=0; done++;
    }
}
static void outlineSpr(u16 (*d)[IW],int nsh){   // tonal outline on the silhouette, like the voxel cubes had
    for(int y=0;y<IH;y++)for(int x=0;x<IW;x++) itemTmp[y][x]=d[y][x];
    for(int y=0;y<IH;y++)for(int x=0;x<IW;x++){
        if(itemTmp[y][x]==IKEY) continue;
        int e=(x==0||itemTmp[y][x-1]==IKEY)||(x==IW-1||itemTmp[y][x+1]==IKEY)||(y==0||itemTmp[y-1][x]==IKEY)||(y==IH-1||itemTmp[y+1][x]==IKEY);
        if(e) d[y][x]=shade(itemTmp[y][x],nsh);
    }
}
static void bakeOne(int k,const IBox*b,int n,int r,int nsh){
    u16 (*d)[IW]=bakeBuf[k]; for(int y=0;y<IH;y++)for(int x=0;x<IW;x++) d[y][x]=IKEY;
    drawObj(d,b,n,r,0); if(nsh<16) outlineSpr(d,nsh);
}
static void bakePix(int k,const char*const*rows,const u16*pal,int mirror){   // pixel art straight into a sprite
    for(int y=0;y<IH;y++)for(int x=0;x<IW;x++){ char c=y<IPAD?'.':rows[y-IPAD][mirror?IW-1-x:x]; bakeBuf[k][y][x]=c=='.'?IKEY:pal[c-'a']; }   // (the art is IH0 rows tall and sits at the bottom of the taller canvas)
}
// ---- the telephone: a nightstand with a phone on it (pixel art) ----
static const u16 pPh[8]={RGB(3,2,5),RGB(27,19,10),RGB(15,9,5),RGB(20,13,7),RGB(26,6,6),RGB(31,15,12),RGB(8,8,11),RGB(31,31,28)};   // outline, top, left, right, phone, highlight, handset, dial
static const char*const phArt[IH0]={
 ".....................",
 ".....................",
 ".....................",
 ".....................",
 ".....................",
 ".....................",
 ".....................",
 ".....................",
 ".....................",
 "......aaaaaaaaa......",
 ".....aggggggggga.....",
 ".....agfffffffga.....",
 "....aabeeeeeeebaa....",
 "..aabbeeehhheeebbaa..",
 ".abbbbbbbbbbbbbbbbba.",
 ".accbbbbbbbbbbbbbdda.",
 ".accccbbbbbbbbbdddda.",
 ".accccccbbbbbdddddda.",
 ".accccccccbddddccdda.",
 ".acccccccccddchdddda.",
 ".acccccccccdcddccdda.",
 ".acccccccccddccdddda.",
 "..aacccccccdcddddaa..",
 "....aacccccddddaa....",
 "......aacccddaa......",
 "........aacaa........",
 "..........a..........",
 "....................."};
// sound pack: RADIO (a boombox, tile 'R') and SOUND SYSTEM (a speaker tower with an amp, tile 'A'), hand-drawn pixel art
static const u16 pSnd[10]={RGB(3,3,5),RGB(23,23,25),RGB(14,15,18),RGB(9,10,13),RGB(5,5,8),RGB(18,18,22),RGB(31,25,6),RGB(31,6,6),RGB(8,30,12),RGB(8,8,10)};   // outline, top, front, side, speaker, cone shine, gold, red, green, handle
static const char*const rdArt[IH0]={
 ".....................",
 ".....................",
 ".....................",
 ".....................",
 ".....................",
 "..................a..",
 ".................aja.",
 ".................aja.",
 "................aja..",
 "................aja..",
 "...............aja...",
 ".......aaaaaaaaaja...",
 "......ajjjjjjjjja....",
 "......ajjjjjjjjja....",
 ".....aajaaaaaaajaa...",
 "....abbbbbbbbbbbbba..",
 "....abbbbbbbbbbbbba..",
 "...acccccccccccccaba.",
 "...acceeeggggeeecdda.",
 "...aceeeeecceeeeedda.",
 "...aeefeeeeeefeeeeda.",
 "...aeeefeeeeeefeeeda.",
 "...aeeeeeeeeeeeeeeda.",
 "...aceeeeehieeeeedda.",
 "...acceeecccceeecdda.",
 "...acccccccccccccaa..",
 "....aaaaaaaaaaaaa....",
 "....................."};
static const char*const syArt[IH0]={
 ".....................",
 ".....................",
 "......aaaaaaaaaaa....",
 ".....abbbbbbbbbbba...",
 ".....abbbbbbbbbbba...",
 "....acccccccccccaba..",
 "....aceeeeeeeeecdda..",
 "....acegggffffecdda..",
 "....acefhfffifecdda..",
 "....aceeeeeeeeecdda..",
 "....acccceeeccccdda..",
 "....acccefeeecccdda..",
 "....accceeeeecccdda..",
 "....accceeeeecccdda..",
 "....acccceeeccccdda..",
 "....acccccccccccdda..",
 "....acccceeeccccdda..",
 "....accceeeeecccdda..",
 "....acceefffeeccdda..",
 "....aceefffffeecdda..",
 "....aceeffeffeecdda..",
 "....aceefffffeecdda..",
 "....acceefffeeccdda..",
 "....accceeeeecccdda..",
 "....acccceeeccccdda..",
 "....acccccccccccaa...",
 ".....aaaaaaaaaaa.....",
 "....................."};
static void bakeAll(void){
    bakeOne(V_CRATE,bxCrate,1,0,11);
    for(int r=0;r<4;r++){ bakeOne(V_FRIDGE+r,bxFridge,1,r,11); bakeOne(V_TOILET+r,bxToilet,3,r,11);
        bakeOne(V_BED+r,bxBed,4,r,11); bakeOne(V_SHOWER+r,bxShower,4,r,11); bakeOne(V_SOFA+r,bxSofa,4,r,11); }
    bakeOne(V_RAILU,bxRailU,3,0,16); bakeOne(V_RAILV,bxRailV,3,0,16);
    bakeOne(V_DOOR,bxDoor,1,0,13); bakeOne(V_BOARD,bxBoard,5,0,12);
    for(int r=0;r<4;r++){ bakeOne(V_KICKER+r,bxKicker,8,r,11); bakeOne(V_QPIPE+r,bxQuarterPipe,8,r,11); }
    bakeOne(V_LEDGEU,bxLedgeU,1,0,12); bakeOne(V_LEDGEV,bxLedgeV,1,0,12); bakeOne(V_BENCHU,bxBenchU,3,0,12); bakeOne(V_BENCHV,bxBenchV,3,0,12);
    for(int r=0;r<4;r++) bakeOne(V_LAUNCH+r,bxLaunch,8,r,11);   // skate pack 2
    bakeOne(V_FUNBOX,bxFunbox,1,0,12); bakeOne(V_BARREL,bxBarrel,1,0,11); bakeOne(V_TRASH,bxTrashCan,2,0,11); bakeOne(V_PLANTER,bxPlanter,1,0,11);
    bakeOne(V_PIPE,bxPipe,4,0,10); bakeOne(V_LAVA,bxLava,3,0,10); for(int r=0;r<4;r++) bakeOne(V_BEANBAG+r,bxBeanbag,2,r,11);   // chill pack
    bakePix(V_DEADSET,dsArt,pDs,0); bakePix(V_DEADSET+1,dsArt,pDs,1);   // the DeadSet
    bakeOne(V_PICNIC,bxPicnicTable,7,0,12); bakeOne(V_JERSEYU,bxJerseyU,3,0,12); bakeOne(V_JERSEYV,bxJerseyV,3,0,12); bakeOne(V_MPAD,bxManualPad,1,0,12);
    bakePix(V_PHONE,phArt,pPh,0);   // the telephone
    bakePix(V_RADIO,rdArt,pSnd,0); bakePix(V_STEREO,syArt,pSnd,0);   // sound pack
    for(int r=0;r<4;r++){ bakeOne(V_TV+r,bxTv,5,r,11); bakeOne(V_SHELF+r,bxShelf,1,r,11); bakeOne(V_TREAD+r,bxTread,5,r,11); }   // home pack
    bakeOne(V_COFFEE,bxCoffee,4,0,11); bakeOne(V_AQUA,bxAqua,3,0,11);
    for(int r=0;r<4;r++){   // long ramps: the chained kicker tiles and launch tiles (see ramps.h)
        bakeOne(V_KSEG+r,bxKickerSeg0,8,r,11); bakeOne(V_KSEG+4+r,bxKickerSeg1,8,r,11); bakeOne(V_KSEG+8+r,bxKickerSeg2,8,r,11); bakeOne(V_KSEG+12+r,bxKickerSeg3,8,r,11);
        bakeOne(V_LSEG+r,bxLaunchSeg0,8,r,11); bakeOne(V_LSEG+4+r,bxLaunchSeg1,8,r,11); bakeOne(V_LSEG+8+r,bxLaunchSeg2,8,r,11); }
}
