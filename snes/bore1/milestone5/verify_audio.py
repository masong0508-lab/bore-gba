#!/usr/bin/env python3
"""Run each resident SPC program in libgme, not a host audio synth."""
import ctypes as C, numpy as np, json, soundfile as sf, hashlib, sys
from pathlib import Path
R=Path(__file__).resolve().parent
lib=C.CDLL('libgme.so.0');P=C.c_void_p
lib.gme_open_file.argtypes=[C.c_char_p,C.POINTER(P),C.c_int];lib.gme_open_file.restype=C.c_char_p
lib.gme_start_track.argtypes=[P,C.c_int];lib.gme_start_track.restype=C.c_char_p
lib.gme_play.argtypes=[P,C.c_int,C.c_void_p];lib.gme_play.restype=C.c_char_p
lib.gme_delete.argtypes=[P];lib.gme_ignore_silence.argtypes=[P,C.c_int]
def run(path,seconds):
    p=P();sr=44100;assert not lib.gme_open_file(str(path).encode(),C.byref(p),sr);lib.gme_ignore_silence(p,1);assert not lib.gme_start_track(p,0)
    x=np.empty((round(seconds*sr),2),np.int16)
    for a in range(0,len(x),sr):
        y=x[a:a+sr];assert not lib.gme_play(p,y.size,y.ctypes.data)
    lib.gme_delete(p);return x
if __name__=='__main__':
    report={}
    for idx in (0,31):
        r=json.loads((R/f'track_{idx}.json').read_text());x=run(R/f'track_{idx}.spc',r['duration_seconds']+4);f=x[:len(x)//44100*44100].astype(float).reshape(-1,88200);rms=np.sqrt(np.mean(f*f,axis=1));active=np.flatnonzero(rms>40);peak=int(np.abs(x.astype(int)).max());assert len(active)>r['duration_seconds']*.8 and peak>1000
        r.update(peak=peak,active_seconds=int(len(active)),max_silent_gap_seconds=int(np.diff(active).max()-1),stereo_rms_difference=float(np.sqrt(np.mean((x[:,0].astype(float)-x[:,1])**2))),emulator='libgme SPC700/S-DSP',spc_sha256=hashlib.sha256((R/f'track_{idx}.spc').read_bytes()).hexdigest());assert r['stereo_rms_difference']>20
        sf.write(R/f'track_{idx}_emulated.wav',x,44100);report[str(idx)]=r;print(r,flush=True)
    (R/'native_verification.json').write_text(json.dumps(report,indent=2))
