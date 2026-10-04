// loading.h - the LOADING screen: a message, a progress bar and a percent, so a long wait never looks like a freeze.
//   ldShow("WHAT IS HAPPENING", done, total);   draws the whole screen once and presents it (call it between the steps of a slow job)
// Needs before it: rect, text, tw, box, present, numStr, GOLD, SW, SH. Costs one frame (vsync) per call.
static void ldShow(const char*msg,int done,int total){
    if(total<1) total=1; if(done<0) done=0; if(done>total) done=total;
    int pct=done*100/total, bw=148, fill=bw*done/total;
    rect(0,0,SW,SH,RGB(3,4,8));
    box(36,48,168,60);
    text(120-tw("LOADING",1)/2,54,"LOADING",GOLD,1);
    text(120-tw(msg,1)/2,68,msg,WHITE,1);
    rect(46,82,bw+2,10,RGB(8,10,14)); rect(47,83,fill,8,GOLD);
    { char t[8]; char*e=t; e+=numStr(e,pct); *e++='%'; *e=0; text(120-tw(t,1)/2,95,t,DIMC,1); }
    text(120-tw("PLEASE WAIT",1)/2,101,"PLEASE WAIT",DIMC,1);
    present();
}
