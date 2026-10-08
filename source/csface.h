// csface.h - CUTSCENE CLOSE-UP FACES (module 4): when the camera is zoomed in (csCz >= 384) csFig hands the head to csFaceBig, which draws a detailed face at SCREEN resolution
// instead of stretching the 9 px sprite head: shaded face, ears, neck, eyes with lids / irises / glints, brows, nose, blush, a mouth for each mood, Missy's round glasses, each
// person's hair (Hal's hard hat, Okafor's stethoscope) and tears for the crying scenes. The mood comes from the pose and the beat's effects (csFxNow, set by csDraw).
// Step 5: the head TURNS (eyes, nose, mouth, brows slide further than the face oval, the face further than the hair and ears), blinks have three phases (half, shut, half) and the mouth
// shape follows the syllable (ah wide and tall, oo small and round, ee wide and flat, oh round).
// Units below are QUARTER world pixels, relative to the face centre. Needs cscam.h (step 2) and csart.h (step 1).
static int csFxNow, csFcx, csFcy;
static int csFrameNo;   // (also declared in csart.h; csface.h comes first)
static short csTn[8] EWRAM_BSS; static int csTnFn[8] EWRAM_BSS;   // step 5: each figure's head turn, in 1/4 quarter-pixels (eased every frame), and the frame it was last eased on (16 B of EWRAM)
static u16 csFSh(u16 c,int k){ int r=(c&31)-k,g=((c>>5)&31)-k,b=((c>>10)&31)-k; if(r<0)r=0; if(g<0)g=0; if(b<0)b=0; return RGB(r,g,b); }
static u16 csFLt(u16 c,int k){ int r=(c&31)+k,g=((c>>5)&31)+k,b=((c>>10)&31)+k; if(r>31)r=31; if(g>31)g=31; if(b>31)b=31; return RGB(r,g,b); }
static u16 csFMix(u16 a,u16 b,int k){ return RGB(((a&31)*(8-k)+(b&31)*k)>>3,(((a>>5)&31)*(8-k)+((b>>5)&31)*k)>>3,(((a>>10)&31)*(8-k)+((b>>10)&31)*k)>>3); }   // k/8 of b
static int csFu(int n){ return (n*csCz)/1024; }                       // quarter pixels -> screen pixels
static int csFr(int n){ int v=csFu(n); return v<1?1:v; }             // a size: at least one pixel
static int csFi(int v){ int w=0; while((w+1)*(w+1)<=v) w++; return w; }
static void csFEl(int cx,int cy,int rx,int ry,int top,u16 c){        // a filled ellipse (top: only the upper half)
    if(ry<1){ rect(cx-rx,cy,2*rx+1,1,c); return; }
    for(int y=-ry;y<=(top?0:ry);y++){ int w=rx*csFi(16*(ry*ry-y*y))/(4*ry); rect(cx-w,cy+y,2*w+1,1,c); }
}
static void csFE(int dx,int dy,int rx,int ry,u16 c){ csFEl(csFcx+csFu(dx),csFcy+csFu(dy),csFr(rx),csFr(ry),0,c); }
static void csFT(int dx,int dy,int rx,int ry,u16 c){ csFEl(csFcx+csFu(dx),csFcy+csFu(dy),csFr(rx),csFr(ry),1,c); }
static void csFR(int dx,int dy,int w,int h,u16 c){ rect(csFcx+csFu(dx),csFcy+csFu(dy),csFr(w),csFr(h),c); }
static void csFStr(int x,int y,int w,int h,u16 c){ int a=h/3, l=csHlag*4; csFR(x,y,w,a,c); csFR(x+l/2,y+a,w,a,c); csFR(x+l,y+2*a,w,h-2*a,c); }   // a long strand of hair: the tip trails (csHlag)
static void csFLn(int x0,int y0,int x1,int y1,u16 c){ line(csFcx+csFu(x0),csFcy+csFu(y0),csFcx+csFu(x1),csFcy+csFu(y1),c); }

static void csFaceBig(int wx,int wy,int who,int pose,int t,int open,u16 sk,u16 hr){
    csFcx=csCamX(wx+csOx); csFcy=csCamY(wy+csOy); int bx=csFcx;   // bx: the head's own centre; csFOff() slides the features off it for the turn
#define csFOff(o) (csFcx=bx+csFu(o))
    u16 dk=RGB(3,2,3), ol=RGB(2,1,4), lip=RGB(20,7,8), wh=RGB(30,30,31);
    int sick=(csFxNow&CF_SICK)!=0, shake=(csFxNow&CF_SHAKE)!=0;
    int lie=pose==CP_LIE||pose==CP_STIR, sad=pose==CP_HEAD||pose==CP_CRY||pose==CP_SLUMP, sing=pose==CP_SING||open, scream=pose==CP_FLAIL||pose==CP_SHOCK||sick, yell=shake&&!sad&&!lie&&(who!=CA_MISSY||pose!=CP_STAND||1);
    int crying=(sad&&shake&&!sick)||pose==CP_CRY, bph=((t+who*23)%100), bk=bph<6?((bph<2||bph>=4)?1:2):0, closed=lie||bk==2||crying, half=bk==1&&!closed, loud=sing||scream||(yell&&!sad);
    u16 fc=sick?csFMix(sk,RGB(12,24,6),4):sk, hat=RGB(31,31,28); int w4=csMd==4;   // csMd: the mood (cutscene redo 14 step 4)
    int ex=(who==CA_MISSY)?10:8;
    int tn; { int w=who&7, tg=lie?0:csLook*20+csWv(t+who*17,170)/2; if(sad) tg=csLook*8; if(csFxNow&CF_SHAKE) tg=tg/2+csWv(t,6)/2;   // where the head wants to point (1/4 quarter px): toward the other figure, glancing, a slow idle sway
        if(csFrameNo-csTnFn[w]>3) csTn[w]=(short)tg; else { int d=tg-csTn[w]; if(csTnFn[w]!=csFrameNo&&d) csTn[w]+=(short)(d/4?d/4:(d>0?1:-1)); }   // ease toward it (a jump after a cut or a pause)
        csTnFn[w]=csFrameNo; tn=csTn[w]/4; if(tn>7) tn=7; if(tn<-7) tn=-7; }
    int tf=tn/2, th=tn/3;   // the face oval slides half as far as the features, the hair a third
    // hair behind, neck, ears
    csFE(0,-7,23,23,ol); csFE(0,-8,21,21,hr);
    if(who==CA_MISSY){ csFStr(-25,-14,9,46,ol); csFStr(16,-14,9,46,ol); csFStr(-24,-12,7,43,hr); csFStr(17,-12,7,43,hr); }
    if(who==CA_MAME){ csFStr(-27,-14,10,62,ol); csFStr(17,-14,10,62,ol); csFStr(-26,-12,8,59,hr); csFStr(18,-12,8,59,hr); }
    csFR(-7,12,14,16,ol); csFR(-6,12,12,15,csFSh(fc,4));
    csFE(-18,2,4,5,ol); csFE(18,2,4,5,ol); csFE(-18,2,3,4,csFSh(fc,2)); csFE(18,2,3,4,csFSh(fc,2));
    // face
    csFOff(tf);
    csFE(0,0,18,20,ol); csFE(0,0,17,19,csFSh(fc,3)); csFE(-1,-1,16,18,fc); csFE(-5,-10,6,3,csFLt(fc,2));
    // hair in front
    csFOff(th);
    if(who==CA_MISSY){ csFE(0,-14,19,9,hr); csFR(-12,-6,6,4,hr); csFR(-3,-6,6,5,hr); csFR(6,-6,6,4,hr); csFR(-8,-19,6,2,csFLt(hr,8)); }
    else if(who==CA_MAME){ csFE(2,-14,19,9,hr); csFR(-19,-9,6,13,hr); csFR(14,-9,5,11,hr); csFR(-8,-8,14,3,hr); csFR(-6,-19,10,2,csFLt(hr,8)); }
    else if(who==CA_HOST){ csFE(2,-15,19,9,hr); csFR(-19,-9,5,8,hr); csFR(-6,-8,12,3,hr); csFR(-4,-21,8,2,csFLt(hr,8)); csFR(-2,-18,2,9,ol); }
    else if(who==CA_DOC){ csFE(0,-14,19,9,hr); csFR(-20,-9,5,12,hr); csFR(15,-9,5,12,hr); csFE(10,-30,6,5,hr); csFR(-6,-20,8,2,csFLt(hr,8)); }
    else if(who==CA_CREW){ csFR(-21,-8,5,10,hr); csFR(16,-8,5,10,hr); csFT(0,-4,23,18,ol); csFT(0,-5,22,17,hat); csFR(-2,-22,4,17,RGB(24,24,22)); csFR(-26,-6,52,4,RGB(20,20,19)); csFR(-26,-6,52,1,hat); }
    // glasses (Missy): frame, clear lens, bridge, arms
    csFOff(tn);
    if(who==CA_MISSY){ u16 gl=RGB(16,16,20);
        csFR(-26,-3,10,2,gl); csFR(16,-3,10,2,gl); csFR(-3,-2,6,2,gl);
        csFE(-ex,0,9,9,gl); csFE(ex,0,9,9,gl); csFE(-ex,0,7,7,csFLt(fc,1)); csFE(ex,0,7,7,csFLt(fc,1)); }
    // eyes
    for(int s=-1;s<=1;s+=2){ int cx=s*ex;
        if(closed){ csFR(cx-5,0,10,2,dk); csFR(cx-5+(s<0?0:8),1,2,2,dk); csFR(cx-4,2,8,1,csFSh(fc,5)); }
        else if(half){ u16 ir=who==CA_MISSY?RGB(18,15,6):RGB(11,7,3); csFE(cx,1,5,2,dk); csFE(cx,1,4,1,wh); csFE(cx+(sad?0:csLook),1,3,2,ir); csFR(cx-5,-3,10,3,csFSh(fc,2)); csFR(cx-5,-1,10,1,dk); csFR(cx-5,-4,10,1,csFSh(fc,5)); }   // step 5: the lid half down
        else { int gy=sad?3:0, gx=sad?0:csLook; u16 ir=who==CA_MISSY?RGB(18,15,6):RGB(11,7,3);
            csFE(cx,0,5+w4,4+w4,dk); csFE(cx,0,4+w4,3+w4,wh);
            csFE(cx+gx,gy,3,3,ir); csFE(cx+gx,gy,1,1,dk); csFR(cx+gx-2,gy-2,1,1,wh);
            if(who==CA_MISSY&&!loud&&!sad){ csFR(cx-5,-4,10,4,csFSh(fc,2)); csFR(cx-5,-1,10,1,dk); csFR(cx-5,-5,10,1,csFSh(fc,5)); }   // the deadpan half lid
            else { csFR(cx-5,-4,10,1,dk); if(loud) csFR(cx-5,-6,10,1,csFSh(fc,4)); }
            if(csMd==2) csFR(cx-5,1,10,3,csFSh(fc,1));                                                                       // a smile pushes the cheeks up into the eyes
            if(csMd==1||csMd==6) csFR(cx-5,-4,10,2,csFSh(fc,2));                                                            // a heavy upper lid
            if(sad) csFR(cx-4,3,8,1,csFLt(fc,0)); }
        if(who==CA_MISSY&&!closed&&!half){ csFR(s*ex-4,-6,2,2,wh); csFR(s*ex-3,-4,1,1,wh); }                                           // a glint on the lens
    }
    // brows: heights from the outer end to the inner end
    csFOff(tn);
    { static const signed char bn[3]={-10,-10,-10}, bs[3]={-9,-11,-13}, ba[3]={-13,-11,-9}, br[3]={-14,-14,-14}; const signed char*bw=(sad||csMd==1||csMd==6)?bs:(csMd==3||(yell&&!sad&&shake))?ba:(csMd==4||loud)?br:bn;
      for(int s=-1;s<=1;s+=2) for(int i=0;i<3;i++){ int x=s<0?-ex-8+i*5:ex+8-i*5-5; csFR(x,bw[i]-(who==CA_MISSY?3:0)-csEm*2-((csMd==5&&s>0)?4:0),5,2,who==CA_CREW?hr:csFLt(hr,2)); } }
    // nose, cheeks (the nose moves furthest: it is nearest the camera)
    csFOff(tn+tn/2);
    csFR(-2,5,4,3,csFSh(fc,4)); csFR(-1,3,2,2,csFLt(fc,2)); csFR(-3,7,2,1,csFSh(fc,6)); csFR(1,7,2,1,csFSh(fc,6));
    csFOff(tn);
    if(!sick){ csFE(-13,9,5,3,csFMix(fc,RGB(28,12,12),3)); csFE(13,9,5,3,csFMix(fc,RGB(28,12,12),3)); }
    // mouth: its width follows the syllable (step 5)
    if(loud||sick){ int h=scream?7:csMh, mw=7, sy=(t>>(sing&&!yell?3:2)); if(!scream){ switch((sy*7+who*3+sy/4)&3){ case 0: mw=7; break; case 1: mw=4; h=h+1; break; case 2: mw=9; h=h*2/3; if(h<2) h=2; break; default: mw=5; } }   // ah / oo / ee / oh
        int tw=mw-2; csFE(0,13,mw,h,RGB(10,1,3)); if(!scream) csFR(-tw,9,2*tw,3,wh); csFE(0,16,mw/2+1,2,RGB(26,8,10)); }
    else if(csMd==3&&!sad){ csFR(-6,14,12,2,csFSh(lip,5)); csFR(-7,15,2,2,lip); csFR(5,15,2,2,lip); }                          // angry: a tight line, down at the corners
    else if(csMd==4&&!sad){ csFE(0,14,4,5,RGB(10,1,3)); csFR(-3,11,6,1,csFSh(lip,3)); }                                      // shocked: a small O
    else if(csMd==5&&!sad){ csFR(-5,14,8,2,lip); csFR(3,12,3,2,lip); }                                                        // puzzled: crooked
    else if(csMd==2&&!sad){ csFR(-6,13,12,2,lip); csFR(-8,11,3,3,lip); csFR(5,11,3,3,lip); csFR(-4,15,8,2,csFLt(lip,6)); }   // a big smile
    else if(sad||csMd==1||csMd==6){ csFR(-5,13,10,2,lip); csFR(-6,14,2,2,lip); csFR(4,14,2,2,lip); if(crying) csFR(-3,15,6,1,csFSh(lip,5)); }
    else if(who==CA_MISSY){ csFR(-5,13,9,2,lip); csFR(4,11,3,2,lip); csFR(7,10,1,1,csFSh(fc,5)); csFR(-3,15,6,1,csFLt(lip,6)); }
    else { u16 lp=who==CA_MAME?csFMix(csFSh(fc,2),RGB(24,10,10),5):lip; csFR(-5,13,10,2,lp); csFR(-6,12,2,2,lp); csFR(4,12,2,2,lp); csFR(-3,15,6,1,csFLt(lp,4)); }
    // tears for the crying scenes, a sweat drop for the sick one
    if(crying||(lie&&shake)) for(int s=-1;s<=1;s+=2){ int y=6+((t*2+(s>0?7:0))%22); csFR(s*ex-1,y,2,3,RGB(14,22,31)); csFR(s*ex-1,6,2,y-6,RGB(10,17,26)); }
    if(sick) csFE(17,-12,2,3,RGB(14,24,31));
    csFOff(0);
    if(who==CA_DOC){ u16 g=RGB(22,22,24); csFLn(-12,18,-2,40,g); csFLn(12,18,2,40,g); csFLn(-11,18,-1,40,g); csFLn(11,18,1,40,g); csFE(0,42,3,3,RGB(26,26,28)); }
}
