"""Harmonic NMF on a semitone-resolution constant-Q spectrum: V[m,t] ~ sum_n H[n,t] * sum_h a[h] * [m == n + OFF[h]]."""
import sys, os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from corp_lib import *
import numpy as np, librosa
NOTE=["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
def nn(m): return "%s%d"%(NOTE[int(m)%12],int(m)//12-1)
OFF=[0,12,19,24,28,31,34,36,38,40]          # semitone offsets of harmonics 1..10
def semispec(x,t0,t1,lo,hi,tune=0.0,sr=22050,hop=256,fs=0.8):
    """semitone magnitude spectrum, notes lo..hi+40, frames every hop/sr s starting at t0"""
    a=int(t0*SR); e=int(t1*SR); seg=x[max(0,a):e].astype(np.float32)
    y=librosa.resample(seg,orig_sr=SR,target_sr=sr)
    nb=(hi+40-lo+1)*3; fmin=librosa.midi_to_hz(lo+tune)*2**(-1/36)
    fmax=fmin*2**(nb/36)
    if fmax>sr/2*0.98: nb=int(np.floor(36*np.log2(sr/2*0.98/fmin)))
    C=np.abs(librosa.cqt(y,sr=sr,hop_length=hop,fmin=fmin,n_bins=nb,bins_per_octave=36,filter_scale=fs))
    n_semi=nb//3; C=C[:n_semi*3].reshape(n_semi,3,-1)
    V=np.sqrt((C**2).sum(1))      # energy of the 3 bins of each semitone
    return V, hop/sr
def hnmf(V,nnotes,iters=60,a0=None,learn=True,sparse=0.0):
    """V: (M,T). notes n=0..nnotes-1 at semitone rows n; returns H (nnotes,T), a (10,)"""
    M,T=V.shape; a=np.array(a0 if a0 is not None else [1,0.6,0.4,0.3,0.2,0.15,0.1,0.08,0.06,0.05],dtype=np.float64)
    H=np.maximum(V[:nnotes],1e-6).copy(); eps=1e-9
    def recon(H,a):
        R=np.zeros((M,T))
        for h,o in enumerate(OFF):
            k=min(nnotes,M-o)
            if k>0: R[o:o+k]+=a[h]*H[:k]
        return R
    for it in range(iters):
        R=recon(H,a)+eps; Q=V/R
        num=np.zeros_like(H); den=np.zeros(nnotes)
        for h,o in enumerate(OFF):
            k=min(nnotes,M-o)
            if k>0: num[:k]+=a[h]*Q[o:o+k]; den[:k]+=a[h]
        H*=num/(den[:,None]+sparse+eps)
        if learn and it>=10:
            R=recon(H,a)+eps; Q=V/R
            for h,o in enumerate(OFF):
                if h==0: continue
                k=min(nnotes,M-o)
                if k>0: a[h]*=(Q[o:o+k]*H[:k]).sum()/(H[:k].sum()+eps)
            a=np.minimum(a,3.0)
    return H,a
