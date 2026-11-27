"""Duck & Cover vocal edit, using immutable converted PCM as input.
Recover six independently timed verse phrases, use actual quarter-beat cuts,
preserve pitch, and add restrained stereo call/response. 32 instruments max.
"""
import numpy as np
from scipy import signal, ndimage
from collections import Counter
BEAT=60/174
CUES={8:2.438,9:10.438,10:18.375,11:26.438,12:84.,15:90.5,16:98.25,17:106.5,18:114.5,19:122.5}
# Source lyric-event time and duration, then measured active boundaries in
# the already time-fitted 174 BPM, semitone-down compiled pad-6 sample.
VERSE=[(34.312,6.018,.174,6.522),(43.713,5.703,9.972,16.001),(50.407,7.443,17.103,24.958),(58.769,7.008,25.915,33.307),(66.358,6.887,34.003,41.249),(74.236,8.217,42.293,50.902)]

def fit(x,n,sr):
    """Short-window waveform-matched overlap-add; no sample-rate pitch change."""
    if abs(len(x)-n)<sr*.004:return np.pad(x[:n],(0,max(0,n-len(x))))
    win=int(sr*.035); hop=win//2; tol=int(sr*.009)
    if len(x)<win*2 or n<win*2:return x[:n] if len(x)>=n else np.pad(x,(0,n-len(x)))
    out=np.zeros(n+win); w=np.zeros_like(out); taper=np.hanning(win); taper[:hop]=np.maximum(taper[:hop],1e-5)
    for p in range(0,n,hop):
        ideal=round(p*(len(x)-win)/max(1,n-win)); ideal=max(0,min(len(x)-win,ideal)); a=max(0,ideal-tol);b=min(len(x)-win,ideal+tol)
        if p==0:j=0
        else:
            ref=out[p:p+hop]/np.maximum(w[p:p+hop],1e-6)
            candidates=x[a:b+hop]; corr=signal.correlate(candidates,ref,mode='valid',method='fft'); energy=np.convolve(candidates*candidates,np.ones(hop),'valid'); j=a+int(np.argmax(corr/np.sqrt(energy+1e-8)))
        out[p:p+win]+=x[j:j+win]*taper;w[p:p+win]+=taper
    return out[:n]/np.maximum(w[:n],1e-5)

def vocal_edit(grid,samples,anchors):
    result=[{e[0]:e for e in row if not (8<=e[1]<=19)} for row in grid]; stats=Counter(); assets={}; anchors=list(anchors)+[0]*9
    source={i:np.asarray(samples[i],dtype=float)/128 for i in range(8,20)}
    def rate(i):return anchors[i]/2**28*18157*2**(-1/12)
    def onset(x,sr):
        env=np.sqrt(ndimage.uniform_filter1d(x*x,max(1,round(sr*.012)))); ix=np.flatnonzero(env>max(.009,env.max()*.10));return int(ix[0]) if len(ix) else 0
    def process(x,sr,fx='lead'):
        x=signal.sosfilt(signal.butter(2,105,'highpass',fs=sr,output='sos'),x)
        if fx=='radio':x=signal.sosfilt(signal.butter(2,[650,min(3300,sr*.43)],'bandpass',fs=sr,output='sos'),x)
        else:
            lo=signal.sosfilt(signal.butter(1,2600,'lowpass',fs=sr,output='sos'),x);x=x+.28*(x-lo)
        x=np.tanh(x*1.30)/1.12
        a=min(len(x)//3,max(1,round(sr*.007)));b=min(len(x)//3,max(1,round(sr*(.024 if fx=='lead' else .013))))
        if a:x[:a]*=np.sin(np.linspace(0,np.pi/2,a))**2
        if b:x[-b:]*=np.cos(np.linspace(0,np.pi/2,b))**2
        return x
    def asset(dst,src,x,fx='lead',pad=0):
        anchors[dst]=anchors[src];y=process(x,rate(src),fx);y=np.r_[np.zeros(pad),y];assets[dst]=np.r_[np.clip(np.rint(y*127),-127,127).astype(np.int8),np.int8(0)]
    def insert(row,ch,ins,n,vol,bus=3):
        if 0<=row<len(result) and ch not in result[row]:result[row][ch]=(ch,ins,bus,n,int(vol));return True
        return False
    def place(t,ch,ins,n,vol):
        r=round(t*8)
        assert insert(r,ch,ins,n,vol),(r,ch,ins);stats['primary_cues']+=1;return r
    for r,row in enumerate(grid):
        for ch,i,bus,n,v in row:
            if not 8<=i<=19:continue
            assert n==47,(i,n)
            if i in (13,14):continue
            t=172. if i==12 and r>1000 else CUES[i];sr=rate(i);x=source[i];a=max(0,onset(x,sr)-round(sr*.008));x=x[a:]
            if i not in assets:asset(i,i,x)
            place(t,ch,i,n,min(105,round(v*1.13)))
    for k,(t,d,a,b) in enumerate(VERSE):
        src=13;dst=13 if k==0 else 22+k;sr=rate(src);x=source[src]
        x=x[max(0,round((a*BEAT-.012)*sr)):min(len(x),round((b*BEAT+.025)*sr))]
        target=round(d*BEAT*sr);target=max(round(len(x)*.83),min(round(len(x)*1.12),target));x=fit(x,target,sr)
        asset(dst,src,x);place(t,8,dst,47,62);stats['verse_slices']+=1
    src=14;sr=rate(src);x=source[src];a=max(0,onset(x,sr)-round(.007*sr));x=x[a:]
    asset(14,14,x[:round(1.75*BEAT*sr)])
    short=x[:round(.266*BEAT*sr)];asset(28,14,short);asset(29,14,short,'radio');asset(30,14,short[::-1],'radio')
    # Reserved channels 11/12 must remain free of non-vocal source events.
    assert not any(e[0] in (11,12) for row in grid for e in row)
    for k,(r,ch,v) in enumerate((r,e[0],e[4]) for r,row in enumerate(grid) for e in row if e[1]==14):
        if r<1796:place(r/8,ch,14,47,min(100,round(v*1.12)));continue
        v=min(100,round(v*(1.15 if k%4==0 else 1.02)));dst=29 if k%8==7 else 28
        place(r/8,ch,dst,47,v);stats['short_stabs']+=1
        if k%4==1 and insert(r+3,11,28,47,max(12,round(v*.36)),1):stats['delay_throws']+=1
        if k%4==1 and insert(r+6,11,28,47,max(8,round(v*.19)),5):stats['delay_throws']+=1
        if k%8==6 and insert(r+2,12,29,47,max(14,round(v*.58)),4):stats['stutters']+=1
        if k%8==7 and insert(r-3,12,30,47,round(v*.38),2):stats['reverse_pickups']+=1
    out=[sorted(x.values()) for x in result]
    assert all(len(x)<=16 for x in out)
    assert [(r,e) for r,row in enumerate(grid) for e in row if e[1] in (6,7)]==[(r,e) for r,row in enumerate(out) for e in row if e[1] in (6,7)]
    assert stats=={'primary_cues':52,'verse_slices':6,'short_stabs':33,'delay_throws':16,'stutters':4,'reverse_pickups':4},stats
    return out,assets,anchors[:31],dict(stats)
