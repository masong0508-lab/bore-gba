// 3DS cinematic acting: fixed-point, deterministic keyed performances.
// No texture atlases, floating point, heap, temporary render targets or
// searches in the character renderer. Only two actors per film frame.
#ifndef GG_FILM_ACTING3D_H
#define GG_FILM_ACTING3D_H
enum { GG_FILM_LISTEN,GG_FILM_SPEAK,GG_FILM_POINT,GG_FILM_PRESENT,
       GG_FILM_REFLECT,GG_FILM_REASSURE,GG_FILM_WALK };
typedef struct {
    signed char pose,head,lean,armL,armR,legL,legR,bob;
    signed char gaze,blink,mouth,holdProp;
} GgFilmPose;
// One 6x3x3 palette, shared by all guest film cels. These identities
// are stable across chapters, alternate endings and nonlinear films.
static const u8 ggFilmGuestPalette[6][3][3]={
    {{126,65,46},{212,168,133},{68,132,111}},
    {{72,64,58},{200,159,129},{68,99,135}},
    {{103,78,64},{207,164,137},{143,58,66}},
    {{83,57,53},{201,154,120},{105,118,78}},
    {{211,191,146},{226,188,155},{110,91,143}},
    {{90,74,58},{204,167,136},{112,99,78}}
};
static int ggFilmGestureFor(const char*line,int beat){
    // Scripted semantic acting for every existing dialogue reel without
    // requiring 100s of copy-pasted cel frames in ROM.
    if(strstr(line,"records")||strstr(line,"names")||
       strstr(line,"pages")||strstr(line,"books")||
       strstr(line,"supplies")||strstr(line,"provisions")||
       strstr(line,"ledgers")||strstr(line,"archive"))
        return GG_FILM_PRESENT;
    if(strstr(line,"families")||strstr(line,"children")||
       strstr(line,"wounded")||strstr(line,"home")||
       strstr(line,"peace")||strstr(line,"living"))
        return GG_FILM_REASSURE;
    if(strstr(line,"army")||strstr(line,"orders")||
       strstr(line,"crown")||strstr(line,"throne")||
       strstr(line,"banners")||strstr(line,"king"))
        return GG_FILM_POINT;
    if(strstr(line,"remember")||strstr(line,"truth")||
       strstr(line,"burden")||strstr(line,"choice")||
       strstr(line,"fear"))
        return GG_FILM_REFLECT;
    return (beat%4==0)?GG_FILM_POINT:
           (beat%4==2)?GG_FILM_REASSURE:GG_FILM_SPEAK;
}
static GgFilmPose ggFilmPoseAt(const char*voice,const char*line,
                              int beat,int local,int hero,int walking,
                              int propKind){
    GgFilmPose p={0};
    int speaking=hero?!strcmp(voice,"YOU"):
        (strcmp(voice,"YOU")&&strcmp(voice,"NARRATOR"));
    int t=ggExpClamp(local,0,GG_SAGA_BEAT_TICKS-1);
    int strength=t<12?t*256/12:
                 t<70?256:
                 (GG_SAGA_BEAT_TICKS-t)*256/26;
    strength=ggExpClamp(strength,0,256);
    p.pose=(signed char)(walking?GG_FILM_WALK:
           speaking?ggFilmGestureFor(line,beat):GG_FILM_LISTEN);
    p.gaze=(signed char)(hero?1:-1);
    // Acting has anticipation, held emphasis, reaction and recovery;
    // no per-frame RNG or universal continuous bobbing.
    if(p.pose==GG_FILM_WALK){
        int phase=(t/6)&3;
        p.legL=(signed char)((phase==0?-2:phase==2?2:0));
        p.legR=(signed char)-p.legL;
        p.armL=(signed char)(phase==0?1:phase==2?0:1);
        p.armR=(signed char)(phase==2?1:phase==0?0:1);
        p.bob=(signed char)((phase==1||phase==3)?1:0);
        p.lean=1;
    }else if(speaking){
        if(p.pose==GG_FILM_POINT){
            p.armR=(signed char)(strength*4/256);
            p.head=(signed char)(strength*2/256);
            p.lean=(signed char)(strength/160);
        }else if(p.pose==GG_FILM_PRESENT){
            p.armR=(signed char)(strength*3/256);
            p.armL=(signed char)(strength*2/256);
            // Records or crates are briefly actually held by the
            // outstretched performer rather than floating over them.
            p.holdProp=(signed char)(strength>170&&
                    (propKind==2||propKind==3));
        }else if(p.pose==GG_FILM_REFLECT){
            p.head=(signed char)(-strength*2/256);
            p.lean=(signed char)(-strength/160);
            p.gaze=0;
        }else if(p.pose==GG_FILM_REASSURE){
            p.armL=(signed char)(strength*2/256);
            p.armR=(signed char)(strength*2/256);
            p.head=(signed char)(strength/170);
        }else{
            p.armR=(signed char)(strength*2/256);
            p.head=(signed char)(strength/180);
        }
        // Short syllabic mouth shapes, with real silent frames;
        // unlike the old mechanically-flapping two-frame character.
        if(t>7&&t<78){
            int syllable=(t/5+beat*3+(hero?1:3))%9;
            p.mouth=(signed char)(syllable==0||syllable>=7?0:
                                   syllable==2||syllable==5?2:1);
        }
    }else{
        // Speaker's partner responds to the line with a small look,
        // a held reaction and then a natural settle.
        p.head=(signed char)(t>23&&t<66?1:0);
        if((beat&3)==2&&t>35&&t<70)
            p.armL=1;
    }
    // Intentionally sparse blinking and breathing.
    p.blink=(signed char)(((t+beat*17+(hero?13:39))%79)<3);
    if(!walking&&((t/24+beat+(hero?0:1))&3)==0)
        p.bob=1;
    return p;
}
// Every pose deforms the SAME original 16x24 cel consistently: head,
// torso, individual sleeves/hands, legs, eyes and mouth are separate
// movement layers. Runs of identical pixels are drawn as one rectangle
// to keep the Old 3DS fill budget bounded.
static void ggFilmDrawActor(u8*out,int x,int feet,int scale,
                            const u8*hair,const u8*skin,const u8*coat,
                            const GgFilmPose *p){
    int s=ggExpClamp(scale,1,3);
    ggExpRect(out,x-7*s,feet-2,14*s,2,29,36,41);
    for(int py=0;py<24;py++){
        const char *row=ggOriginHumanCel[py];
        for(int px=0;px<16;){
            int kind=row[px],xoff=0,yoff=0;
            if(kind=='.'){px++;continue;}
            int head=py<=8;
            int left=py>=10&&py<=15&&px<=3;
            int right=py>=10&&py<=15&&px>=12;
            int leg=py>=17&&py<=23;
            if(head){
                xoff=p->head;
                if(py<=4)xoff+=p->gaze;
                yoff=p->head<0?1:0;
            }else if(left){
                xoff=p->lean-(p->armL>1?1:0);
                yoff=-p->armL;
            }else if(right){
                xoff=p->lean+(p->armR>1?1:0);
                yoff=-p->armR;
            }else if(leg){
                xoff=px<8?p->legL:p->legR;
                yoff=-p->bob;
            }else{
                xoff=p->lean;
                yoff=-(p->bob);
            }
            int blink=(p->blink&&(kind=='e'||kind=='i'));
            if(blink)kind='s';
            // Batch contiguous body pixels with the same palette
            // entry and movement transform; never allocate a cel.
            int end=px+1;
            while(end<16){
                int k=row[end],xo=0,yo=0;
                if(k=='.')break;
                int h=py<=8,l=py>=10&&py<=15&&end<=3;
                int r=py>=10&&py<=15&&end>=12;
                int legs=py>=17&&py<=23;
                if(h){xo=p->head+(py<=4?p->gaze:0);
                      yo=p->head<0?1:0;}
                else if(l){xo=p->lean-(p->armL>1?1:0);yo=-p->armL;}
                else if(r){xo=p->lean+(p->armR>1?1:0);yo=-p->armR;}
                else if(legs){xo=end<8?p->legL:p->legR;yo=-p->bob;}
                else {xo=p->lean;yo=-p->bob;}
                if(p->blink&&(k=='e'||k=='i'))k='s';
                if(k!=kind||xo!=xoff||yo!=yoff)break;
                end++;
            }
            int r=42,g=46,b=51;
            switch(kind){
            case 'h':r=hair[0];g=hair[1];b=hair[2];break;
            case 's':r=skin[0];g=skin[1];b=skin[2];break;
            case 'e':r=237;g=232;b=210;break;
            case 'i':r=44;g=47;b=52;break;
            case 'c':r=coat[0];g=coat[1];b=coat[2];break;
            case 'd':r=coat[0]*3/5;g=coat[1]*3/5;b=coat[2]*3/5;break;
            case 'a':r=196;g=166;b=95;break;
            case 'm':r=202;g=192;b=159;break;
            case 'p':r=57;g=68;b=80;break;
            default:break;
            }
            ggExpRect(out,x+(px-8+xoff)*s,
                      feet-(24-py-yoff)*s,(end-px)*s,s,r,g,b);
            px=end;
        }
    }
    // A deliberate lip sync shape is placed on the chin of this
    // character rather than drawing arbitrary text in its face.
    if(p->mouth){
        int mx=x+p->head*s, my=feet-17*s;
        ggExpRect(out,mx-s,my,p->mouth==2?3*s:2*s,
                  p->mouth==2?2*s:s,113,65,68);
    }
    if(p->holdProp){
        int handX=x+7*s+p->lean*s;
        int handY=feet-(15+p->armR)*s;
        ggExpRect(out,handX,handY,5*s,2*s,220,203,164);
        ggExpRect(out,handX+s,handY-s,3*s,s,175,130,80);
        ggExpRect(out,handX+s,handY+s,3*s,s,84,67,55);
    }
}
#endif
