import sys, os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from corp_lib import *
import numpy as np, json
from scipy import signal
x=load('bass').astype(np.float64)
FS=8000; xl=signal.resample_poly(x,FS,SR)
def f0_seg(t0,t1,fmin=36.0,fmax=210.0):
    a=int(round(t0*FS)); b=int(round(t1*FS)); seg=xl[a:b]; n=len(seg)
    if n<int(0.03*FS): return None
    w=np.hanning(n); N=1<<16; X=np.abs(np.fft.rfft(seg*w,N))/w.sum()*2; df=FS/N
    cents=np.arange(1200*np.log2(fmin/55.0),1200*np.log2(fmax/55.0),5.0); f=55.0*2**(cents/1200.0)
    S=np.zeros(len(f)); H=10
    for h in range(1,H+1):
        idx=np.clip(np.round(f*h/df).astype(int),0,len(X)-1); ok=(f*h)<FS/2*0.95
        S+=np.where(ok,X[idx],0)/np.sqrt(h)
    best=S.max(); pk,_=signal.find_peaks(S,height=0.80*best)
    if len(pk)==0: pk=np.array([int(np.argmax(S))])
    j=pk[-1]          # the highest pitch that explains (almost) as much as the best one: avoids the octave-below error
    # parabolic refinement
    if 0<j<len(S)-1:
        y0,y1,y2=S[j-1],S[j],S[j+1]; dlt=0.5*(y0-y2)/(y0-2*y1+y2+1e-12); f0=f[j]*2**(dlt*5.0/1200.0)
    else: f0=f[j]
    return f0, float(np.sqrt((seg**2).mean()))
def onsets():
    e=env(bp(x,250,1500),4.0); hop=32; ee=e[::hop]; le=np.log(ee+3e-4)
    d=np.concatenate([[0,0,0],le[3:]-le[:-3]]); w=int(0.045*SR/hop); on=[]; i=0
    while i<len(d):
        if d[i]>0.5 and d[i]>=d[max(0,i-w):i+w].max() and ee[min(len(ee)-1,i+8)]>0.0025: on.append(i*hop); i+=w
        else: i+=1
    return np.array(on)
if __name__=="__main__":
    on=onsets(); print(len(on),"pluck onsets")
    full=env(x,6.0)
    notes=[]
    for i,o in enumerate(on):
        nxt=on[i+1] if i+1<len(on) else len(x)
        t0=o/SR; tn=nxt/SR
        # end: where the level stays below 12% of the note's peak
        seg=full[o:nxt]; pk=seg[:int(0.08*SR)].max() if len(seg) else 0
        below=np.where(seg<0.12*pk)[0]; end=o+(below[0] if len(below) else len(seg))
        r=f0_seg(t0+0.012,min(t0+0.13,tn-0.004))
        if r is None: continue
        f0,lev=r; m=69+12*np.log2(f0/440.0)
        notes.append(dict(t0=t0,t1=end/SR,f0=f0,midi=m,lev=lev,pk=float(pk)))
    json.dump(notes,open('bass_notes_raw.json','w'))
    dev=np.array([n['midi']-np.round(n['midi']) for n in notes if n['pk']>0.05]); 
    print("fractional pitch: circular mean %.1f cents"%(np.angle(np.mean(np.exp(2j*np.pi*dev)))/(2*np.pi)*100))
    B0=int(sys.argv[1]); B1=int(sys.argv[2])
    for n in notes:
        B,tk=pos_of(n['t0'])
        if B0<=B<B1:
            m=n['midi']+0.45
            print("bar %3d tick %5.1f (slot %4.2f)  %-4s %+3.0fc  dur %3.0f ms  lev %.3f"%(B,tk,tk/6,nn(m),(m-round(m))*100,(n['t1']-n['t0'])*1000,n['pk']))
