#ifndef GG_FIELD_UI3D_H
#define GG_FIELD_UI3D_H

enum { GG_FIELD_HIDDEN, GG_FIELD_MAP_LOCKED, GG_FIELD_MAP_READY,
       GG_FIELD_PICK, GG_FIELD_WALK, GG_FIELD_REIGN, GG_FIELD_GUIDE,
       GG_FIELD_MODAL };
static const char *const ggFieldButtons[][3]={
    {NULL,NULL,NULL},
    {"WORLD WALK","AS A RESIDENT","REIGN AFTER 3"},
    {"WORLD WALK","AS A RESIDENT","ENTER REIGN"},
    {NULL,"CANCEL",NULL},
    {"BACK TO MAP","CONTROLS","INTERACT"},
    {"BACK TO MAP","CONTROLS","COUNCIL"},
    {NULL,"CLOSE",NULL},
    {NULL,"CLOSE",NULL}
};
enum { GG_FIELD_LEFT=8, GG_FIELD_WIDTH=96, GG_FIELD_STEP=104,
       GG_FIELD_TOP=2, GG_FIELD_HEIGHT=20 };
static int ggFieldButtonAt(int mode,int x,int y){
    if(x<GG_FIELD_LEFT||y<GG_FIELD_TOP||y>=GG_FIELD_TOP+GG_FIELD_HEIGHT)return -1;
    int local=x-GG_FIELD_LEFT,button=local/GG_FIELD_STEP;
    return button<3&&local%GG_FIELD_STEP<GG_FIELD_WIDTH&&ggFieldButtons[mode][button]?button:-1;
}
static void ggFieldStripText(u8 *out,int x,const char *label,int muted){
    for(;*label;label++,x+=6)for(int row=0;row<7;row++){
        int bits=font5x7[(unsigned char)*label-32][row];
        for(int col=0;col<5;col++)if(bits&(16>>col)){
            unsigned char *p=out+((8+row)*320+x+col)*3;
            p[0]=(u8)(muted?156:238);p[1]=(u8)(muted?168:221);p[2]=(u8)(muted?174:172);
        }
    }
}
static void ggFieldStripDraw(u8 *out,int mode,int reignChapter){
    memset(out,0,320*GG_3DS_MAP_TOP*3);
    for(int i=0;i<3;i++){
        const char *label=ggFieldButtons[mode][i];
        char lock[24];
        if(mode==GG_FIELD_MAP_LOCKED&&i==2){
            snprintf(lock,sizeof lock,"REIGN AFTER %d",reignChapter);
            label=lock;
        }
        if(!label)continue;
        int x=GG_FIELD_LEFT+i*GG_FIELD_STEP;
        int muted=mode==GG_FIELD_MAP_LOCKED&&i==2;
        for(int y=GG_FIELD_TOP;y<GG_FIELD_TOP+GG_FIELD_HEIGHT;y++){
            unsigned char *p=out+(y*320+x)*3;
            int line=y==GG_FIELD_TOP+GG_FIELD_HEIGHT-1;
            for(int u=0;u<GG_FIELD_WIDTH;u++,p+=3){
                p[0]=(u8)(line?muted?48:133:19);
                p[1]=(u8)(line?muted?58:110:32);
                p[2]=(u8)(line?muted?68:71:44);
            }
        }
        int tx=x+(GG_FIELD_WIDTH-(int)strlen(label)*6+1)/2;
        ggFieldStripText(out,tx,label,muted);
    }
    if(mode==GG_FIELD_PICK)ggFieldStripText(out,16,"TAP A PERSON",0);
}
static void ggFieldGuideDraw(u8 *out,int combat,int following,int sculpt){
    ggExpRect(out,24,34,352,157,16,28,38);
    ggExpRect(out,24,34,352,2,200,158,90);
    ggExpText(out,40,47,combat?"REIGN CONTROLS":"3D WORLD CONTROLS",247,224,174);
    ggExpText(out,40,68,"PAD MOVE    L / R TURN    B RUN",225,234,232);
    if(combat){
        ggExpText(out,40,88,"A TAP ATTACK    HOLD A TO CHARGE",225,234,232);
        ggExpText(out,40,104,"ZR SWITCHES SWORD, BOW, AND EMBER",225,234,232);
        ggExpText(out,40,120,"ZL GUARDS, STEADIES AIM, OR WARDS",225,234,232);
        ggExpText(out,40,136,"X OBJECTIVE    Y RATION    SELECT COUNCIL",225,234,232);
        ggExpText(out,40,152,"START RETURNS TO THE WORLD MAP",225,234,232);
    }else{
        ggExpText(out,40,88,sculpt?"L + A INTERACT AT A NEARBY SITE":"A INTERACT AT A NEARBY SITE",225,234,232);
        ggExpText(out,40,104,sculpt?"A PLACE    Y CLEAR    X MATERIAL":
                  "TAP THE MAP TO MARK A WAYPOINT",225,234,232);
        ggExpText(out,40,120,"SELECT SAVE    START RETURN TO MAP",225,234,232);
        ggExpText(out,40,136,following?"THIS RESIDENT'S HEALTH AND FOOD APPLY":
                  "HALLS AND MARKETS OFFER LOCAL WORK",225,234,232);
    }
    ggExpText(out,40,179,"A OR B CLOSE",174,211,221);
}

#endif
