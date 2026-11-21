// GodGuy 3DS cinema lettering: proportional 5x7 base glyphs with
// native-pixel edge smoothing, not the six-pixel monospaced computer HUD.
// Reuses the existing ROM font5x7; no new font files or textures.
// Fully clipped to the 400x240 top screen, no heap or framebuffer.
#ifndef GG_FILM_TYPE3D_H
#define GG_FILM_TYPE3D_H
static int ggFilmTypeGlyph(int ch){
    if(ch>='a'&&ch<='z')ch-=32;
    return ch>=32&&ch<=95?ch-32:0;
}
static int ggFilmTypeWidth(int glyph){
    int far=0;
    for(int y=0;y<7;y++){
        unsigned bits=font5x7[glyph][y];
        for(int x=0;x<5;x++)
            if(bits&(1u<<(4-x)))if(x+1>far)far=x+1;
    }
    return far?far:3;
}
static int ggFilmTypeMeasure(const char*str){
    int width=0;
    for(;*str;str++)
        width+=ggFilmTypeWidth(ggFilmTypeGlyph((unsigned char)*str))+2;
    return width;
}
static void ggFilmTypeBlend(u8*out,int x,int y,
                             int r,int g,int b,int strength){
    if((unsigned)x>=400||(unsigned)y>=240)return;
    u8 *p=out+(y*400+x)*3;
    p[0]=(u8)((p[0]*(4-strength)+r*strength)/4);
    p[1]=(u8)((p[1]*(4-strength)+g*strength)/4);
    p[2]=(u8)((p[2]*(4-strength)+b*strength)/4);
}
static void ggFilmText(u8*out,int x,int y,const char*str,
                        int r,int g,int b){
    int full=ggFilmTypeMeasure(str);
    if(!full)return;
    int available=400-x-8;if(available<1)return;
    int cursor=0;
    for(;*str;str++){
        int glyph=ggFilmTypeGlyph((unsigned char)*str);
        int width=ggFilmTypeWidth(glyph);
        for(int py=0;py<7;py++){
            unsigned bits=font5x7[glyph][py];
            for(int px=0;px<width;px++)if(bits&(1u<<(4-px))){
                int screenX=x+(full>available?
                    (cursor+px)*available/full:cursor+px);
                // Saturated stroke and restrained, single-pixel halos.
                // Unlike the old monospace renderer, diagonals appear
                // less stair-stepped on a 400px native-resolution screen.
                ggFilmTypeBlend(out,screenX,y+py,r,g,b,4);
                if(px==0||!(bits&(1u<<(5-px))))
                    ggFilmTypeBlend(out,screenX-1,y+py,r,g,b,1);
                if(px==width-1||!(bits&(1u<<(3-px))))
                    ggFilmTypeBlend(out,screenX+1,y+py,r,g,b,1);
            }
        }
        cursor+=width+2;
    }
}
#endif
