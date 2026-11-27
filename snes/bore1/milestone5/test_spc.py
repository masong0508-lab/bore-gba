#!/usr/bin/env python3
"""Audition actual SPC700 executable with libgme's SPC/S-DSP emulation."""
import ctypes as C, numpy as np, sys, soundfile as sf,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parent
lib=C.CDLL('libgme.so.0'); P=C.c_void_p
lib.gme_open_file.argtypes=[C.c_char_p,C.POINTER(P),C.c_int];lib.gme_open_file.restype=C.c_char_p
lib.gme_start_track.argtypes=[P,C.c_int];lib.gme_start_track.restype=C.c_char_p
lib.gme_play.argtypes=[P,C.c_int,C.c_void_p];lib.gme_play.restype=C.c_char_p
lib.gme_delete.argtypes=[P];lib.gme_ignore_silence.argtypes=[P,C.c_int];lib.gme_enable_accuracy.argtypes=[P,C.c_int]
p=P();sr=44100
assert not lib.gme_open_file(str(ROOT/'native_title.spc').encode(),C.byref(p),sr);lib.gme_ignore_silence(p,1);assert not lib.gme_start_track(p,0)
# Include the loop transition at the end, proving the resident score continues.
dur=json.loads((ROOT/'audio_report.json').read_text())['duration_seconds'];n=round((dur+8)*sr);samples=np.empty((n,2),np.int16)
for a in range(0,n,sr):
    x=samples[a:a+sr];assert not lib.gme_play(p,x.size,x.ctypes.data)
lib.gme_delete(p)
# Report output and timing; a short machine-start silence is allowed.
rms=np.sqrt(np.mean(samples.astype(float)**2,axis=1));frames=np.sqrt(np.mean(samples[:n//sr*sr].astype(float).reshape(-1,sr*2)**2,axis=1));peak=int(np.abs(samples.astype(int)).max());active=np.flatnonzero(frames>40)
report={'sample_rate':sr,'length_seconds':n/sr,'peak':peak,'rms_first_30s':frames[:30].round(1).tolist(),'active_second_count':int(len(active)),'max_silent_gap_seconds':int(np.diff(active).max()-1) if len(active)>1 else None,'stereo_difference_rms':float(np.sqrt(np.mean((samples[:,0].astype(float)-samples[:,1])**2))),'spc_emulator':'libgme SPC700/S-DSP','spc_sha256':hashlib.sha256((ROOT/'native_title.spc').read_bytes()).hexdigest()}
print(json.dumps(report,indent=2));assert peak>1000 and len(active)>.8*dur and report['stereo_difference_rms']>20
sf.write(ROOT/'native_title_emulated.wav',samples,sr);(ROOT/'spc_test.json').write_text(json.dumps(report,indent=2))
