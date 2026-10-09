// wpsecret.h - WEAPONS (module 4: SECRET STASHES). Each kind of community lot hides one weapon in a far corner. Nothing marks it until you are within
// 4 tiles: then the spot twinkles. Stand next to it and press R to take it (once: the find is saved, in wpSecr). The stash is the first empty floor tile
// found walking in from the lot's corner, so it is the same every visit.
//   PARK: BAT (top right)   SKATE PARK: KNIFE (top left)   PLAZA: TASER (bottom left)   LOUNGE: PISTOL + 12 bullets (bottom right)   OLD TOWN: ROCKET + 3 missiles (bottom left)
// Only a teen or older can take one.
static int secSpot(int*tx,int*ty,int*w,int*bit){   // where this lot's stash is (0 = none on this lot)
    if(!nbOk||nbT.cur>=NB_LOTS) return 0;
    const NbLot*L=&nbT.lot[nbT.cur];
    if(!L->on||L->kind!=LKIND_COMM) return 0;
    int x0,y0,x1,y1; nbRect(L,&x0,&y0,&x1,&y1);
    int cx=x0, cy=y0; *w=0; *bit=0;
    switch(L->type){
        case CT_PARK:    *w=WP_BAT;    *bit=0; cx=x1; cy=y0; break;
        case CT_SKATE: case CT_BOTH: *w=WP_KNIFE; *bit=1; cx=x0; cy=y0; break;
        case CT_PLAZA:   *w=WP_TASER;  *bit=2; cx=x0; cy=y1; break;
        case CT_LOUNGE:  *w=WP_PISTOL; *bit=3; cx=x1; cy=y1; break;
        case CT_OLDTOWN: *w=WP_ROCKET; *bit=4; cx=x0; cy=y1; break;
        default: return 0; }
    int ix=cx==x0?1:-1, iy=cy==y0?1:-1;
    for(int d=0;d<8;d++) for(int ox=0;ox<=d;ox++) for(int oy=0;oy<=d;oy++){ if((ox>oy?ox:oy)!=d) continue;
        int x=cx+ix*ox, y=cy+iy*oy; if(x<0||y<0||x>=MW||y>=MH) continue;
        if(lifeMap[y][x]=='.'){ *tx=x; *ty=y; return 1; } }
    return 0;
}
static int secUse(void){   // R next to the stash (weapons.h wpUseSpot)
    int tx,ty,w,bit; if(!secSpot(&tx,&ty,&w,&bit)||(wpSecr>>bit&1)||stage<AG_TEEN) return 0;
    int dx=(int)(lfx>>8)-tx, dy=(int)(lfy>>8)-ty; if(dx<-1||dx>1||dy<-1||dy>1) return 0;
    wpGive(w,w==WP_PISTOL?12:0,w==WP_ROCKET?3:0); wpSecr|=(u8)(1<<bit); wpSave(); sfxPlay(SFX_STICK);
    static char b[32] EWRAM_BSS; char*e=simCat(b,"SECRET STASH  "); simCat(e,wpT[w].nm); toast(b);
    return 1;
}
static void secDraw(void){   // the twinkle, only when you are close
    int tx,ty,w,bit; if(!secSpot(&tx,&ty,&w,&bit)||(wpSecr>>bit&1)||stage<AG_TEEN) return;
    int dx=(int)(lfx>>8)-tx, dy=(int)(lfy>>8)-ty; if(dx*dx+dy*dy>16) return;
    int sx,sy; wpScr(tx*256+128,ty*256+128,2,&sx,&sy); int t=(int)((lfr>>2)&3);
    u16 c=t&1?RGB(31,28,10):RGB(31,31,31);
    px(sx,sy-t,c); px(sx-1,sy-t,c); px(sx+1,sy-t,c); px(sx,sy-t-1,c); px(sx,sy-t+1,c);
    px(sx+4,sy-4+t,RGB(24,20,6)); px(sx-5,sy-2-t,RGB(24,20,6));
}
