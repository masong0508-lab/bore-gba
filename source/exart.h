// exart.h - EXPANSION PACK BANNERS: every pack has a little animated picture, drawn by code, that opens its screens (the shelf, the mission list, the
// mission cards, the journal). exBanner(p,x,y,w,h,t,title,tag) draws the pack's scene into the box (x,y,w,h) for frame t, then its name on a darkened plate.
// The scenes work for any w >= 200 and h from 19 to 32; everything that hangs from the ground is placed from the bottom (h - n). Needs exMix (expand.h),
// xPk, rect, px, disc, line, clipSet, clipAll, text, tw and the frame buffer fb.
static u32 exH(u32 a){ a=a*2654435761u+0x9E3779B9u; a^=a<<13; a^=a>>17; a^=a<<5; return a; }   // a cheap hash: the same stars every frame
static const signed char exS64[64]={0,12,25,37,49,60,71,81,90,98,106,112,117,122,125,126,127,126,125,122,117,112,106,98,90,81,71,60,49,37,25,12,0,-12,-25,-37,-49,-60,-71,-81,-90,-98,-106,-112,-117,-122,-125,-126,-127,-126,-125,-122,-117,-112,-106,-98,-90,-81,-71,-60,-49,-37,-25,-12};
static int exSn(int t,int per){ int p=t%per; if(p<0) p+=per; return exS64[(p*64/per)&63]; }   // a smooth wave, -127..127, once round in per frames
#define R_(x,y,w,h,c) rect(X+(x),Y+(y),(w),(h),(c))
#define P_(x,y,c) px(X+(x),Y+(y),(c))
#define D_(x,y,r,c) disc(X+(x),Y+(y),(r),(c))
#define L_(x0,y0,x1,y1,c) line(X+(x0),Y+(y0),X+(x1),Y+(y1),(c))
static void exSky(int X,int Y,int BW,int BH,u16 a,u16 b){ for(int j=0;j<BH;j++) rect(X,Y+j,BW,1,exMix(a,b,BH>1?j*16/(BH-1):0)); }

// ---- 1 BORE-OUT MAN!: stars, the Earth turning, the moon, a rocket climbing with a smoke trail ----
static void exScSpace(int X,int Y,int BW,int BH,int t){
    exSky(X,Y,BW,BH,RGB(0,0,2),RGB(2,4,13));
    for(int i=0;i<44;i++){ u32 h=exH((u32)i+11); int sx=(int)(h%(u32)BW), sy=(int)((h>>12)%(u32)(BH-3)), ph=((t>>3)+i)&7; u16 c=ph<5?RGB(26,26,31):ph<7?RGB(14,14,22):RGB(7,7,12);
        P_(sx,sy,c); if(i%9==0&&ph<5){ u16 g=RGB(10,10,18); P_(sx-1,sy,g); P_(sx+1,sy,g); P_(sx,sy-1,g); P_(sx,sy+1,g); } }
    int cx=BW-34, cy=BH+30, R=46;                                  // the Earth: only its rim shows, the land and the clouds slide across it
    D_(cx,cy,R+2,RGB(5,11,26)); D_(cx,cy,R+1,RGB(9,19,31)); D_(cx,cy,R,RGB(2,7,22));
    for(int k=0;k<10;k++){ int bx=cx+R-((t/4+k*29)%(2*R)), by=cy-R+5+(k*7)%13, dx=bx-cx, dy=by-cy; if(dx*dx+dy*dy<(R-7)*(R-7)){ D_(bx,by,2+(k&1),k%3==2?RGB(27,28,29):RGB(4,18,7)); if(k%3==0) D_(bx+2,by+1,1,RGB(11,20,6)); } }
    D_(36,BH/2,4,RGB(23,23,24)); D_(35,BH/2-1,1,RGB(17,17,19)); D_(38,BH/2+2,1,RGB(18,18,20));   // the moon
    int u=(t/2)%(BW+40), rx=u-20, ry=BH-5-u*(BH-10)/(BW+40);        // the rocket, up and to the right
    for(int i=1;i<14;i++){ int tx=rx-i*3, ty=ry+3+i*(BH-10)*3/(BW+40)+((i&1)?1:0); u16 c=i<5?RGB(30,24,10):i<9?RGB(22,22,24):RGB(12,12,16); P_(tx,ty,c); if(i<9) P_(tx,ty+1,c); }
    int f=2+((t>>1)&3); R_(rx-1,ry+1,3,f,RGB(31,19,4)); R_(rx,ry+1,1,f+1,RGB(31,30,13));
    R_(rx-1,ry-8,3,9,RGB(28,28,30)); R_(rx,ry-11,1,3,RGB(31,8,6)); R_(rx-1,ry-8,1,9,RGB(21,21,25)); R_(rx-2,ry-3,1,4,RGB(25,6,6)); R_(rx+2,ry-3,1,4,RGB(25,6,6)); P_(rx,ry-6,RGB(6,22,31));
}
// ---- 2 BOREZIES!: a green moon behind drifting clouds, a graveyard, hands coming up out of the ground, fog, a bat ----
static void exScZomb(int X,int Y,int BW,int BH,int t){
    exSky(X,Y,BW,BH,RGB(0,2,2),RGB(4,9,6));
    D_(BW-46,BH/2-2,11,RGB(5,9,6)); D_(BW-46,BH/2-2,9,RGB(26,29,20)); D_(BW-49,BH/2-4,2,RGB(20,23,15)); D_(BW-43,BH/2+1,2,RGB(21,24,16)); D_(BW-45,BH/2-8,1,RGB(20,23,15));
    for(int k=0;k<3;k++){ int cx=((t/5+k*90)%(BW+60))-30, cy=3+k*(BH/4); R_(cx,cy,26,2,RGB(2,4,3)); R_(cx+4,cy-1,16,1,RGB(2,4,3)); R_(cx+6,cy+2,18,1,RGB(2,4,3)); }
    int gy=BH-7; R_(0,gy,BW,7,RGB(1,2,1));
    static const u8 gx[8]={14,41,70,98,126,152,181,209};
    for(int i=0;i<8;i++){ int x=gx[i], hh=5+(i%3)*2; R_(x-3,gy-hh,7,hh,RGB(3,5,4)); D_(x,gy-hh,3,RGB(3,5,4)); R_(x-1,gy-hh+1,3,1,RGB(7,10,8));
        if(i&1){ R_(x,gy-hh-6,1,6,RGB(3,5,4)); R_(x-2,gy-hh-4,5,1,RGB(3,5,4)); } }
    for(int i=0;i<6;i++){ int x=(gx[(i*3+1)&7]+gx[(i*3+2)&7])/2+2, ph=((t>>1)+i*17)%64, up=exS64[ph]>0?exS64[ph]*9/127:0; if(up<1) continue;
        u16 gr=RGB(9,21,8), dk=RGB(5,12,5); R_(x,gy-up,2,up+1,gr); R_(x+2,gy-up+1,1,up,dk);
        R_(x-1,gy-up-3,1,3,gr); R_(x+1,gy-up-4,1,4,gr); R_(x+2,gy-up-3,1,3,gr); if(up>5) P_(x,gy-up-1,RGB(26,28,20)); }
    for(int x=0;x<BW;x+=2) if((((x>>1)+(t>>2))&3)==0){ P_(x,gy-2+((x>>3)&1),RGB(8,12,9)); P_(x+1,gy-3,RGB(5,9,6)); }
    int bx=(t*2)%(BW+60)-30, by=6+exSn(t,44)/36; u16 bt=RGB(1,1,2); R_(bx,by,3,2,bt);
    if((t>>2)&1){ L_(bx-1,by,bx-4,by-2,bt); L_(bx+3,by,bx+6,by-2,bt); } else { L_(bx-1,by,bx-4,by+2,bt); L_(bx+3,by,bx+6,by+2,bt); }
}
// ---- 3 WORLD WAR BORE: a dusk battlefield: the sun on the hills, smoke columns, a tank that fires, two searchlights, flashes on the horizon ----
static void exScWar(int X,int Y,int BW,int BH,int t){
    exSky(X,Y,BW,BH,RGB(4,1,5),RGB(28,13,4));
    D_(BW-96,BH-4,10,RGB(31,18,5)); D_(BW-96,BH-4,8,RGB(31,25,9));
    for(int s=0;s<2;s++){ int sx=22+s*(BW-44), a=exSn(t+s*60,170)/5*(s?-1:1); u16 b=RGB(17,18,22);   // the searchlights sweep across
        for(int o=-1;o<=1;o++) L_(sx+o,BH-3,sx+a*2+o*3+(s?-30:30),-2,b); }
    for(int c=0;c<3;c++){ int bx=44+c*70+(c&1)*12; for(int k=0;k<8;k++){ int yy=BH-8-k*(BH/8+1)-((t>>2)&3), xx=bx+exSn(t+k*9+c*23,100)/40+k, r=2+k/2; D_(xx,yy,r,k<3?RGB(8,3,3):RGB(6,4,5)); } }
    if((t%150)<10){ int fx=30+(t/150*61)%(BW-60), r=2+((t%150)<5?3:1); D_(fx,BH-7,r,RGB(31,27,9)); D_(fx,BH-7,r-1>0?r-1:1,RGB(31,31,24)); }   // a flash far off
    for(int x=0;x<BW;x++){ int hh=5+(exSn(x*3,80)+127)/50+(exSn(x*7+30,40)+127)/90; R_(x,BH-hh,1,hh,RGB(1,1,2)); }
    int tx=BW-62; R_(tx-1,BH-6,19,2,RGB(1,1,1)); R_(tx,BH-9,17,4,RGB(3,4,3)); R_(tx+4,BH-12,8,3,RGB(3,4,3));
    int rc=(t%110)<3?-2:0; L_(tx+12+rc,BH-11,tx+22+rc,BH-12,RGB(2,2,2)); L_(tx+12+rc,BH-10,tx+22+rc,BH-11,RGB(2,2,2));
    if((t%110)<5){ D_(tx+24,BH-12,3,RGB(31,28,10)); D_(tx+24,BH-12,1,RGB(31,31,26)); }
    if((t%110)>=5&&(t%110)<40){ int sx=tx+24+((t%110)-5)*5, sy=BH-12-((t%110)-5)/6; P_(sx,sy,RGB(31,30,20)); P_(sx-1,sy,RGB(24,16,6)); P_(sx-2,sy,RGB(14,8,4)); }
}
// ---- 4 BORTUA FIGHTER: an arena: two spotlights sweeping, a crowd, two fighters trading punches ----
static void exFighter(int X,int Y,int x,int fy,int dir,u16 c1,u16 c2,int arm){   // feet at (x,fy), facing dir (+1 right); arm: how far the front fist is out
    R_(x-2,fy-6,2,6,c2); R_(x+1,fy-6,2,6,c2); R_(x-3,fy-14,7,8,c1); D_(x,fy-18,3,RGB(26,19,13)); R_(x-2,fy-20,5,2,RGB(3,2,2));
    R_(x+dir*3-(dir<0?1:0),fy-12,2,2+arm*0,c1); if(arm>0){ R_(dir>0?x+3:x-3-arm,fy-12,arm,2,RGB(26,19,13)); R_(dir>0?x+3+arm:x-5-arm,fy-13,3,4,RGB(31,12,6)); } else { R_(dir>0?x+3:x-5,fy-12,3,2,RGB(26,19,13)); }
}
static void exScFight(int X,int Y,int BW,int BH,int t){
    exSky(X,Y,BW,BH,RGB(2,0,1),RGB(10,3,2));
    for(int s=0;s<2;s++){ int ox=70+s*95+exSn(t+s*50,150)/5; for(int j=0;j<BH-5;j++){ int half=2+j*2/3; for(int x=ox-half;x<ox+half;x++) if(((x+j)&1)==0) P_(x,j,RGB(12,8,5)); } }
    R_(0,BH-6,BW,6,RGB(5,1,1)); R_(0,BH-6,BW,1,RGB(20,6,4)); for(int x=0;x<BW;x+=14) R_(x+((t>>3)&7),BH-3,6,1,RGB(9,3,2));
    int ph=t%180, a=(ph<90)?ph%45:0, b=(ph>=90)?ph%45:0;                                            // A throws a punch for 90 frames, then B
    int ea=a<8?a:a<16?16-a:0, eb=b<8?b:b<16?16-b:0;
    int ax=BW-100, bx=BW-52; exFighter(X,Y,ax,BH-3,1,RGB(8,20,31),RGB(4,6,14),ea>1?ea:0); exFighter(X,Y,bx,BH-3,-1,RGB(31,9,6),RGB(12,3,3),eb>1?eb:0);
    if(ea==8||eb==8){ int ix=(ax+bx)/2, iy=BH-15; u16 s=RGB(31,28,8); P_(ix,iy,s); L_(ix-4,iy,ix+4,iy,s); L_(ix,iy-4,ix,iy+4,s); L_(ix-3,iy-3,ix+3,iy+3,RGB(31,13,4)); L_(ix-3,iy+3,ix+3,iy-3,RGB(31,13,4)); }
    for(int x=3;x<BW;x+=9){ int sw=exSn(t+x*5,60)/90; D_(x+sw,BH-1,2,RGB(1,0,1)); }
    int pu=(t>>3)&1; text(X+BW-12-tw("VS",1)+0,Y+2,"VS",pu?RGB(31,26,6):RGB(31,10,6),1);
}
// ---- 5 BOREAGOTCHI!: a pastel LCD with three pocket pets bouncing, an egg rocking, hearts floating up ----
static void exScGotchi(int X,int Y,int BW,int BH,int t){
    exSky(X,Y,BW,BH,RGB(27,21,28),RGB(19,15,28));
    for(int y=1;y<BH;y+=3) for(int x=1+(y/3&1);x<BW;x+=3) P_(x,y,RGB(23,18,26));
    R_(0,BH-3,BW,3,RGB(13,10,19)); R_(0,BH-3,BW,1,RGB(24,19,30));
    int ex=BW-190+4; R_(ex-4+((exSn(t,60)>40)?1:0),BH-14,9,11,RGB(31,30,27)); D_(ex+((exSn(t,60)>40)?1:0),BH-14,4,RGB(31,30,27)); R_(ex-2,BH-9,2,2,RGB(10,24,28)); R_(ex+1,BH-6,2,2,RGB(30,14,18)); R_(ex-1,BH-12,2,1,RGB(30,14,18));
    static const u16 pc[3]={RGB(30,26,5),RGB(8,26,30),RGB(10,28,13)};
    for(int i=0;i<3;i++){ int x=BW-132+i*46, bob=exSn(t*2+i*21,64)>0?exSn(t*2+i*21,64)*5/127:0, y=BH-9-bob; int bl=((t+i*23)%70)<3;
        R_(x-4,BH-3,9,1,RGB(9,6,14)); D_(x,y,5,pc[i]); R_(x-1,y-8,1,3,pc[i]); R_(x+1,y-8,1,3,pc[i]);
        if(bl){ R_(x-3,y-1,2,1,RGB(3,2,5)); R_(x+1,y-1,2,1,RGB(3,2,5)); } else { R_(x-3,y-2,2,2,RGB(3,2,5)); R_(x+1,y-2,2,2,RGB(3,2,5)); P_(x-3,y-2,RGB(31,31,31)); P_(x+1,y-2,RGB(31,31,31)); }
        R_(x-1,y+2,3,1,RGB(24,6,8)); R_(x-3,y+4,2,2,pc[i]); R_(x+2,y+4,2,2,pc[i]);
        int hp=(t+i*40)%120; if(hp<48){ int hx=x+3+hp/12, hy=y-10-hp/3; u16 hc=hp<30?RGB(31,8,12):RGB(24,14,20); P_(hx,hy,hc); P_(hx+2,hy,hc); R_(hx-1,hy+1,5,1,hc); R_(hx,hy+2,3,1,hc); P_(hx+1,hy+3,hc); } }
}
// ---- 6 BOREPILOGUE: sepia dust and a big hourglass: the sand runs down all the way, then it turns ----
static void exScEpi(int X,int Y,int BW,int BH,int t){
    exSky(X,Y,BW,BH,RGB(7,4,2),RGB(21,14,7));
    for(int i=0;i<14;i++){ int mx=(int)(exH((u32)i+3)%(u32)BW), my=BH-((t/5+i*7+(int)(exH((u32)i)%9u))%(BH+4)); P_(mx,my,RGB(27,21,11)); }
    int cx=BW-44, hh=(BH-6)/2; if(hh>13) hh=13; int cyc=t%420, flip=cyc>=390, p=flip?0:cyc, L=hh*(390-p)/390, Lb=hh*p/390; if(flip){ L=0; Lb=hh; }
    int top=2; R_(cx-10,top,21,2,RGB(18,11,5)); R_(cx-10,top+2*hh+3,21,2,RGB(18,11,5)); R_(cx-8,top,1,2*hh+5,RGB(11,7,3)); R_(cx+8,top,1,2*hh+5,RGB(11,7,3));
    for(int j=0;j<hh;j++){ int w=7-j*7/hh; R_(cx-w,top+2+j,2*w+1,1,RGB(14,12,12)); R_(cx-w,top+2+hh+j,2*(j*7/hh)+1,1,RGB(14,12,12)); }
    int tw0=(hh-L>=0)?hh-L:0; for(int j=tw0;j<hh;j++){ int w=7-j*7/hh; if(w>0) R_(cx-w+1,top+2+j,2*w-1,1,RGB(28,21,8)); }
    for(int j=hh-Lb;j<hh;j++){ int w=j*7/hh; if(j>=0&&w>0) R_(cx-w+1,top+2+hh+j,2*w-1,1,RGB(28,21,8)); }
    if(!flip&&L>0) for(int i=0;i<hh;i++) if(((t>>1)+i)%3==0) P_(cx,top+2+hh+i-Lb+0,RGB(31,25,10));
    int ck=BW-130, cy=BH/2; D_(ck,cy,BH/2-3,RGB(14,9,5)); D_(ck,cy,BH/2-5,RGB(23,17,10)); L_(ck,cy,ck+((exSn(t*2,64)*(BH/2-7))/127),cy-(exSn(t*2+16,64)*(BH/2-7))/127,RGB(5,3,2)); L_(ck,cy,ck+(exSn(t/6,64)*(BH/2-9))/127,cy-(exSn(t/6+16,64)*(BH/2-9))/127,RGB(5,3,2));
}
// ---- 7 BOREFORE TIME: a sunrise over hills, drifting clouds, a castle with a flag, tiny folk walking ----
static void exScKing(int X,int Y,int BW,int BH,int t){
    exSky(X,Y,BW,BH,RGB(7,17,29),RGB(28,29,21));
    D_(BW-66,BH/2,9,RGB(31,31,19)); D_(BW-66,BH/2,7,RGB(31,31,26));
    for(int k=0;k<3;k++){ int cx=(t/(5+k*2)+k*100)%(BW+60)-30, cy=3+k*(BH/4); D_(cx,cy+2,3,RGB(30,31,31)); D_(cx+4,cy,4,RGB(31,31,31)); D_(cx+9,cy+2,3,RGB(29,30,31)); R_(cx,cy+2,10,3,RGB(30,31,31)); }
    for(int x=0;x<BW;x++){ int hh=8+(exSn(x*2+11,120)+127)/36; R_(x,BH-hh,1,hh,RGB(8,17,17)); }
    int cx=BW-84, by=BH-8; u16 st=RGB(15,15,18), dk=RGB(8,8,11);
    R_(cx-11,by-10,6,11,st); R_(cx+6,by-10,6,11,st); R_(cx-4,by-14,10,15,st); R_(cx-4,by-14,1,15,dk);
    for(int i=0;i<3;i++){ R_(cx-11+i*3,by-12,2,2,st); R_(cx+6+i*3,by-12,2,2,st); } for(int i=0;i<4;i++) R_(cx-4+i*3,by-16,2,2,st);
    R_(cx,by-4,3,5,RGB(5,3,2)); P_(cx-9,by-7,RGB(31,27,8)); P_(cx+9,by-7,RGB(31,27,8)); P_(cx,by-10,RGB(31,27,8));
    R_(cx+1,by-23,1,7,RGB(6,5,5)); for(int i=0;i<5;i++) R_(cx+2+i,by-23+exSn(t*2+i*8,32)/60,1,3,RGB(31,7,5));
    for(int x=0;x<BW;x++){ int hh=4+(exSn(x*3,90)+127)/45; R_(x,BH-hh,1,hh,RGB(6,16,5)); if(((x+hh)&7)==0) P_(x,BH-hh,RGB(9,22,7)); }
    for(int k=0;k<3;k++){ int vx=(t/4+k*77)%BW, hh=4+(exSn(vx*3,90)+127)/45; P_(vx,BH-hh-2,RGB(31,22,10)); P_(vx,BH-hh-1,RGB(10,12,24)); P_(vx+((t>>3)&1),BH-hh,RGB(8,6,4)); }
}
// ---- the base game's missions: a quiet street at dawn ----
static void exScBase(int X,int Y,int BW,int BH,int t){
    exSky(X,Y,BW,BH,RGB(9,17,28),RGB(30,25,18)); D_(40,BH-8,7,RGB(31,29,17));
    for(int i=0;i<8;i++){ int hx=10+i*30, hh=9+(i*7)%7; R_(hx,BH-hh-3,18,hh+3,(i&1)?RGB(14,9,7):RGB(11,10,15)); for(int q=0;q<3;q++) R_(hx+q*5+2,BH-hh,3,3,((t>>5)+i+q)%5?RGB(30,26,10):RGB(8,8,12)); }
    R_(0,BH-3,BW,3,RGB(7,7,9));
}
static void exScene(int p,int X,int Y,int BW,int BH,int t){
    switch(p){ case 0: exScSpace(X,Y,BW,BH,t); break; case 1: exScZomb(X,Y,BW,BH,t); break; case 2: exScWar(X,Y,BW,BH,t); break; case 3: exScFight(X,Y,BW,BH,t); break;
        case 4: exScGotchi(X,Y,BW,BH,t); break; case 5: exScEpi(X,Y,BW,BH,t); break; case 7: exScKing(X,Y,BW,BH,t); break; default: exScBase(X,Y,BW,BH,t); break; }
}
#undef R_
#undef P_
#undef D_
#undef L_
static void exDarken(int x,int y,int w,int h,int fade){   // halve the light in a box (a plate under the lettering); the last `fade` columns thin out to every other pixel
    for(int j=0;j<h;j++){ int yy=y+j; if(yy<0||yy>=SH) continue; for(int i=0;i<w;i++){ int xx=x+i; if(xx<0||xx>=SW) continue; if(i>=w-fade&&((i+j)&1)) continue; u16*q=&fb[yy*SW+xx]; *q=(u16)((*q>>1)&0x3DEF); } }
}
// the banner: the pack's scene, then its name (big on a tall banner) on a darkened plate; tag goes small at the right
static void exBanner(int p,int x,int y,int w,int h,int t,const char*title,const char*tag){
    clipSet(x,y,x+w,y+h);
    exScene(p,x,y,w,h,t);
    int big=h>=19, tx=x+8;
    int pw=tw(title,big?2:1)+22; if(pw>w-40) pw=w-40; exDarken(x,y,pw,h,10);
    if(tag){ int tg=tw(tag,1)+10; exDarken(x+w-tg,y,tg,10,4); text(x+w-tg+5,y+2,tag,RGB(31,26,6),1); }
    if(big){ text(tx+1,y+h/2-6,title,RGB(1,1,3),2); text(tx,y+h/2-7,title,WHITE,2); if(h>=26) text(tx,y+h-9,xPk[p].sub,exMix(xPk[p].b,WHITE,6),1); }
    else text(tx,y+h/2-3,title,WHITE,1);
    rect(x,y+h-1,w,1,exMix(xPk[p].b,WHITE,5));
    clipAll();
}
