// csart.h - CUTSCENE CAST ART (module 1): every figure in the in-game look: dark tousled hair, round face, game skin / hair / top colours, a dark outline, shading and highlights.
// Replaces csFig of cutscene.h (the old one stays as csFigV1, unused). Same call: csFig(x,y,who,pose,t): 34 px tall, feet at (x,y). Needs csR csD csLn csWv from cutscene.h.
static u16 csSh(u16 c,int k){ int r=(c&31)-k,g=((c>>5)&31)-k,b=((c>>10)&31)-k; if(r<0)r=0; if(g<0)g=0; if(b<0)b=0; return RGB(r,g,b); }   // darker
static u16 csLt(u16 c,int k){ int r=(c&31)+k,g=((c>>5)&31)+k,b=((c>>10)&31)+k; if(r>31)r=31; if(g>31)g=31; if(b>31)b=31; return RGB(r,g,b); }   // lighter
static void csFig(int x,int y,int who,int pose,int t){
    //                         -    MISSY        MAMESY       DEX          HAL          OKAFOR     (the game's own skinTones / hairTones / topTones / botTones)
    static const u16 SKc[6]={0,RGB(24,16,10),RGB(24,16,10),RGB(30,23,17),RGB(19,12,7),RGB(13,8,5)};
    static const u16 HRc[6]={0,RGB(5,3,2),RGB(14,8,4),RGB(5,3,2),RGB(14,8,4),RGB(5,3,2)};
    static const u16 CLc[6]={0,RGB(8,10,26),RGB(8,20,22),RGB(8,9,14),RGB(30,16,4),RGB(29,29,30)};   // blazer, teal top, navy suit jacket, hi-vis vest, white coat
    static const u16 BTc[6]={0,RGB(7,8,15),RGB(9,13,23),RGB(4,4,7),RGB(17,14,8),RGB(8,12,20)};   // pencil skirt, jeans, suit trousers, work trousers, scrub trousers
    u16 sk=SKc[who], hr=HRc[who], cl=CLc[who], bt=BTc[who], dk=RGB(3,2,3), ol=RGB(2,1,4);
    int mv=0; if(pose==CP_WALK){ if(t<60){ x+=(x>120?60-t:t-60); mv=1; } } else if(pose==CP_LEAVE){ if(t>45){ x+=(x>120?t-45:45-t); mv=1; } }   // cutscene redo 10: walk in from, and out toward, the nearer side
    int dress=who==CA_MISSY;   /* (Mamesy wears a top and jeans, like in the game) */
    if(pose==CP_LIE||pose==CP_STIR){ int st=pose==CP_STIR, lf=st?(csWv(t,30)+8)/3:0, br=csWv(t,100)>4; csR(x-19,y-1,36,1,RGB(2,1,3)); csR(x-11,y-7,18,7,ol); csR(x-10,y-6,17,6,cl); csR(x-10,y-6,17,1,csLt(cl,5)); if(br) csR(x-8,y-8,13,1,cl); csR(x+7,y-5-lf,10,3,ol); csR(x+7,y-4-lf,9,2,dress?sk:bt); if(st) csR(x+17,y-5-lf+((t>>2)&1),1,2,sk);   // (cutscene redo 10: she breathes; STIR lifts her hand and her fingers move)
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
    // cutscene redo 10: TALK gestures and the mouth moves while the caption types; LAUGH bounces; CRY hides the face and heaves; POINT at the other figure; SHOCK jolts back with the arms up;
    // WALK / LEAVE step in or out; SLUMP sinks and hangs the head
    case CP_TALK:  lean=csWv(t,50)/6; lh=-7; rh=8; rv=-4+csWv(t,14)/3; open=csTalking&&((t>>2)&1); break;
    case CP_LAUGH: bob=(csWv(t,8)+8)/5; lean=csWv(t,8)/5; hd=-1; lh=-4; lv=-2; rh=4; rv=-2; open=1; break;
    case CP_CRY:   bob=(csWv(t,12)+8)/8; hd=3; lean=-1; lh=-2; lv=-6; rh=2; rv=-6; break;
    case CP_POINT: lean=csDir*2; if(csDir>0){ rh=13; rv=-5; lh=-6; } else { lh=-13; lv=-5; rh=6; } open=csTalking&&((t>>2)&1); break;
    case CP_SHOCK: bob=t<6?-2:0; lean=-csDir*3; hd=-1; ls=2; lh=-9; lv=-12; rh=9; rv=-12; open=1; break;
    case CP_WALK: case CP_LEAVE: if(mv){ lean=1; lh=-5+csWv(t,14)/2; rh=5-csWv(t,14)/2; ls=csWv(t,14)/2; } break;
    case CP_SLUMP: bob=t/4>7?7:t/4; hd=3; lean=-1; break;
    }
    int sy=y-21+bob, hx=x+lean, hy=y-29+bob+hd; u16 lc=dress?sk:bt; int bl=((t+who*23)%110)<4;   // bl: a blink every ~2 s (cutscene redo 10)
    int fem=(who==CA_MISSY||who==CA_MAME), ax=fem?5:6;   // the build: the women slimmer through the shoulders, the men broader (the arms hang from the edge of the shoulders)
    csR(x-6,y,12,1,RGB(2,1,3));                                                                                       // the floor shadow
    csR(x-4+ls,y-12,3,12,ol); csR(x+1-ls,y-12,3,12,ol); csR(x-3+ls,y-11,2,11,lc); csR(x+1-ls,y-11,2,11,lc);               // legs: a third of the figure, trousers or jeans (bare under Missy's skirt)
    csR(x-5+ls,y-2,4,2,dk); csR(x+1-ls,y-2,4,2,dk); csR(x-4+ls,y-2,2,1,csLt(dk,5));                                       // shoes
    u16 ac=(who==CA_CREW)?RGB(14,14,16):cl;                                                                // arms: a sleeve (Hal: a grey work shirt under the vest)
    csLn(x+lean-ax,sy,x+lean+lh-1,sy+lv,ol); csLn(x+lean+ax,sy,x+lean+rh+1,sy+rv,ol);
    csLn(x+lean-ax+1,sy,x+lean+lh,sy+lv,ac); csLn(x+lean-ax+2,sy,x+lean+lh+1,sy+lv,ac); csLn(x+lean+ax-1,sy,x+lean+rh,sy+rv,ac); csLn(x+lean+ax-2,sy,x+lean+rh-1,sy+rv,ac);
    csR(x+lean+lh-1,sy+lv-1,2,2,sk); csR(x+lean+rh-1,sy+rv-1,2,2,sk);                                                    // hands
    for(int i=0;i<11;i++){ int w=i<3?(fem?10:12):i<7?(fem?9:11):(fem?8:10), cx=x+lean*(11-i)/11, yy=y-22+i+bob;           // the body: straight, shoulders to hips, with an outline and a shaded side
        u16 rc=(who==CA_MISSY&&i>=9)?bt:cl;                                                                              // Missy: the blazer ends at the hip, the skirt starts
        csR(cx-w/2-1,yy,w+2,1,ol); csR(cx-w/2,yy,w,1,rc); csR(cx-w/2,yy,2,1,csSh(rc,4)); csR(cx+w/2-2,yy,1,1,csLt(rc,3));
        if(who==CA_MISSY){ if(i<4) csR(cx-(i<2?1:0),yy,i<2?3:1,1,WHITE); if(i==6) csR(cx,yy,1,1,csLt(cl,9)); }          // a white blouse in the V of the blazer, one button
        if(who==CA_MAME){ if(i<2) csR(cx-1,yy,3,1,csSh(sk,2)); if(i==10) csR(cx-w/2,yy,w,1,csSh(cl,3)); }                 // a round neck, a plain hem
        if(who==CA_HOST){ if(i>=1&&i<7){ csR(cx-2,yy,1,1,csLt(cl,5)); csR(cx+2,yy,1,1,csLt(cl,5)); csR(cx-1,yy,1,1,WHITE); csR(cx+1,yy,1,1,WHITE); } if(i<2) csR(cx-1,yy,3,1,WHITE); if(i>=1&&i<8) csR(cx,yy,1,1,RGB(24,4,6)); }   // Dex: lapels, white shirt, red tie
        if(who==CA_DOC){ if(i<2) csR(cx-1,yy,3,1,RGB(8,18,20)); if(i>=2&&i<10) csR(cx,yy,1,1,csSh(cl,10)); if(i==4||i==5) csR(cx-w/2+1,yy,2,1,csSh(cl,8)); }   // Okafor: scrubs at the neck, the coat seam, a pocket
        if(who==CA_CREW){ if(i<2) csR(cx-1,yy,3,1,RGB(14,14,16)); if(i>1&&i<10){ csR(cx-w/2+2,yy,1,1,RGB(31,30,16)); csR(cx+w/2-3,yy,1,1,RGB(31,30,16)); } }   // Hal: the grey shirt, the reflective vest stripes
    }
    if(who==CA_DOC) for(int k=0;k<7;k++){ int yy=y-11+k; csR(x-6,yy,12,1,ol); csR(x-5,yy,10,1,cl); csR(x-5,yy,2,1,csSh(cl,4)); csR(x,yy,1,1,csSh(cl,10)); }   // the white coat hangs to the knee
    if(who==CA_MISSY) for(int k=0;k<7;k++){ int yy=y-11+k; csR(x-5,yy,10,1,ol); csR(x-4,yy,8,1,bt); csR(x-4,yy,1,1,csSh(bt,2)); if(k==6) csR(x-4,yy,8,1,csLt(bt,5)); }   // a straight knee-length skirt
    if(csCz>=384){ csFaceBig(hx,hy+2,who,pose,t,open,sk,hr); return; }
    if(who==CA_MAME){ csR(hx-7,hy-1,3,15,ol); csR(hx+4,hy-1,3,15,ol); csR(hx-6,hy,2,14,hr); csR(hx+4,hy,2,14,hr); csR(hx-6,hy+9,1,3,csLt(hr,5)); csR(hx+5,hy+9,1,3,csLt(hr,5)); }      // long hair behind
    if(who==CA_MISSY){ csR(hx-6,hy-1,2,10,ol); csR(hx+5,hy-1,2,10,ol); csR(hx-6,hy,2,9,hr); csR(hx+4,hy,2,9,hr); }       // the bob, to the chin
    csD(hx,hy+1,6,ol); csD(hx,hy,5,hr); csD(hx,hy+2,4,sk);                                                                 // head: outline, hair, face
    if(who!=CA_MAME){ csR(hx-2,hy-4,2,1,csLt(hr,8)); } else { csR(hx-3,hy-2,7,2,hr); csR(hx-4,hy-1,2,4,hr); csR(hx+3,hy-1,2,3,hr); csR(hx+1,hy-4,2,1,csLt(hr,8)); }                       // the tousled tufts and a shine (the in-game hair)
    csR(hx-4,hy+4,1,1,RGB(28,12,12)); csR(hx+4,hy+4,1,1,RGB(28,12,12));                                                   // blush
    if(who==CA_MISSY){ u16 gl=RGB(9,9,12); int gx=hx, gy=hy+1; csR(gx-4,gy-1,3,1,gl); csR(gx-4,gy+1,3,1,gl); csR(gx-4,gy,1,1,gl); csR(gx-2,gy,1,1,gl); csR(gx+1,gy-1,3,1,gl); csR(gx+1,gy+1,3,1,gl); csR(gx+1,gy,1,1,gl); csR(gx+3,gy,1,1,gl); csR(gx-1,gy,2,1,gl); csR(gx-3,gy,1,1,bl?sk:dk); csR(gx+2,gy,1,1,bl?sk:dk); csR(gx-3,gy-3,7,2,hr); }   // small round glasses with a bridge, flat sleepy eyes, bangs
    else { csR(hx-2,hy+1,1,1,bl?sk:dk); csR(hx+1,hy+1,1,1,bl?sk:dk); csR(hx-3,hy-1,2,1,hr); csR(hx+1,hy-1,2,1,hr); }                    // eyes, brows
    if(who==CA_HOST) csR(hx-4,hy-3,8,2,hr);                                                                               // Dex: swept fringe
    if(who==CA_CREW){ csR(hx-6,hy-3,12,3,RGB(31,31,28)); csR(hx-7,hy-1,14,1,RGB(24,24,22)); csR(hx-2,hy-4,4,1,RGB(31,31,31)); }   // Hal: the hard hat
    if(who==CA_DOC){ csLn(hx-3,hy+6,hx,hy+10,RGB(22,22,24)); csLn(hx+3,hy+6,hx,hy+10,RGB(22,22,24)); csD(hx,hy+11,1,RGB(26,26,28)); }   // Okafor: the stethoscope
    if(who==CA_MISSY&&!open){ csR(hx-1,hy+4,3,1,RGB(18,6,6)); csR(hx+2,hy+3,1,1,RGB(18,6,6)); }                            // a deadpan smirk
    else if(open) csR(hx-1,hy+4,2,2,RGB(18,2,3)); else { csR(hx-1,hy+4,2,1,who==CA_MAME?RGB(22,9,9):RGB(18,6,6)); if(who==CA_MAME){ csR(hx-2,hy+3,1,1,RGB(22,9,9)); csR(hx+1,hy+3,1,1,RGB(22,9,9)); } }
}
