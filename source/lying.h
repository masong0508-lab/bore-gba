// lying.h - THE SLEEPING POSE. While you sleep in a bed your Bore lies on it: head on the pillow (the head of the baked sprite, laid flat and squeezed onto the
// pillow), the quilt over the body rising and falling as it breathes, and Zs drifting up. Drawn in the room loop at the bed's own tile (painter order), so
// the walls and furniture in front of the bed still hide it. Tents, cribs and the like (not in a BEDS tab) keep the old slump. The picture only.
static u8 lieOn, lieX, lieY; static int lieSX, lieSY;   // asleep on a bed: its map tile and where it is on the screen
static int bedWallAt(int x,int y){ if(x<0||y<0||x>=MW||y>=MH) return 1; return !itemOpen(x,y)&&lifeMap[y][x]!='S'; }
static int bedFacing(int x,int y){   // a bed turns its head to the wall even when another bed stands beside it
    static const signed char dx[4]={0,1,0,-1}, dy[4]={1,0,-1,0};
    for(int d=0;d<4;d++) if(itemOpen(x+dx[d],y+dy[d])&&bedWallAt(x-dx[d],y-dy[d])) return d;
    return itemFacing(x,y);
}
static int bedJoin(int x,int y,int id,int d){   // 0 alone, 1 its partner is on the +a side, 2 on the -a side (same look, same facing, side by side)
    static const signed char dx[4]={0,1,0,-1}, dy[4]={1,0,-1,0};
    for(int k=0;k<2;k++){ int e=(d+1+2*k)&3, nx=x+dx[e], ny=y+dy[e]; if(nx<0||ny<0||nx>=MW||ny>=MH) continue;
        if(lifeMap[ny][nx]=='S'&&catIdAt(nx,ny)==id&&bedFacing(nx,ny)==d) return 1+k; }
    return 0;
}
static int lieFace(int x,int y){ int id=catIdAt(x,y); return (id&&catTab[id-1].rot==2)?bedFacing(x,y):itemFacing(x,y); }
static void lieTick(void){
    if(simAct!=1||ldead){ lieOn=0; return; }
    if(!lieOn){ int px=(int)(lfx>>8), py=(int)(lfy>>8), best=1<<30, bx=-1, by=0;
        for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){ int tx=px+dx, ty=py+dy; if(tx<0||ty<0||tx>=MW||ty>=MH||lifeMap[ty][tx]!='S') continue;
            int id=catIdAt(tx,ty); if(id&&catTab[id-1].cat!=CAT_C_BEDS&&catTab[id-1].cat!=CAT_C_BEDS_2) continue;
            int ex=((tx*256+128)-(int)lfx)>>4, ey=((ty*256+128)-(int)lfy)>>4, d=ex*ex+ey*ey; if(d<best){ best=d; bx=tx; by=ty; } }
        if(bx<0) return; lieOn=1; lieX=(u8)bx; lieY=(u8)by; }
    int rx,ry; switch(cview){ case 0:rx=lieX;ry=lieY;break; case 1:rx=MW-1-lieY;ry=lieX;break; case 2:rx=MW-1-lieX;ry=MH-1-lieY;break; default:rx=lieY;ry=MH-1-lieX; }
    lieSX=LOX+(rx-ry)*CA; lieSY=LOY+(rx+ry+1)*CB;
}
static void lieZ(int x,int y,int s){ u16 w=RGB(31,31,31), o=RGB(8,10,20);   // a little Z, s = 2 or 3 px
    for(int i=0;i<s+2;i++){ int xs[3]={0,1,2}; (void)xs; }
    for(int i=0;i<s;i++){ px(x+i,y,w); px(x+i,y+s-1,w); px(x+s-1-i,y+i,w); } px(x-1,y,o); }
static void lieDraw(int sx,int sy){   // sx, sy: the bed tile's centre
    int d=lieFace(lieX,lieY), r=(d-cview)&3, br=(int)((lfr>>5)&1), kz=0;
    const u8*s=spr4[3]; (void)kz;
    int cr=RGB(6,10,26), id=catIdAt(lieX,lieY); if(id) cr=catTab[id-1].col; else cr=RGB(10,14,28);
    u16 qa=(u16)cr, qb=(u16)(((cr&31)*3/4)|((((cr>>5)&31)*3/4)<<5)|((((cr>>10)&31)*3/4)<<10));
    for(int Y=sy-24;Y<sy+8;Y++) for(int X=sx-16;X<sx+16;X++){
        if((unsigned)(X-cX0)>=cW||(unsigned)(Y-cY0)>=cH) continue;
        for(int pass=0;pass<2;pass++){   // pass 0: the quilt (mattress top, breathing), pass 1: the head (on the pillow)
            int z2=pass?13:(11+br), dx=X-sx, sum=2*(Y-sy+4)+z2, u=4*(dx+sum), v=4*(sum-dx);   // model units in eighths (z2 = 2 z)
            int a,b; switch(r){ case 0:a=u;b=v;break; case 1:b=u;a=64-v;break; case 2:a=64-u;b=64-v;break; default:a=v;b=64-u; }
            if(pass==0){ if(a>=8&&a<=56&&b>=29&&b<=62){ fb[Y*SW+X]=((a>>3)+(b>>3))&1?qa:qb; if(b<32||a<12||a>52) fb[Y*SW+X]=qb; } }
            else if(a>=14&&a<=50&&b>=6&&b<=34){   // the head: sprite columns across the bed, rows along it
                int c=0, sxs=16+((a-32)*9>>4), sys=spBy0+((b-6)*6>>3);
                for(int k=0;k<4&&!c;k++){ int xx=sxs+(k&1), yy=sys+(k>>1); if(xx>=0&&xx<SPW&&yy>=0&&yy<SPH) c=s[yy*SPW+xx]; }
                if(c) fb[Y*SW+X]=sprPal[c]; }
        }
    }
    int ph=(int)((lfr>>4)&3); lieZ(sx+5+ph,sy-14-ph*3,2+(ph>>1)); lieZ(sx+10-(ph&1),sy-22-ph,3);
}
