// csart.h - CUTSCENE CAST ART (module 1): every figure in the in-game look: dark tousled hair, round face, game skin / hair / top colours, a dark outline, shading and highlights.
// Replaces csFig of cutscene.h (the old one stays as csFigV1, unused). Same call: csFig(x,y,who,pose,t): 34 px tall, feet at (x,y). Needs csR csD csLn csWv from cutscene.h.
static u16 csSh(u16 c,int k){ int r=(c&31)-k,g=((c>>5)&31)-k,b=((c>>10)&31)-k; if(r<0)r=0; if(g<0)g=0; if(b<0)b=0; return RGB(r,g,b); }   // darker
static u16 csLt(u16 c,int k){ int r=(c&31)+k,g=((c>>5)&31)+k,b=((c>>10)&31)+k; if(r>31)r=31; if(g>31)g=31; if(b>31)b=31; return RGB(r,g,b); }   // lighter
static void csFig(int x,int y,int who,int pose,int t){
    //                         -    MISSY        MAMESY       DEX          HAL          OKAFOR     (the game's own skinTones / hairTones / topTones / botTones)
    static const u16 SKc[6]={0,RGB(24,16,10),RGB(24,16,10),RGB(30,23,17),RGB(19,12,7),RGB(13,8,5)};
    static const u16 HRc[6]={0,RGB(5,3,2),RGB(9,5,3),RGB(5,3,2),RGB(14,8,4),RGB(5,3,2)};
    static const u16 CLc[6]={0,RGB(8,10,26),RGB(8,20,22),RGB(5,5,8),RGB(30,16,4),RGB(29,29,30)};
    static const u16 BTc[6]={0,RGB(8,9,20),RGB(6,14,16),RGB(8,9,20),RGB(8,9,20),RGB(8,10,22)};
    u16 sk=SKc[who], hr=HRc[who], cl=CLc[who], bt=BTc[who], dk=RGB(3,2,3), ol=RGB(2,1,4);
    int dress=who==CA_MISSY||who==CA_MAME;
    if(pose==CP_LIE){ csR(x-19,y-1,36,1,RGB(2,1,3)); csR(x-11,y-7,18,7,ol); csR(x-10,y-6,17,6,cl); csR(x-10,y-6,17,1,csLt(cl,5)); csR(x+7,y-5,10,3,ol); csR(x+7,y-4,9,2,dress?sk:bt);
        csD(x-14,y-4,5,ol); csD(x-14,y-4,4,sk); csR(x-19,y-9,6,6,hr); csR(x-18,y-10,3,1,csLt(hr,7)); csR(x-15,y-5,1,1,dk); csR(x-13,y-3,2,1,RGB(24,8,8)); if(csCz>=384) csFaceBig(x-14,y-6,who,CP_LIE,t,0,SKc[who],HRc[who]); return; }
    int lean=0, bob=0, lh=-6, lv=8, rh=6, rv=8, ls=0, hd=0, open=0;
    switch(pose){
    case CP_SWAY:  lean=csWv(t,70)/2; lh=-7+csWv(t,50)/3; rh=7-csWv(t,50)/3; ls=csWv(t,70)/4; break;
    case CP_DANCE: bob=(csWv(t,16)+8)/6; lean=csWv(t,40)/2; lh=-9; lv=-9+csWv(t,16)/2; rh=9; rv=-9-csWv(t,16)/2; ls=csWv(t,16)/3; break;
    case CP_SING:  lean=csWv(t,60)/4; rh=2; rv=-8; lh=-10; lv=-3+csWv(t,30)/3; open=1; break;
    case CP_HEAD:  lean=-1; hd=2; lh=-4; lv=-9; rh=4; rv=-9; break;
    case CP_RUN:   lean=3; lh=-5+csWv(t,10)/2; rh=5-csWv(t,10)/2; lv=rv=2; ls=csWv(t,10); break;
    case CP_CLIMB: lh=-4; rh=4; lv=-8+csWv(t,24)/2; rv=-8-csWv(t,24)/2; ls=csWv(t,24)/2; break;
    case CP_FLAIL: lean=csWv(t,8)/3; lh=-9; lv=-8+csWv(t,6); rh=9; rv=-8-csWv(t,6); ls=csWv(t,6)/2; break;
    }
    int sy=y-21+bob, hx=x+lean, hy=y-29+bob+hd; u16 lc=dress?sk:bt;
    csR(x-6,y,12,1,RGB(2,1,3));                                                                                       // the floor shadow
    csR(x-4+ls,y-9,3,9,ol); csR(x+1-ls,y-9,3,9,ol); csR(x-3+ls,y-8,2,8,lc); csR(x+1-ls,y-8,2,8,lc);                       // legs (bare under a dress, trousers otherwise)
    csR(x-5+ls,y-2,4,2,dk); csR(x+1-ls,y-2,4,2,dk); csR(x-4+ls,y-2,2,1,csLt(dk,5));                                       // shoes
    u16 ac=(who==CA_MISSY||who==CA_MAME)?sk:cl;                                                                          // arms: bare, or a sleeve
    csLn(x+lean-5,sy,x+lean+lh-1,sy+lv,ol); csLn(x+lean+5,sy,x+lean+rh+1,sy+rv,ol);
    csLn(x+lean-4,sy,x+lean+lh,sy+lv,ac); csLn(x+lean-3,sy,x+lean+lh+1,sy+lv,ac); csLn(x+lean+4,sy,x+lean+rh,sy+rv,ac); csLn(x+lean+3,sy,x+lean+rh-1,sy+rv,ac);
    csR(x+lean+lh-1,sy+lv-1,2,2,sk); csR(x+lean+rh-1,sy+rv-1,2,2,sk);                                                    // hands
    for(int i=0;i<14;i++){ int w=8+i*6/13, cx=x+lean*(14-i)/14, yy=y-22+i+bob;                                            // the body, a trapezoid with an outline and a shaded side
        csR(cx-w/2-1,yy,w+2,1,ol); csR(cx-w/2,yy,w,1,cl); csR(cx-w/2,yy,2,1,csSh(cl,4)); csR(cx+w/2-2,yy,1,1,csLt(cl,3));
        if(dress&&i==13) csR(cx-w/2,yy,w,1,csLt(cl,6));                                                                  // the hem
        if(!dress&&i==9) csR(cx-w/2,yy,w,1,bt);                                                                           // the belt line
        if(who==CA_HOST&&i>0&&i<9){ csR(cx,yy,1,1,i<2?WHITE:RGB(24,4,6)); }                                              // Dex: shirt collar, red tie
        if(who==CA_DOC){ if(i<7) csR(cx,yy,1,1,csSh(cl,10)); if(i==4||i==5) csR(cx-w/2+1,yy,2,1,csSh(cl,8)); }          // Okafor: the coat seam, a pocket
        if(who==CA_CREW&&i>1&&i<12){ csR(cx-w/2+2,yy,1,1,RGB(31,30,16)); csR(cx+w/2-3,yy,1,1,RGB(31,30,16)); }          // Hal: the reflective vest stripes
        if(dress&&i<3) csR(cx-1,yy,3,1,csSh(sk,2));                                                                        // the neckline
    }
    if(csCz>=384){ csFaceBig(hx,hy+2,who,pose,t,open,sk,hr); return; }
    if(who==CA_MAME){ csR(hx-7,hy-1,3,13,ol); csR(hx+4,hy-1,3,13,ol); csR(hx-6,hy,2,11,hr); csR(hx+4,hy,2,11,hr); }      // long hair behind
    if(who==CA_MISSY){ csR(hx-6,hy-1,2,10,ol); csR(hx+5,hy-1,2,10,ol); csR(hx-6,hy,2,9,hr); csR(hx+4,hy,2,9,hr); }       // the bob, to the chin
    csD(hx,hy+1,6,ol); csD(hx,hy,5,hr); csD(hx,hy+2,4,sk);                                                                 // head: outline, hair, face
    csR(hx-3,hy-6,1,2,hr); csR(hx+2,hy-6,1,2,hr); csR(hx,hy-7,1,2,hr); csR(hx-2,hy-4,2,1,csLt(hr,8));                       // the tousled tufts and a shine (the in-game hair)
    csR(hx-4,hy+4,1,1,RGB(28,12,12)); csR(hx+4,hy+4,1,1,RGB(28,12,12));                                                   // blush
    if(who==CA_MISSY){ u16 gl=RGB(16,16,20); int gx=hx, gy=hy+1; csR(gx-4,gy-1,4,1,gl); csR(gx-4,gy+1,4,1,gl); csR(gx-4,gy,1,1,gl); csR(gx-1,gy,1,1,gl); csR(gx,gy-1,4,1,gl); csR(gx,gy+1,4,1,gl); csR(gx,gy,1,1,gl); csR(gx+3,gy,1,1,gl); csR(gx-2,gy,1,1,dk); csR(gx+1,gy,1,1,dk); csR(gx-3,gy-3,7,2,hr); }   // round glasses, flat sleepy eyes, bangs
    else { csR(hx-2,hy+1,1,1,dk); csR(hx+1,hy+1,1,1,dk); csR(hx-3,hy-1,2,1,hr); csR(hx+1,hy-1,2,1,hr); }                    // eyes, brows
    if(who==CA_HOST) csR(hx-4,hy-3,8,2,hr);                                                                               // Dex: swept fringe
    if(who==CA_CREW){ csR(hx-6,hy-3,12,3,RGB(31,31,28)); csR(hx-7,hy-1,14,1,RGB(24,24,22)); csR(hx-2,hy-4,4,1,RGB(31,31,31)); }   // Hal: the hard hat
    if(who==CA_DOC){ csLn(hx-3,hy+6,hx,hy+10,RGB(22,22,24)); csLn(hx+3,hy+6,hx,hy+10,RGB(22,22,24)); csD(hx,hy+11,1,RGB(26,26,28)); }   // Okafor: the stethoscope
    if(who==CA_MISSY&&!open){ csR(hx-1,hy+4,3,1,RGB(18,6,6)); csR(hx+2,hy+3,1,1,RGB(18,6,6)); }                            // a deadpan smirk
    else if(open) csR(hx-1,hy+4,2,2,RGB(18,2,3)); else csR(hx-1,hy+4,2,1,RGB(18,6,6));
}
