// housegen.h - HOUSES THAT ARE NOT ALL THE SAME. hgHouse builds a furnished house on the live map inside a lot rectangle (the street is at the
// bottom): a style (COTTAGE, FAMILY HOME, LOFT, MODERN HOUSE, MANSION), a seed (the same seed always builds the same house, so a lot keeps its own
// house), how many beds, and the story extras (SL_ flags, storylot.h).
//   THE PLAN   bedrooms and the bathroom(s) in a strip along the back, their widths drawn from the seed; the kitchen and the living room in front
//              (side by side with a door between, or open plan in a loft); a door into every room and the front door to the street.
//   WINDOWS    in the outer walls, never where an inside wall meets them and never beside a door. The style picks the kind.
//   FURNITURE  against the walls (never in front of a door), so every room can be walked through: beds, dressers, desks in the bedrooms; the
//              toilet and the shower; the fridge, counters, coffee and a dining table in the kitchen; the sofa, the TV, a rug, shelves, plants,
//              lamps, a fireplace, an aquarium, a stereo ... in the living room, by style.
//   MANSION    big rooms, velvet walls, two bathrooms, a dining table for four, and a low garden fence round the lot with an open gateway at
//              the front: lock it with SECURITY GATES and watch it with cameras (TV SHOW & TELL chapter 3, "a private house with tight security").
// Needs: main.c (lifeMap, floorMap, wallMap, gRoom, gBox, gPut, NWP), storylot.h's SL_ flags (passed in as numbers).
enum { HS_COTTAGE, HS_FAMILY, HS_LOFT, HS_MODERN, HS_MANSION, HS_N };
static const char* const hsNm[HS_N]={"COTTAGE","FAMILY HOME","LOFT","MODERN HOUSE","MANSION"};
static const u16 hsCost[HS_N]={2500,4000,3000,5000,15000};   // what the house adds to the land's price
// floors (main.c flTex): 0 TAN CHECK 1 BEIGE CARPET 2 TEAL CARPET 3 CHECKER LINO 4 WOOD PLANKS 5 PINK TILE 6 BLUE TILE 7 CONCRETE 8 STEEL PLATE 11 GREEN LINO 13 RED TILE
// wallpapers (wallart.h, + NWP): 7 BLANK CANVAS 10 CALICO CITY 11 CALICO FANCY 15 CHALKBOARD 23 DEBUTANTE 24 DESIGN STYLE 33 GRUNGEE GOLD 38 HERCULANEUM 39 HYPER LYFE
//   41 IMPERIAL 46 KOORDINATED 48 LEMON MERINGUE 49 LOVER S LACE 53 METAL DECK 56 NOBLE VELVET 57 OCEANIC 58 ORNAMENTAL 61 PARLOR 63 PINEAPPLE 65 POMPEII DAYS
//   66 QUIET ROOM 67 RED VELVET 70 SAUNISSIMO 72 SETTLER S 74 SHOOTING STARS 75 SUGARPLUM 77 TENDER SLUMBER 79 TICKLED FANCY 84 UP UP AND AWAY 85 VINTNER S 87 WESTCHESTER 88 YUMMY TUMMU
typedef struct { u8 fl[4][2], wp[4][3]; char win; } HsLook;   // per room kind (living, kitchen, bedroom, bathroom): two floors and three wallpapers to pick from
enum { HR_LIV, HR_KIT, HR_BED, HR_BATH };
static const HsLook hsLook[HS_N]={
    {{{4,4},{3,11},{1,4},{6,5}},   {{61,77,72},{63,88,48},{48,75,11},{57,70,57}},  'E'},   // COTTAGE: wood floors, warm papers, white frame windows
    {{{2,1},{3,0},{1,2},{5,6}},    {{87,66,10},{48,63,88},{23,74,84},{70,57,70}},  'E'},   // FAMILY HOME: carpets, checker lino, bright bedrooms
    {{{7,8},{8,7},{4,7},{6,6}},    {{53,7,15},{7,53,7},{33,15,53},{53,57,53}},     'f'},   // LOFT: concrete and steel, metal deck, strip windows
    {{{4,0},{6,3},{2,1},{6,6}},    {{7,24,39},{46,7,24},{66,7,74},{57,70,57}},     'f'},   // MODERN HOUSE: blank walls, design prints, strip windows
    {{{13,4},{3,13},{1,2},{5,6}},  {{67,56,41},{58,41,58},{61,79,49},{38,65,38}},  'e'},   // MANSION: red tile and velvet, imperial, dark framed windows
};
static u32 hgR;
static int hgRnd(int n){ hgR=hgR*1664525u+1013904223u; return n>0?(int)((hgR>>16)%(u32)n):0; }
static int hgNearDoor(int x,int y){ return lifeMap[y][x-1]=='D'||lifeMap[y][x+1]=='D'||lifeMap[y-1][x]=='D'||lifeMap[y+1][x]=='D'; }
// THE WALK-THROUGH CHECK: every piece of furniture is tried, then kept only if the house can still be walked: every floor tile inside is reachable
// from the front door, and every piece of furniture has a reachable tile beside it (so it can be used). The search borrows the path-search scratch
// (bfsQ as the queue, hhDist as the visited marks with a running number), which no search is using while a house is built.
static int hgFX, hgFY, hgX0, hgY0, hgX1, hgY1; static u16 hgGen;
static int hgWalk(char c){ return c=='.'||c=='r'||c=='D'||c=='P'||c=='B'; }
static int hgOk(void){
    if(++hgGen==0xFFFF){ for(int i=0;i<MW*MH;i++) hhDist[i]=0; hgGen=1; }
    u16*q=bfsQ; int qh=0, qt=0; hhDist[hgFY*MW+hgFX]=hgGen; q[qt++]=(u16)(hgFY*MW+hgFX);
    while(qh<qt){ int p=q[qh++], x=p%MW, y=p/MW;
        for(int d=0;d<4;d++){ int nx=x+(d==0)-(d==1), ny=y+(d==2)-(d==3); if(nx<hgX0||ny<hgY0||nx>hgX1||ny>hgY1) continue;
            int np=ny*MW+nx; if(hhDist[np]==hgGen||!hgWalk(lifeMap[ny][nx])) continue; hhDist[np]=hgGen; q[qt++]=(u16)np; } }
    for(int y=hgY0+1;y<hgY1;y++) for(int x=hgX0+1;x<hgX1;x++){ char c=lifeMap[y][x]; int p=y*MW+x;
        if(hgWalk(c)){ if(hhDist[p]!=hgGen) return 0; continue; }
        if(c=='W'||isWinCh(c)) continue;
        if(hhDist[p-1]!=hgGen&&hhDist[p+1]!=hgGen&&hhDist[p-MW]!=hgGen&&hhDist[p+MW]!=hgGen) return 0; }   // (furniture nobody can stand next to)
    return 1;
}
// put c on a free floor tile inside the room (inner tiles x0..x1, y0..y1) against a wall (side 1: the back wall only), never in front of a door,
// and only where the house can still be walked. 1 = placed
static int hgPut(int x0,int y0,int x1,int y1,int side,char c){
    u16 cand[64]; int n=0;
    for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){ int edge=side==1?y==y0:(x==x0||x==x1||y==y0||y==y1);
        if(edge&&lifeMap[y][x]=='.'&&!hgNearDoor(x,y)&&n<64) cand[n++]=(u16)(y*MW+x); }
    while(n>0){ int k=hgRnd(n), p=cand[k]; cand[k]=cand[--n]; int x=p%MW, y=p/MW;
        lifeMap[y][x]=c; if(hgOk()) return 1; lifeMap[y][x]='.'; }
    return 0;
}
static void hgSofaTv(int x0,int y0,int x1,int y1){   // the sofa against the back wall and the TV two tiles in front of it (as every house had it), or both on walls
    for(int t=0;t<12;t++){ int x=x0+hgRnd(x1-x0+1);
        if(y1-y0>=4&&lifeMap[y0][x]=='.'&&lifeMap[y0+2][x]=='.'&&!hgNearDoor(x,y0)&&!hgNearDoor(x,y0+2)){
            lifeMap[y0][x]='C'; lifeMap[y0+2][x]='v'; if(hgOk()) return; lifeMap[y0][x]='.'; lifeMap[y0+2][x]='.'; } }
    hgPut(x0,y0,x1,y1,0,'C'); hgPut(x0,y0,x1,y1,0,'v');
}
static void hgTable(int x0,int y0,int x1,int y1,int seats){   // a dining table with chairs either side (in the open, one tile from the walls)
    if(x1-x0<4||y1-y0<2) return;
    for(int t=0;t<10;t++){ int x=x0+2+hgRnd(x1-x0-3), y=y0+1+hgRnd(y1-y0-1);
        if(lifeMap[y][x]!='.'||lifeMap[y][x-1]!='.'||lifeMap[y][x+1]!='.'||hgNearDoor(x,y)) continue;
        lifeMap[y][x]='t'; lifeMap[y][x-1]='h'; lifeMap[y][x+1]='h';
        if(!hgOk()){ lifeMap[y][x]=lifeMap[y][x-1]=lifeMap[y][x+1]='.'; continue; }
        if(seats>2&&y-1>y0&&lifeMap[y-1][x]=='.'){ lifeMap[y-1][x]='h'; if(!hgOk()) lifeMap[y-1][x]='.'; }
        if(seats>3&&y+1<y1&&lifeMap[y+1][x]=='.'){ lifeMap[y+1][x]='h'; if(!hgOk()) lifeMap[y+1][x]='.'; }
        return; }
}
static void hgWindows(int hx0,int hy0,int hx1,int hy1,char w){   // windows in the outer walls: not at a corner, not where an inside wall meets, not beside a door
    for(int x=hx0+2;x<=hx1-2;x+=3){
        if(lifeMap[hy1][x]=='W'&&lifeMap[hy1-1][x]!='W'&&lifeMap[hy1][x-1]!='D'&&lifeMap[hy1][x+1]!='D') lifeMap[hy1][x]=w;   // the front
        if(lifeMap[hy0][x]=='W'&&lifeMap[hy0+1][x]!='W'&&lifeMap[hy0][x-1]!='D'&&lifeMap[hy0][x+1]!='D') lifeMap[hy0][x]=w; }   // the back
    for(int y=hy0+2;y<=hy1-2;y+=3){
        if(lifeMap[y][hx0]=='W'&&lifeMap[y][hx0+1]!='W'&&lifeMap[y-1][hx0+1]!='W'&&lifeMap[y+1][hx0+1]!='W') lifeMap[y][hx0]=w;   // the sides
        if(lifeMap[y][hx1]=='W'&&lifeMap[y][hx1-1]!='W'&&lifeMap[y-1][hx1-1]!='W'&&lifeMap[y+1][hx1-1]!='W') lifeMap[y][hx1]=w; }
}
static int hgStyleFor(int seed,int cells){   // a lot's house: what fits it, picked by the lot's own seed (cells = its area in town cells)
    static const u8 sm[3]={HS_COTTAGE,HS_LOFT,HS_MODERN}, md[4]={HS_COTTAGE,HS_FAMILY,HS_LOFT,HS_MODERN}, lg[4]={HS_FAMILY,HS_MODERN,HS_MANSION,HS_FAMILY};
    u32 h=(u32)seed*2654435761u; h^=h>>15; h*=2246822519u; h^=h>>13;
    return cells<=20?sm[h%3]:cells<=40?md[h%4]:lg[h%4];   // (a mansion wants a 7 x 7 lot or bigger)
}
// THE HOUSE. x0..y1: the lot (tiles of the live map). beds 1-3, sep 1 = a bedroom each, flags: the story extras (SL_ in storylot.h: 1 coffee maker,
// 2 sofa and TV, 4 bookshelf, 8 phone, 16 skate rail and manual pad in the yard, 32 moving crates; 0x80 = bare: only what living needs)
static void hgHouse(u32 seed,int style,int beds,int sep,int flags,int x0,int y0,int x1,int y1){
    hgR=seed*2654435761u+0x9E37u; hgRnd(1);
    const HsLook*L=&hsLook[style]; int lw=x1-x0+1, lh=y1-y0+1, man=style==HS_MANSION, loft=style==HS_LOFT, bare=flags&0x80;
    // the size: by style, as big as the lot allows (1 tile to the sides and the back, the yard in front)
    static const u8 wMin[HS_N]={11,13,12,13,18}, wMax[HS_N]={13,16,16,17,22}, hMin[HS_N]={10,11,10,11,13}, hMax[HS_N]={11,13,12,13,16};
    int wd=wMin[style]+hgRnd(wMax[style]-wMin[style]+1), ht=hMin[style]+hgRnd(hMax[style]-hMin[style]+1);
    if(lw>=32&&(man||style==HS_FAMILY||style==HS_MODERN)){ wd+=4+hgRnd(5); ht+=2+hgRnd(3); }   // (a big lot: a bigger house)
    int cw=lw-2-(man?2:0), ch=lh-4-(man?1:0); if(wd>cw) wd=cw; if(ht>ch) ht=ch; if(wd<10) wd=10; if(ht<9) ht=9;
    int hx0=x0+(lw-wd)/2, hy1=y1-3, hy0=hy1-ht+1, hx1=hx0+wd-1;
    if(lw-wd>=4) hx0+=hgRnd(3)-1, hx1=hx0+wd-1;   // (a little off the middle, when there is room)
    for(int y=hy0;y<=hy1;y++)for(int x=hx0;x<=hx1;x++) if(lifeMap[y][x]!='P') lifeMap[y][x]='.';   // (whatever stood there goes)
    int d=man?6:5; if(ht-d<5) d=ht-5; if(d<4) d=4;   // the back strip's depth (its rooms are d-1 tiles deep inside)
    int py=hy0+d;   // the wall between the back rooms and the front
    // the back rooms: bedrooms (shared or one each) and the bathroom(s), in an order from the seed
    int nb=sep?beds:1, nbath=man?2:1, nr=nb+nbath; if(nb<1) nb=1;
    u8 kind[6]; int k=0; for(int i=0;i<nb;i++) kind[k++]=HR_BED; for(int i=0;i<nbath;i++) kind[k++]=HR_BATH;
    while(nr>2){ int need=0; for(int i=0;i<nr;i++) need+=kind[i]==HR_BATH?4:5; if(need<=wd-1) break; if(nbath>1){ nbath--; nr--; kind[nr]=0; } else if(nb>1){ nb--; nr--; for(int i=0;i<nr;i++) kind[i]=i<nb?HR_BED:HR_BATH; } else break; }
    for(int i=nr-1;i>0;i--){ int j=hgRnd(i+1); u8 t=kind[i]; kind[i]=kind[j]; kind[j]=t; }
    int xs[7]; { int need=0; for(int i=0;i<nr;i++) need+=kind[i]==HR_BATH?4:5; int extra=wd-1-need; xs[0]=hx0;
        for(int i=0;i<nr;i++){ int w=kind[i]==HR_BATH?4:5; int add=i==nr-1?extra:hgRnd(extra+1); if(kind[i]==HR_BATH&&add>2&&i<nr-1) add=hgRnd(3); extra-=add; xs[i+1]=xs[i]+w+add; } xs[nr]=hx1; }
    // the front: the kitchen on one side, the living room on the other
    int kl=hgRnd(2), kw=5+hgRnd(3); if(man) kw+=2; if(kw>wd-7) kw=wd-7; int kx=kl?hx0+kw:hx1-kw;   // the kitchen / living room wall
    int lx0=kl?kx:hx0, lx1=kl?hx1:kx, kx0=kl?hx0:kx, kx1=kl?kx:hx1;
    // walls and floors
    gRoom(hx0,hy0,hx1,hy1,L->fl[HR_LIV][hgRnd(2)],NWP+L->wp[HR_LIV][hgRnd(3)]);
    for(int i=0;i<nr;i++){ int rk=kind[i]; gRoom(xs[i],hy0,xs[i+1],py,L->fl[rk][hgRnd(2)],NWP+L->wp[rk][hgRnd(3)]); }
    if(loft) gBox(kx0+1,py+1,kx1-1,hy1-1,L->fl[HR_KIT][0]);   // (open plan: only the floor tells the kitchen)
    else gRoom(kx0,py,kx1,hy1,L->fl[HR_KIT][hgRnd(2)],NWP+L->wp[HR_KIT][hgRnd(3)]);
    // doors: every back room into the front, the kitchen into the living room, the front door to the street
    for(int i=0;i<nr;i++){ int a=xs[i]+1, b=xs[i+1]-1, x=a+hgRnd(b-a+1); if(!loft&&x==kx) x=x>a?x-1:x+1; lifeMap[py][x]='D'; }
    if(!loft){ int y=py+1+hgRnd(hy1-py-1); lifeMap[y][kx]='D'; }
    { int a=lx0+2, b=lx1-2; if(b<a) b=a; int fx=a+hgRnd(b-a+1); lifeMap[hy1][fx]='D'; hgFX=fx; hgFY=hy1; }
    hgX0=hx0; hgY0=hy0; hgX1=hx1; hgY1=hy1+1;   // (the walk-through check: the house and the step outside the front door)
    hgWindows(hx0,hy0,hx1,hy1,L->win);
    // furniture
    for(int i=0;i<nr;i++){ int rx0=xs[i]+1, rx1=xs[i+1]-1, ry0=hy0+1, ry1=py-1;
        if(kind[i]==HR_BATH){ hgPut(rx0,ry0,rx1,ry1,1,'T'); hgPut(rx0,ry0,rx1,ry1,1,'H'); if(man||!bare) hgPut(rx0,ry0,rx1,ry1,0,'p'); continue; }
        int bi=0; for(int j=0;j<i;j++) bi+=kind[j]==HR_BED; int bb=beds/nb+(bi<beds%nb);   // (the beds shared out over the bedrooms there are)
        for(int b=0;b<bb;b++) if(!hgPut(rx0,ry0,rx1,ry1,1,'S')) hgPut(rx0,ry0,rx1,ry1,0,'S');
        if(bare) continue;
        hgPut(rx0,ry0,rx1,ry1,0,'i'); if(hgRnd(2)||man) hgPut(rx0,ry0,rx1,ry1,0,hgRnd(2)?'l':'p');
        if((style==HS_MODERN||style==HS_FAMILY||man)&&hgRnd(3)==0) hgPut(rx0,ry0,rx1,ry1,0,'d'); }
    { int rx0=kx0+1, rx1=kx1-1, ry0=py+1, ry1=hy1-1;   // the kitchen
        hgPut(rx0,ry0,rx1,ry1,0,'F'); if(!bare){ int nc=1+hgRnd(2)+man; for(int i=0;i<nc;i++) hgPut(rx0,ry0,rx1,ry1,0,'y'); }
        if(flags&1) hgPut(rx0,ry0,rx1,ry1,0,'c');
        if(!bare){ hgTable(rx0,ry0,rx1,ry1,man?4:2); if(hgRnd(2)) hgPut(rx0,ry0,rx1,ry1,0,'Y'); } }
    { int rx0=lx0+1, rx1=lx1-1, ry0=py+1, ry1=hy1-1;   // the living room
        if(flags&2) hgSofaTv(rx0,ry0,rx1,ry1);
        if(flags&4) hgPut(rx0,ry0,rx1,ry1,0,'b');
        if(flags&8) hgPut(rx0,ry0,rx1,ry1,0,'I');
        if(flags&32){ for(int i=0;i<3;i++) hgPut(rx0,ry0,rx1,ry1,0,'#'); }
        if(!bare){
            { int cx=(rx0+rx1)/2, cy=(ry0+ry1+1)/2; if(lifeMap[cy][cx]=='.'&&!hgNearDoor(cx,cy)) lifeMap[cy][cx]='r'; }   // a rug in the middle
            hgPut(rx0,ry0,rx1,ry1,0,'p'); if(hgRnd(2)) hgPut(rx0,ry0,rx1,ry1,0,'l');
            switch(style){
            case HS_COTTAGE: hgPut(rx0,ry0,rx1,ry1,1,'o'); if(hgRnd(2)) hgPut(rx0,ry0,rx1,ry1,0,'R'); break;
            case HS_FAMILY:  if(!(flags&4)) hgPut(rx0,ry0,rx1,ry1,0,'b'); if(hgRnd(2)) hgPut(rx0,ry0,rx1,ry1,0,'q'); break;
            case HS_LOFT:    hgPut(rx0,ry0,rx1,ry1,0,'A'); hgPut(rx0,ry0,rx1,ry1,0,'V'); hgPut(rx0,ry0,rx1,ry1,0,'U'); hgPut(rx0,ry0,rx1,ry1,0,'U'); if(hgRnd(2)) hgPut(rx0,ry0,rx1,ry1,0,'m'); break;
            case HS_MODERN:  hgPut(rx0,ry0,rx1,ry1,0,'q'); hgPut(rx0,ry0,rx1,ry1,0,'d'); hgPut(rx0,ry0,rx1,ry1,0,'p'); break;
            case HS_MANSION: hgPut(rx0,ry0,rx1,ry1,1,'o'); hgPut(rx0,ry0,rx1,ry1,0,'q'); hgPut(rx0,ry0,rx1,ry1,0,'A'); hgPut(rx0,ry0,rx1,ry1,0,'C');
                hgPut(rx0,ry0,rx1,ry1,0,'p'); hgPut(rx0,ry0,rx1,ry1,0,'p'); hgPut(rx0,ry0,rx1,ry1,0,'l'); if(!(flags&4)) hgPut(rx0,ry0,rx1,ry1,0,'b'); break;
            } } }
    // the yard
    if(flags&16){ gLine(x0+1,y1-1,x0+4,y1-1,'=',0); gPut(x1-3,y1-1,'M'); gPut(x1-2,y1-1,'M'); }   // a rail and a manual pad
    if(!bare){ int n=man?6:2; for(int i=0;i<n;i++){ int x=x0+1+hgRnd(lw-2), y=y0+1+hgRnd(lh-2); if(x>=hx0-1&&x<=hx1+1&&y>=hy0-1&&y<=hy1+1) continue; if(y>=y1-2) continue; if(lifeMap[y][x]=='.') lifeMap[y][x]='Z'; } }
    if(man){   // the garden fence: low walls round the lot, an open gateway at the front by the path (SECURITY GATES lock it)
        int gx=(x0+x1)/2;
        for(int x=x0;x<=x1;x++){ if(lifeMap[y0][x]=='.') lifeMap[y0][x]='w'; if((x<gx-2||x>gx+2)&&lifeMap[y1][x]=='.') lifeMap[y1][x]='w'; }
        for(int y=y0;y<=y1;y++){ if(lifeMap[y][x0]=='.') lifeMap[y][x0]='w'; if(lifeMap[y][x1]=='.') lifeMap[y][x1]='w'; }
        for(int x=x0;x<=x1;x++){ wallMap[y0][x]=13; wallMap[y1][x]=13; } for(int y=y0;y<=y1;y++){ wallMap[y][x0]=13; wallMap[y][x1]=13; }
    }
}
