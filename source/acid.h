// ACID RAINBOW: the main menu's live backdrop. A plasma made of four sine waves (columns, rows, diagonal and rings from the
// middle) plus a sideways wobble per row, coloured through a full rainbow wheel that keeps rotating, with dark contour lines
// between the bands. Zero ROM data: the sine table and the colour wheel are built at start-up (512 bytes of EWRAM), the code is
// about half a kilobyte. It is computed in 4x2 pixel blocks and written as 32-bit stores, and only rows 0..149 are drawn
// (the tip line at the bottom of the menu covers the rest). Smooth motion only, nothing strobes.
static s8  acSin[256] EWRAM_BSS;   // sine, -127..127, 256 steps per turn
static u16 acHue[128] EWRAM_BSS;   // fully saturated rainbow wheel, 128 steps
static int acT;                    // animation clock (the menu adds 2 per frame)

static void acidInit(void){
    for(int i=0;i<256;i++){ int x=i&127, v=(x*(128-x))>>5; if(v>127) v=127; acSin[i]=(s8)(i<128?v:-v); }   // parabola: close enough to a sine for a plasma
    for(int i=0;i<128;i++){
        int p=i*12, seg=p>>8, f=(p&255)>>3, up=f, dn=31-f, r, g, b;   // 6 segments of the colour wheel, 32 levels each
        switch(seg){ case 0: r=31; g=up; b=0; break; case 1: r=dn; g=31; b=0; break; case 2: r=0; g=31; b=up; break;
                     case 3: r=0; g=dn; b=31; break; case 4: r=up; g=0; b=31; break; default: r=31; g=0; b=dn; }
        acHue[i]=RGB(r,g,b);
    }
}

IWRAM_CODE static void acidRect(int t,int xb0,int xb1,int yb0,int yb1){   // blocks xb0..xb1-1 (4 pixels wide) of rows yb0..yb1-1 (2 pixels tall)
    for(int yb=yb0;yb<yb1;yb++){
        u32 *r0=(u32*)fb+(yb*2)*ROW_W, *r1=r0+ROW_W;
        int dy=yb-37, wob=acSin[(yb*9+t*3)&255]>>4, sy=acSin[(yb*7-t*2)&255];   // wob: the row sways -8..7 blocks
        for(int xb=xb0;xb<xb1;xb++){
            int x=xb+wob, dx=x-30;
            int v=acSin[(x*5+t)&255]+sy+acSin[((x+yb)*3+t*3)&255]+acSin[(((dx*dx+dy*dy)>>1)-t*5)&255];
            u16 c=acHue[((v>>2)+(t>>1))&127];
            if(((v>>3)&15)==0) c=(c>>1)&0x3DEF;   // half brightness: a dark contour line between bands
            u32 w=c|((u32)c<<16);
            r0[xb*2]=w; r0[xb*2+1]=w; r1[xb*2]=w; r1[xb*2+1]=w;
        }
    }
}
static void acidBg(int t){ acidRect(t,0,60,0,75); }   // the whole backdrop, rows 0..149
