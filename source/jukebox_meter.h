#ifndef BORE_JUKEBOX_METER_H
#define BORE_JUKEBOX_METER_H
/* Shared eight-band analyzer and 14-bar renderer. The mixer samples are read-only.
   Fixed calibration avoids treble clipping when the bass is quiet. */
static inline int vzAbs(int x){return x<0?-x:x;}
static inline __attribute__((always_inline)) void meter_analyze_channel(const signed char *L,const signed char *R,int *lp,volatile unsigned short *out,int side){
    unsigned e[8]={0,0,0,0,0,0,0,0};
    static const unsigned char alpha[5]={192,112,64,32,16};
    for(int i=0;i<304;i+=4){
        int a=side?L[i]-R[i]:L[i]+R[i],b=side?L[i+1]-R[i+1]:L[i+1]+R[i+1];
        int c=side?L[i+2]-R[i+2]:L[i+2]+R[i+2],d=side?L[i+3]-R[i+3]:L[i+3]+R[i+3];
        e[0]+=vzAbs(a-b)+vzAbs(c-d);int ab=a+b,cd=c+d;
        e[1]+=vzAbs(ab-cd);int x=ab+cd,prev=x;
        for(int k=0;k<5;k++){
            int v=lp[k]+(((x-lp[k])*alpha[k])>>8);lp[k]=v;
            e[k+2]+=vzAbs(prev-v);prev=v;
        }
        e[7]+=vzAbs(prev);
    }
    for(int k=0;k<8;k++)out[k]=(unsigned short)(e[k]>>6);
}
/* Use the side channel when the mid channel nearly cancels. The sparse
   energy estimate adds 24 pairs per buffer, not a second filter bank. */
static inline __attribute__((always_inline)) void meter_analyze(const signed char *L,const signed char *R,int *lp,volatile unsigned short *out){
    unsigned mid=0,side=0;
    for(int i=1;i<304;i+=13){mid+=vzAbs((int)L[i]+R[i]);side+=vzAbs((int)L[i]-R[i]);}
    if(side>2*mid)meter_analyze_channel(L,R,lp,out,1);
    else meter_analyze_channel(L,R,lp,out,0);
}
static const unsigned short meter_gain[8]={256,331,799,660,778,700,734,312};
static const unsigned short meter_thresholds[48]={
  2,2,3,3,3,4,4,5,5,6,7,7,8,9,10,11,12,14,15,17,19,21,23,26,
  29,32,36,40,44,49,54,60,67,74,82,91,101,112,124,138,153,170,189,209,232,258,286,318
};
static inline unsigned meter_level(unsigned raw,unsigned k){
    unsigned v=(raw*meter_gain[k]+128)>>8,lo=0,hi=48;
    while(lo<hi){unsigned m=(lo+hi)>>1;if(v>=meter_thresholds[m])lo=m+1;else hi=m;}
    return lo;
}
#define VC(r,g,b) ((unsigned short)((r)|((g)<<5)|((b)<<10)))
static void meter_paint(unsigned short *fb,volatile unsigned short *out,volatile unsigned char *liveflag,
                        unsigned char *eq,unsigned char *pk,unsigned char *hold,unsigned char *beat,int *prevbass,int live){
    unsigned h[8];for(unsigned k=0;k<8;k++)h[k]=live?meter_level(out[k],k):0;
    if(live)*liveflag=30;
    int bass=h[6]+h[7];if(live&&bass>=*prevbass+10)*beat=6;*prevbass=bass;
    if(!live)*beat=0;
    const unsigned short bg=VC(5,7,12),dark=VC(3,5,8);
    unsigned bgbg=bg|((unsigned)bg<<16);
    static const unsigned short pal[12]={
        VC(3,17,21),VC(3,21,25),VC(4,25,27),VC(5,28,26),VC(5,29,20),VC(7,30,14),
        VC(15,30,8),VC(23,29,7),VC(29,25,5),VC(31,20,5),VC(31,13,5),VC(31,7,5)
    };
    unsigned *row=(unsigned*)fb;
    for(unsigned i=0;i<14;i++){
        unsigned k=7-(i>>1),t=h[k];
        if(i&1)t=i==13?h[0]:(t+h[k-1]+1)>>1;
        int e=eq[i];if(e>48)e=48;
        if(!live)e=0;
        else if((int)t>e){int d=(int)t-e;e+=(d+1)>>1;}
        else if((int)t<e)e-=(e-(int)t+7)>>3;
        if(e<0)e=0;if(e>48)e=48;eq[i]=(unsigned char)e;
        if(!live){pk[i]=hold[i]=0;}
        else if(e>=pk[i]){pk[i]=(unsigned char)e;hold[i]=15;}
        else if(hold[i])hold[i]--;
        else pk[i]=(unsigned char)(pk[i]>e+2?pk[i]-2:e);
        unsigned *p=row+28*120+4+i*2;
        for(unsigned y=0;y<4;y++,p+=120)p[0]=p[1]=bgbg;
        p=row+54*120+4+i*2;int cap=(pk[i]+3)>>2;
        for(int j=0;j<12;j++,p-=240){
            unsigned c=(e>=j*4+2)?pal[j]:dark,shade=(c>>1)&0x3def;
            if(live&&cap==j+1&&pk[i]>4)c=VC(23,29,31);
            p[0]=bg|(c<<16);p[1]=c|(c<<16);
            p[120]=bg|(shade<<16);p[121]=shade|(shade<<16);
        }
        unsigned base=*beat?VC(22,18,5):bg;
        unsigned *foot=row+56*120+4+i*2;foot[0]=foot[1]=base|(base<<16);
    }
    for(unsigned y=28;y<57;y++)row[y*120+32]=bgbg;
    if(*beat)(*beat)--;
}
#undef VC
#endif
