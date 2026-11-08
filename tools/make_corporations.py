#!/usr/bin/env python3
"""CORPORATIONS (YOU'RE WINNER, YOU RULE): a vocal song for the GBA jukebox, built from six isolated stems (bass, drums, other, guitar, piano, vocals).

What it does (run from the project root, needs ffmpeg + numpy + scipy):
  1. reads the stems, measures them and finds the vocal phrases (voice activity) and the song's tempo from the drums, so every phrase has a timing in seconds,
     in beats and sixteenths, in GBA music samples (18157 Hz) and in 60 Hz frames  ->  tools/corporations_vocal_timing.json
  2. squeezes the singing: high-pass, a fast 8:1 compressor, a slow 3:1 levelling compressor, a presence lift and a soft clipper, so the voice stays at one loudness
     through the whole song (a 4-bit 18 kHz stream has no headroom for a quiet verse)
  3. sets it in the mix: the instruments are split in three bands and, while the voice sings, the mid band (where the voice lives) is pulled down by up to 5 dB
     and the top band by 1.5 dB, so the voice has its own space instead of fighting the guitar and the piano
  4. saves it: the normal game format (mono 4-bit IMA-ADPCM at 18157 Hz, source/music/corporations.adp + a SONG_ADP line in source/songs.h)
  5. as the theoretical split version (instrumental stream + a vocal track of phrase clips at their exact times), it also writes the singing alone as 2-bit ADPCM at
     8 kHz, only where somebody is singing: tools/corporations_vocal2.bin, and checks that the clips land on the original timing.
"""
import os, sys, json, subprocess, struct
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from encode_sfx import encode, decode
import encode_song

SR = 44100
STEMS = ["bass", "drums", "other", "guitar", "piano", "vocals"]
SRC = os.environ.get("STEM_DIR", "/root/.claude/uploads/2b9164b2-7e8c-59d8-8b67-0e06180c3de2")
GBA = 18157

def find_stem(name):
    for f in os.listdir(SRC):
        if f.endswith("_%s_mvsep.com.flac" % name): return os.path.join(SRC, f)
    raise SystemExit("stem not found: " + name)

def load(path, sr=SR):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-ac", "2", "-ar", str(sr), "-f", "f32le", "-"], capture_output=True, check=True).stdout
    x = np.frombuffer(raw, dtype="<f4").reshape(-1, 2)
    return x.mean(axis=1).astype(np.float64)

def db(x): return 20 * np.log10(max(1e-9, x))
def rms(x): return float(np.sqrt(np.mean(x * x) + 1e-18))

def ctl(x, hop):    # block peak envelope at the control rate
    n = len(x) // hop; return np.abs(x[:n * hop]).reshape(n, hop).max(axis=1)

def smooth(env, att, rel, rate):   # one-pole attack/release follower at the control rate
    a = np.exp(-1.0 / (att * rate)); r = np.exp(-1.0 / (rel * rate)); out = np.empty_like(env); y = 0.0
    for i, v in enumerate(env):
        y = a * y + (1 - a) * v if v > y else r * y + (1 - r) * v
        out[i] = y
    return out

def compress(x, thr_db, ratio, att, rel, hop=110):
    rate = SR / hop; env = smooth(ctl(x, hop), att, rel, rate)
    lev = 20 * np.log10(np.maximum(env, 1e-6)); gr = np.where(lev > thr_db, (thr_db - lev) * (1 - 1 / ratio), 0.0)
    g = 10 ** (gr / 20); gfull = np.interp(np.arange(len(x)), (np.arange(len(g)) + 0.5) * hop, g)
    return x * gfull

def biquad_peak(f0, gain_db, q, sr=SR):
    A = 10 ** (gain_db / 40); w = 2 * np.pi * f0 / sr; al = np.sin(w) / (2 * q); c = np.cos(w)
    b = [1 + al * A, -2 * c, 1 - al * A]; a = [1 + al / A, -2 * c, 1 - al / A]; return np.array(b) / a[0], np.array(a) / a[0]

def vad(v, hop=441):   # voice activity: returns per-hop boolean + the phrases (start s, end s): runs of singing, cut at breaths and at deep dips of long runs
    n = len(v) // hop; e = np.sqrt((v[:n * hop].reshape(n, hop) ** 2).mean(axis=1) + 1e-12); ed = 20 * np.log10(e)
    act = ed > np.percentile(ed, 99) - 36; runs = []; i = 0
    while i < n:                                        # runs, bridging pauses shorter than 0.12 s
        if act[i]:
            j = i
            while j < n and (act[j] or act[j + 1:j + 12].any()): j += 1
            if j - i >= 25: runs.append((i, j))
            i = j
        else: i += 1
    ph = []
    for a, b in runs:                                    # a run longer than 6 s is cut at its deepest dips (the singer's breaths and word gaps)
        cuts = [a]
        while b - cuts[-1] > 600:
            lo = cuts[-1] + 200; hi = min(b - 100, cuts[-1] + 600); k = lo + int(np.argmin(np.convolve(ed[lo:hi], np.ones(5) / 5, "same"))); cuts.append(k)
        cuts.append(b)
        for x, y in zip(cuts[:-1], cuts[1:]): ph.append((x * hop / SR, y * hop / SR))
    return act, ph

def tempo(d):   # BPM and the time of the first strong beat from the drum stem
    hop = 441; f, t, S = signal.stft(d, SR, nperseg=1024, noverlap=1024 - hop, boundary=None); mag = np.abs(S)
    flux = np.maximum(0, np.diff(mag, axis=1)).sum(axis=0); flux = flux / (flux.max() + 1e-9); flux -= np.convolve(flux, np.ones(50) / 50, "same")
    best = (0, 0, 0)
    for bpm in np.arange(70, 200, 0.25):
        per = SR / hop * 60 / bpm; idx = np.arange(0, len(flux) - 1, per)
        for ph in np.linspace(0, per, 16, endpoint=False):
            ii = np.minimum((idx + ph).astype(int), len(flux) - 1); sc = flux[ii].sum() / len(ii)
            if sc > best[0]: best = (sc, bpm, ph * hop / SR)
    return best[1], best[2]

# ---- 2-bit adaptive delta (IMA step table, one sign bit + one magnitude bit): the singing alone, 2 bits per sample
STEP = np.array([7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767])
def enc2(x):
    out = []; pred = 0; idx = 20
    for s in x:
        st = int(STEP[idx]); d = int(s) - pred; sg = 1 if d < 0 else 0; ad = abs(d); m = 1 if ad > st // 2 else 0
        dq = (st if m else st // 4) ; pred += -dq if sg else dq; pred = max(-32768, min(32767, pred)); idx = max(0, min(88, idx + (3 if m else -1)))
        out.append(sg * 2 + m)
    b = bytearray((len(out) + 3) // 4)
    for i, c in enumerate(out): b[i >> 2] |= c << ((i & 3) * 2)
    return bytes(b), len(out)
def dec2(b, n):
    out = np.empty(n, dtype=np.int32); pred = 0; idx = 20
    for i in range(n):
        c = (b[i >> 2] >> ((i & 3) * 2)) & 3; sg = c >> 1; m = c & 1; st = int(STEP[idx]); dq = st if m else st // 4
        pred += -dq if sg else dq; pred = max(-32768, min(32767, pred)); idx = max(0, min(88, idx + (3 if m else -1))); out[i] = pred
    return out

def main():
    st = {n: load(find_stem(n)) for n in STEMS}; L = min(len(v) for v in st.values()); st = {k: v[:L] for k, v in st.items()}
    print("stems: %.1f s" % (L / SR)); [print("  %-7s rms %6.1f dBFS peak %6.1f" % (k, db(rms(v)), db(np.abs(v).max()))) for k, v in st.items()]
    voc = st["vocals"]; act, ph = vad(voc); bpm, t0 = tempo(st["drums"]); print("vocal phrases: %d, singing %.0f s of %.0f s;  tempo %.2f BPM, first beat at %.3f s" % (len(ph), sum(b - a for a, b in ph), L / SR, bpm, t0))
    # --- the singing, squeezed
    v = signal.sosfilt(signal.butter(2, 95, "hp", fs=SR, output="sos"), voc)
    b, a = biquad_peak(2800, 3.5, 0.9); v = signal.lfilter(b, a, v); b, a = biquad_peak(300, -2.0, 1.0); v = signal.lfilter(b, a, v)
    pre = rms(v[np.repeat(act, 441)[:len(v)] if len(act) * 441 >= len(v) else np.pad(np.repeat(act, 441), (0, len(v) - len(act) * 441))]) if act.any() else rms(v)
    v = compress(v, db(pre) - 6, 8.0, 0.002, 0.060); v = compress(v, db(pre) - 4, 3.0, 0.012, 0.220); v = np.tanh(v / (3 * rms(v))) * 3 * rms(v) * 0.9
    actmask = np.pad(np.repeat(act, 441), (0, max(0, len(v) - len(act) * 441)))[:len(v)] > 0
    print("singing: %.1f dB (original active %.1f dB), crest %.1f dB" % (db(rms(v[actmask])), db(rms(voc[actmask])), db(np.abs(v).max()) - db(rms(v[actmask]))))
    # --- the instruments, with room cut out for the voice
    inst = 1.0 * st["bass"] + 0.9 * st["drums"] + 0.8 * st["other"] + 0.8 * st["guitar"] + 0.8 * st["piano"]
    lo = signal.sosfilt(signal.butter(3, 600, "lp", fs=SR, output="sos"), inst); hi = signal.sosfilt(signal.butter(3, 5000, "hp", fs=SR, output="sos"), inst); mid = inst - lo - hi
    sc = smooth(actmask[::110].astype(float), 0.020, 0.150, SR / 110); duck = np.interp(np.arange(len(v)), (np.arange(len(sc)) + 0.5) * 110, sc)
    inst2 = lo + mid * 10 ** (-5.0 * duck / 20) + hi * 10 ** (-1.5 * duck / 20)
    ir = rms(inst2[actmask]); vr = rms(v[actmask]); gain = ir / vr * 10 ** (2.5 / 20)      # the voice sits 2.5 dB above the instruments while it sings
    mix = inst2 + v * gain; mix = np.tanh(mix / (0.9 * np.abs(mix).max()) * 1.4) / np.tanh(1.4)
    print("voice over instruments while singing: %+.1f dB (originally %+.1f dB)" % (db(rms(v[actmask]) * gain / ir), db(rms(voc[actmask]) / rms((st["bass"] + st["drums"] + st["other"] + st["guitar"] + st["piano"])[actmask]))))
    wav = "/tmp/corporations_mix.wav"; pcm = (np.clip(mix, -1, 1) * 32767).astype("<i2")
    with open(wav, "wb") as f: f.write(b"RIFF" + struct.pack("<I", 36 + pcm.nbytes) + b"WAVEfmt " + struct.pack("<IHHIIHH", 16, 1, 1, SR, SR * 2, 2, 16) + b"data" + struct.pack("<I", pcm.nbytes) + pcm.tobytes())
    x = encode_song.load(wav); b4 = encode(x); y = decode(b4); snr = 10 * np.log10(np.sum(x.astype(float) ** 2) / max(1, np.sum((x - y).astype(float) ** 2)))
    os.makedirs("source/music", exist_ok=True); open("source/music/corporations.adp", "wb").write(b4)
    print("saved source/music/corporations.adp: %.0f s, %d KB, ADPCM SNR %.1f dB" % (len(x) / GBA, len(b4) / 1024, snr))
    encode_song.add_line("corporations", "CORPORATIONS (YOU'RE WINNER)", "source/music/corporations.adp")
    # --- the vocal timing (and the singing alone as 2-bit clips, the theoretical split version)
    phr = []; clips = bytearray(); pks = []; vv = signal.sosfilt(signal.butter(4, [110, 3600], "bp", fs=SR, output="sos"), v)
    for i, (a, b_) in enumerate(ph):
        s0 = max(0, int((a - 0.05) * SR)); s1 = min(len(vv), int((b_ + 0.08) * SR)); seg = vv[s0:s1]; pk = float(np.abs(seg).max()) + 1e-9; seg = seg / pk * 0.9; pks.append(pk)
        seg8 = signal.resample_poly(seg, 8000, SR); q = (seg8 * 30000).astype(np.int32); data, n = enc2(q)
        beats = (a - t0) * bpm / 60.0
        phr.append(dict(i=i, start_s=round(a, 3), end_s=round(b_, 3), beat=round(beats, 3), sixteenth=int(round(beats * 4)), gba_sample=int(round((s0 / SR) * GBA)), gba_frame=int(round(s0 / SR * 60)),
                        vol_q8=0, clip_offset=len(clips), clip_samples=n, clip_bytes=len(data)))
        clips += data
    grid = [min(abs(p["beat"] * 4 - round(p["beat"] * 4)), 0.5) / 4 * 60 / bpm for p in phr]; print("phrase starts within 30 ms of a sixteenth: %d of %d (median offset %.0f ms)" % (sum(1 for g in grid if g < 0.030), len(grid), 1000 * float(np.median(grid))))
    vmax = max(pks)
    for p_, k_ in zip(phr, pks): p_["vol_q8"] = int(round(255 * k_ / vmax))      # per-clip loudness byte: the voice track plays each clip at this volume
    open("tools/corporations_vocal2.bin", "wb").write(bytes(clips))
    json.dump(dict(bpm=round(bpm, 3), first_beat_s=round(t0, 3), duration_s=round(L / SR, 2), gba_rate=GBA, clip_rate=8000, clip_format="2-bit adaptive delta (IMA step table, sign + 1 magnitude bit, index +3 / -1)", phrases=phr), open("tools/corporations_vocal_timing.json", "w"), indent=1)
    # check: decode clips, put them back at their times and compare with the processed singing (timing error in samples, SNR)
    tot_snr = []; lags = []
    for p in phr[:40]:
        d = dec2(clips[p["clip_offset"]:p["clip_offset"] + (p["clip_samples"] + 3) // 4], p["clip_samples"]).astype(float)
        a = int(p["gba_sample"] / GBA * SR); ref = signal.resample_poly(vv[a:a + int(p["clip_samples"] / 8000 * SR)], 8000, SR)[:len(d)] if a < len(vv) else None
        if ref is None or len(ref) < 100: continue
        n = min(len(ref), len(d)); r = ref[:n] / (np.abs(ref[:n]).max() + 1e-9); dd = d[:n] / (np.abs(d[:n]).max() + 1e-9)
        cc = signal.correlate(dd, r, "full"); lag = int(np.argmax(cc)) - (n - 1); lags.append(lag); tot_snr.append(10 * np.log10(np.sum(r ** 2) / max(1e-9, np.sum((r - dd) ** 2))))
    print("2-bit vocal clips: %d phrases, %.0f KB (%.1f s of singing at 8 kHz), SNR %.1f dB, timing error %d samples (max %d)" % (len(phr), len(clips) / 1024, sum(p["clip_samples"] for p in phr) / 8000, np.mean(tot_snr), int(np.median(lags)), int(np.max(np.abs(lags)))))
    if os.environ.get("PREVIEW"):
        def to_gba(wavpath, norm):
            af = "lowpass=f=7500" + (",loudnorm=I=-16:TP=-1.5" if norm else "")
            return np.frombuffer(subprocess.run(["ffmpeg", "-v", "error", "-i", wavpath, "-ac", "1", "-ar", str(GBA), "-af", af, "-f", "s16le", "-"], capture_output=True, check=True).stdout, dtype="<i2").astype(np.float64)
        def savewav(path, x):
            x = np.clip(x, -32768, 32767).astype("<i2")
            with open(path, "wb") as f: f.write(b"RIFF" + struct.pack("<I", 36 + x.nbytes) + b"WAVEfmt " + struct.pack("<IHHIIHH", 16, 1, 1, GBA, GBA * 2, 2, 16) + b"data" + struct.pack("<I", x.nbytes) + x.tobytes())
        savewav("/tmp/corporations_game.wav", y)                                           # what the single 4-bit stream sounds like on the GBA
        def wavfile(x, path):
            pcm = (np.clip(x, -1, 1) * 32767).astype("<i2")
            with open(path, "wb") as f: f.write(b"RIFF" + struct.pack("<I", 36 + pcm.nbytes) + b"WAVEfmt " + struct.pack("<IHHIIHH", 16, 1, 1, SR, SR * 2, 2, 16) + b"data" + struct.pack("<I", pcm.nbytes) + pcm.tobytes())
        wavfile(mix, "/tmp/c_mix.wav"); wavfile(inst2 / np.abs(mix).max() * 0.9, "/tmp/c_inst.wav")
        ref = to_gba("/tmp/c_mix.wav", False); xi = to_gba("/tmp/c_inst.wav", False); yi = decode(encode(xi.astype(np.int32))).astype(np.float64)      # the instrumental stream, 4-bit
        voc_track = np.zeros(len(xi))
        for p_ in phr:      # the vocal track: every clip decoded, put back at its start sample, at its own volume byte
            d = dec2(clips[p_["clip_offset"]:p_["clip_offset"] + (p_["clip_samples"] + 3) // 4], p_["clip_samples"]).astype(float) / 30000 / 0.9
            d = signal.resample(d, int(round(len(d) * GBA / 8000))); a0 = p_["gba_sample"]; n = min(len(d), len(voc_track) - a0)
            if n > 0: voc_track[a0:a0 + n] += d[:n] * (p_["vol_q8"] / 255.0)
        def fit(r_, y_):      # the loudness of each version is scaled to the reference before the error is measured
            n = min(len(r_), len(y_)); r_ = r_[:n]; y_ = y_[:n]; k = np.dot(r_, y_) / np.dot(y_, y_); return 10 * np.log10(np.sum(r_ ** 2) / np.sum((r_ - k * y_) ** 2)), k
        mask = np.abs(voc_track) > 1e-6; kv = np.dot(ref[mask] - yi[mask], voc_track[mask]) / max(1e-9, np.dot(voc_track[mask], voc_track[mask])); ysplit = yi + voc_track * kv
        snr_s, _ = fit(ref, ysplit); snr_m, km = fit(ref, y.astype(float)); y = y * km if False else y
        savewav("/tmp/corporations_split.wav", ysplit); print("preview: single 4-bit stream SNR vs the processed mix %.1f dB; split (instrumental 4-bit + 2-bit vocal track) %.1f dB (both after level fit)" % (snr_m, snr_s))
    print("instrumental + vocal-track total: ~%.0f KB (instrumental at 4-bit %.0f KB + vocals %.0f KB) against %.0f KB for the single mixed stream" % ((len(b4) + len(clips)) / 1024, len(b4) / 1024, len(clips) / 1024, len(b4) / 1024))

if __name__ == "__main__": main()
